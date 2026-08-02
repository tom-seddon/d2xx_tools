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
    CommandLineParser parser("send file over FTDI serial device (Version: " + std::string(GetToolsVersionString()) + ")",
                             "[OPTIONS] DEVICE FILE0 [FILE1...]");

    AddDeviceSpecCommandLineOptions(&parser, &options->device_spec);
    AddDeviceOptionsCommandLineOptions(&parser, &options->device_options);

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

    bool show_progress = true;

    for (size_t path_index = 0; path_index < options.paths.size(); ++path_index) {
        const std::string &path = options.paths[path_index];
#if SYSTEM_WINDOWS
        // The paths are in the thread code page, but the file_io functions
        // assume the paths are UTF-8.
        const std::string &fopen_path = GetUTF8String(GetWideString(path, CP_THREAD_ACP));
#else
        const std::string &fopen_path = path;
#endif

        unsigned char buffer[65536];
        FILE *f = fopenUTF8(fopen_path.c_str(), "rb");
        if (!f) {
            fprintf(stderr, "FATAL: failed to open file: %s\n", path.c_str());
            return false;
        }

        uint64_t size = 0;
        if (show_progress) {
            if (fseek64(f, 0, SEEK_END) != 0) {
                fprintf(stderr, "FATAL: failed to get file size (1): %s\n", path.c_str());
                return false;
            }

            int64_t pos = ftell64(f);
            if (pos < 0) {
                fprintf(stderr, "FATAL: failed to get file size (2): %s\n", path.c_str());
                return false;
            }

            if (fseek64(f, 0, SEEK_SET) != 0) {
                fprintf(stderr, "FATAL: failed to get file size (3): %s\n", path.c_str());
                return false;
            }

            size = (uint64_t)pos;
            printf("%s:\n", path.c_str());
        }

        if (size > 0) {
            static const char PROGRESS_PREFIX[] = "  ";

            char size_str[MAX_UINT64_THOUSANDS_SIZE];
            GetThousandsString(size_str, size);

            uint64_t num_sent = 0;

            printf("%s0/%s", PROGRESS_PREFIX, size_str);

            while (!feof(f)) {
                size_t left = fread(buffer, 1, sizeof buffer, f);
                if (ferror(f)) {
                    fprintf(stderr, "FATAL: failed to read from file: %s\n", path.c_str());
                    return false;
                }

                const unsigned char *p = buffer;
                while (left > 0) {
                    DWORD num_written;
                    status = FT_Write(handle, (LPVOID)p, (DWORD)left, &num_written);
                    if (status != FT_OK) {
                        return PrintFTD2xxError(status, "FT_Write", options.device.c_str());
                    }

                    num_sent += num_written;
                    p += num_written;
                    left -= num_written;

                    if (show_progress) {
                        char num_sent_str[MAX_UINT64_THOUSANDS_SIZE];
                        GetThousandsString(num_sent_str, num_sent);
                        printf("\r%s%s/%s", PROGRESS_PREFIX, num_sent_str, size_str);
                    }
                }
            }

            printf("\n");
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
