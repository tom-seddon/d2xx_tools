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
#if SYSTEM_WINDOWS
#include <conio.h>
#endif

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct Options {
    std::string path;
    bool help = false;
    std::string device;
    DeviceSpec device_spec;
    DeviceOptions device_options;
    size_t max_num_bytes = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool DoCommandLine(int argc, char *argv[], Options *options) {
    CommandLineParser parser("receive file over FTDI serial device (Version: " + std::string(GetToolsVersionString()) + ")",
                             "[OPTIONS] DEVICE FILE");

    AddDeviceSpecCommandLineOptions(&parser, &options->device_spec);
    AddDeviceOptionsCommandLineOptions(&parser, &options->device_options);

    parser.AddHelpOption(&options->help);
    parser.AddOption('n', "num-bytes").Arg(&options->max_num_bytes).Meta("N").Help("finish once N bytes have been read");

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

    options->device = other_args[0];
    options->path = other_args[1];

    return true;
}

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
    double seconds_per_progress_update = 0.5;

    if (options.max_num_bytes > 0) {
        static constexpr DWORD NUM_TO_READ = 65536;
        std::vector<unsigned char> buffer(options.max_num_bytes);
        size_t index = 0;
        while (index < buffer.size()) {
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

            if (show_progress || index == buffer.size()) {
                uint64_t now_ticks = GetCurrentTickCount();
                if (GetSecondsFromTicks(now_ticks - last_progress_ticks) > seconds_per_progress_update) {
                    char total_num_read_str[MAX_UINT64_THOUSANDS_SIZE];
                    GetThousandsString(total_num_read_str, index);

                    printf("\r%s%s", PROGRESS_PREFIX, total_num_read_str);
                    fflush(stdout);

                    last_progress_ticks = now_ticks;
                }
            }
        }

        size_t num_written = fwrite(buffer.data(), 1, buffer.size(), f);
        if (num_written != buffer.size()) {
            fprintf(stderr, "FATAL: failed to write to file: %s\n", options.path.c_str());
            return false;
        }

    } else {
        for (;;) {
            unsigned char buffer[65536];
            DWORD num_read;
            status = FT_Read(handle, buffer, sizeof buffer, &num_read);
            if (status != FT_OK) {
                return PrintFTD2xxError(status, "FT_Read", options.device.c_str());
                return false;
            }

            if (num_read > 0) {
                size_t num_written = fwrite(buffer, 1, num_read, f);
                if (num_written != num_read) {
                    fprintf(stderr, "FATAL: failed to write to file: %s\n", options.path.c_str());
                    return false;
                }
            }

            total_num_read += num_read;

            if (show_progress) {
                uint64_t now_ticks = GetCurrentTickCount();
                if (GetSecondsFromTicks(now_ticks / last_progress_ticks) > seconds_per_progress_update) {
                    char total_num_read_str[MAX_UINT64_THOUSANDS_SIZE];
                    GetThousandsString(total_num_read_str, total_num_read);

                    printf("\r%s%s", PROGRESS_PREFIX, total_num_read_str);
                    fflush(stdout);

                    last_progress_ticks = now_ticks;
                }
            }

#if SYSTEM_WINDOWS
            if (_kbhit()) {
                break;
            }
#endif
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
