#include <shared/system.h>
#include <shared/CommandLineParser.h>
#include <shared/system_specific.h>
#include <shared/file_io.h>
#include <stdio.h>
#include <d2xx_shared.h>
#include <vector>
#include <string>
#include <inttypes.h>
#include <map>
#include <type_traits>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct Options {
    std::vector<std::string> paths;
    bool help = false;
    std::string device;
    DeviceSpec device_spec;
    DeviceOptions device_options;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool DoCommandLine(int argc, char *argv[], Options *options) {
    CommandLineParser parser("send file over FTDI serial device", "[OPTIONS] DEVICE FILE0 [FILE1...]");

    parser.AddHelpOption(&options->help);

    std::vector<std::string> other_args;
    if (!parser.Parse(argc, argv, &other_args)) {
        return false;
    }

    if (other_args.size() < 2) {
        fprintf(stderr, "FATAL: must specify DEVICE and FILE\n");
        return false;
    }

    if (options->device_options.flow_control == FT_FLOW_XON_XOFF) {
        fprintf(stderr, "FATAL: XON/XOFF flow control not currently supported\n");
        return false;
    }

    options->device = other_args[0];

    options->paths = other_args;
    options->paths.erase(options->paths.begin());

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

//static void PrintFailureMessage(FT_STATUS status, const std::string &call, const std::string &rest) {
//    fprintf(stderr, "FATAL: %s failed (%ld; 0x%lx; %s)", call.c_str(), status, status, GetFT_STATUSEnumName(status));
//    if (!rest.empty()) {
//        fprintf(stderr, ": %s", rest.c_str());
//    }
//    fprintf(stderr, "\n");
//}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

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

    for (size_t path_index = 0; path_index < options.paths.size(); ++path_index) {
#if SYSTEM_WINDOWS
        // The paths are in the thread code page, but the file_io functions
        // assume the paths are UTF-8.
        const std::string &path = GetUTF8String(GetWideString(options.paths[path_index], CP_THREAD_ACP));
#else
        const std::string &path = options.paths[path._index];
#endif

        unsigned char buffer[65536];
        FILE *f = fopenUTF8(path.c_str(), "rb");
        if (!f) {
            fprintf(stderr, "FATAL: failed to open file: %s\n", options.paths[path_index].c_str());
            return false;
        }

        while (!feof(f)) {
            size_t left = fread(buffer, 1, sizeof buffer, f);
            if (ferror(f)) {
                fprintf(stderr, "FATAL: failed to read from file: %s\n", options.paths[path_index].c_str());
                return false;
            }

            const unsigned char *p = buffer;
            while (left > 0) {
                DWORD num_written;
                status = FT_Write(handle, (LPVOID)p, (DWORD)left, &num_written);
                if (status != FT_OK) {
                    return PrintFTD2xxError(status, "FT_Write", options.device.c_str());
                }

                p += num_written;
                left -= num_written;
            }
        }
    }

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
