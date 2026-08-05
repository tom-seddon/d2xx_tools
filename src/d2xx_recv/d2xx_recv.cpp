#include <shared/system.h>
#include <shared/CommandLineParser.h>
#include <shared/system_specific.h>
#include <shared/file_io.h>
#include <shared/debug.h>
#include <stdio.h>
#include <d2xx_shared.h>
#include <vector>
#include <string>
#include <inttypes.h>
#include <map>
#include <type_traits>
#if SYSTEM_WINDOWS
#include <conio.h>
#else
#include <signal.h>
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct Options {
    std::string path;
    bool help = false;
    std::string device;
    DeviceSpec device_spec;
    DeviceOptions device_options;
    size_t buffer_size=0;
    bool circular_buffer=false;
    bool buffer=false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool DoCommandLine(int argc, char *argv[], Options *options) {
    CommandLineParser parser("receive file over FTDI serial device (Version: " + std::string(GetToolsVersionString()) + ")",
                             "[OPTIONS] DEVICE FILE");

    AddDeviceSpecCommandLineOptions(&parser, &options->device_spec);
    AddDeviceOptionsCommandLineOptions(&parser, &options->device_options);

    parser.AddHelpOption(&options->help);
    parser.AddOption('b', "buffer").Arg(&options->buffer_size).Meta("N").SetIfPresent(&options->buffer).Help("read into a fixed-size buffer of N bytes, stopping once full");
    parser.AddOption('c',"circular").Arg(&options->buffer_size).Meta("N").SetIfPresent(&options->circular_buffer).Help("read indefinitely, keeping up to approx the last N bytes (exact amount kept may differ slightly)");

    std::vector<std::string> other_args;
    if (!parser.Parse(argc, argv, &other_args)) {
        return false;
    }

    if (other_args.size() != 2) {
        fprintf(stderr, "FATAL: must specify DEVICE and FILE\n");
        return false;
    }

    if (options->device_options.flow_control == FT_FLOW_XON_XOFF) {
        fprintf(stderr, "FATAL: XON/XOFF flow control not currently supported\n");
        return false;
    }
   
    if(options->buffer||options->circular_buffer){
        if(options->buffer&&options->circular_buffer){
            fprintf(stderr,"FATAL: --buffer and --circular are mutually exclusive\n");
            return false;
        }
        
        if(options->buffer_size==0){
            fprintf(stderr,"FATAL: invalid buffer size: %zu\n",options->buffer_size);
            return false;
        }
    }

    options->device = other_args[0];
    options->path = other_args[1];

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if !SYSTEM_WINDOWS
static std::atomic<bool> g_received_SIGINT{false};

static void HandleSIGINT(int){
    g_received_SIGINT.store(true,std::memory_order_release);
}
#endif

static bool ShouldQuit(){
#if SYSTEM_WINDOWS
    
    return _kbhit();
    
#else
    
    return g_received_SIGINT.load(std::memory_order_acquire);
    
#endif
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static constexpr double SECONDS_PER_PROGRESS_UPDATE = 0.5;

static bool ShowProgress(bool show_progress,bool force,uint64_t *last_progress_ticks){
    if(show_progress){
        uint64_t now_ticks=GetCurrentTickCount();
        if(force||GetSecondsFromTicks(now_ticks-*last_progress_ticks)>SECONDS_PER_PROGRESS_UPDATE){
            *last_progress_ticks=now_ticks;
            return true;
        }
    }
    
    return false;
}

static bool WriteDataToFile(const void *data,size_t data_size_bytes,FILE *f,const std::string&path){
    if(data_size_bytes>0){
        size_t num_written = fwrite(data,1,data_size_bytes,f);
        if (num_written != data_size_bytes) {
            fprintf(stderr, "FATAL: failed to write to file: %s\n", path.c_str());
            return false;
        }
    }
        
    return true;
}

static bool main2(int argc, char *argv[]) {
    FT_STATUS status;

    Options options;
    if (!DoCommandLine(argc, argv, &options)) {
        if (options.help) {
            return true;
        } else {
            return false;
        }
    }

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices;
    if (!GetDeviceList(&devices)) {
        return false;
    }

    FT_HANDLE handle = OpenDevice(devices, options.device, options.device_spec, options.device_options);
    if (!handle) {
        return false;
    }

    status = FT_SetTimeouts(handle, 250, 0);
    if (status != FT_OK) {
        return PrintFTD2xxError(status, "FT_SetTimeouts", options.device.c_str());
    }

    FILE *f = fopen(options.path.c_str(), "wb");
    if (!f) {
        fprintf(stderr, "FATAL: failed to open output file: %s\n", options.path.c_str());
        return false;
    }

    bool show_progress = true;

    static const char PROGRESS_PREFIX[] = "  Received bytes: ";

    uint64_t total_num_read = 0;
    if (show_progress) {
        printf("Reading from %s:\n", options.device.c_str());
        printf("%s0", PROGRESS_PREFIX);
    }

    uint64_t last_progress_ticks = GetCurrentTickCount();
    
#if !SYSTEM_WINDOWS
    {
        struct sigaction act={};
        act.sa_handler=&HandleSIGINT;
        if(sigaction(SIGINT,&act,nullptr)==-1){
            fprintf(stderr,"FATAL: failed to install SIGINT handler: %s\n",strerror(errno));
            return false;
        }
    }
#endif
    
    if(options.buffer){
        static constexpr DWORD NUM_TO_READ = 4096;
        std::vector<unsigned char> buffer(options.buffer_size);
        char buffer_size_str[MAX_UINT64_THOUSANDS_SIZE];
        GetThousandsString(buffer_size_str,options.buffer_size);
        size_t index = 0;
        while(index<buffer.size()&&!ShouldQuit()){
            size_t n = buffer.size() - index;
            if (n > NUM_TO_READ) {
                n = NUM_TO_READ;
            }

            DWORD num_read;
            status = FT_Read(handle, &buffer[index], (DWORD)n, &num_read);
            if (status != FT_OK) {
                return PrintFTD2xxError(status, "FT_Read", options.device.c_str());
            }

            index += num_read;

            if(ShowProgress(show_progress,index==buffer.size(),&last_progress_ticks)){
                char total_num_read_str[MAX_UINT64_THOUSANDS_SIZE];
                GetThousandsString(total_num_read_str, index);
                
                printf("\r%s%s/%s", PROGRESS_PREFIX, total_num_read_str,buffer_size_str);
                fflush(stdout);
            }
        }

        if(!WriteDataToFile(buffer.data(),index,f,options.path)){
            return false;
        }
    }else if(options.circular_buffer){
        static constexpr DWORD NUM_TO_READ=4096;
        std::vector<unsigned char>buffer(options.buffer_size);
        size_t index=0;
        bool ever_wrapped=false;
        while(!ShouldQuit()){
            size_t n=buffer.size()-index;
            if(n>NUM_TO_READ){
                n=NUM_TO_READ;
            }
            
            DWORD num_read;
            status=FT_Read(handle,&buffer[index],(DWORD)n,&num_read);
            if(status!=FT_OK){
                return PrintFTD2xxError(status,"FT_Read",options.device.c_str());
            }
            
            index+=num_read;
            ASSERT(index<=buffer.size());
            if(index==buffer.size()){
                index=0;
                ever_wrapped=true;
            }
            
            if(ShowProgress(show_progress,false,&last_progress_ticks)){
                char total_num_read_str[MAX_UINT64_THOUSANDS_SIZE];
                GetThousandsString(total_num_read_str, index);
                
                printf("\r%s%s", PROGRESS_PREFIX, total_num_read_str);
                fflush(stdout);
            }
        }
        
        if(ever_wrapped){
            if(!WriteDataToFile(&buffer[index],buffer.size()-index,f,options.path)){
                return false;
            }
               
            if(!WriteDataToFile(&buffer[0],index,f,options.path)){
                return false;
            }
        }else{
            if(!WriteDataToFile(buffer.data(),index,f,options.path)){
                return false;
            }
        }
    } else {
        while(!ShouldQuit()){
            unsigned char buffer[65536];
            DWORD num_read;
            status = FT_Read(handle, buffer, sizeof buffer, &num_read);
            if (status != FT_OK) {
                return PrintFTD2xxError(status, "FT_Read", options.device.c_str());
                return false;
            }

            if(!WriteDataToFile(buffer,num_read,f,options.path)){
                return false;
            }

            total_num_read += num_read;

            if(ShowProgress(show_progress,false,&last_progress_ticks)){
                char total_num_read_str[MAX_UINT64_THOUSANDS_SIZE];
                GetThousandsString(total_num_read_str, total_num_read);
                
                printf("\r%s%s", PROGRESS_PREFIX, total_num_read_str);
                fflush(stdout);
            }
        }
    }

    printf("\n");

    fclose(f), f = nullptr;

    FT_Close(handle), handle = nullptr;

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    bool good = main2(argc, argv);
    if (good) {
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
