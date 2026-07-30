#include <shared/system.h>
#include <shared/CommandLineParser.h>
#include <shared/system_specific.h>
#include <shared/file_io.h>
#include <stdio.h>
#include <ftd2xx.h>
#include <vector>
#include <string>
#include <inttypes.h>
#include <map>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <shared/enum_def.h>
#define ENAME FT_STATUS
NBEGIN(FT_STATUS)
NN(FT_OK)
NN(FT_INVALID_HANDLE)
NN(FT_DEVICE_NOT_FOUND)
NN(FT_DEVICE_NOT_OPENED)
NN(FT_IO_ERROR)
NN(FT_INSUFFICIENT_RESOURCES)
NN(FT_INVALID_PARAMETER)
NN(FT_INVALID_BAUD_RATE)
NN(FT_DEVICE_NOT_OPENED_FOR_ERASE)
NN(FT_DEVICE_NOT_OPENED_FOR_WRITE)
NN(FT_FAILED_TO_WRITE_DEVICE)
NN(FT_EEPROM_READ_FAILED)
NN(FT_EEPROM_WRITE_FAILED)
NN(FT_EEPROM_ERASE_FAILED)
NN(FT_EEPROM_NOT_PRESENT)
NN(FT_EEPROM_NOT_PROGRAMMED)
NN(FT_INVALID_ARGS)
NN(FT_NOT_SUPPORTED)
NN(FT_OTHER_ERROR)
NN(FT_DEVICE_LIST_NOT_READY)
NEND()
#undef ENAME

#include <shared/enum_end.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const std::map<std::string, UCHAR> g_ft_bits_by_name{
    {"8", FT_BITS_8},
    {"7", FT_BITS_7},
};

static const std::map<std::string, UCHAR> g_ft_parity_by_name{
    {"none", FT_PARITY_NONE},
    {"odd", FT_PARITY_ODD},
    {"even", FT_PARITY_EVEN},
    {"mark", FT_PARITY_MARK},
    {"space", FT_PARITY_SPACE},
};

static const std::map<std::string, UCHAR> g_ft_stop_bits_by_name{
    {"1", FT_STOP_BITS_1},
    {"2", FT_STOP_BITS_2},
};

static const std::map<std::string, USHORT> g_ft_flow_control_by_name{
    {"none", FT_FLOW_NONE},
    {"rts/cts", FT_FLOW_RTS_CTS},
    {"dtr/dsr", FT_FLOW_DTR_DSR},
    //{"xon/xoff", FT_FLOW_XON_XOFF},//deliberately not available, as it'd require extra options
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

template <class ValueType>
static bool GetValueByName(ValueType *value, const std::map<std::string, ValueType> &map, const std::string &key, const char *what) {
    const auto &it = map.find(key);
    if (it == map.end()) {
        fprintf(stderr, "FATAL: invalid %s: %s\n", what, key.c_str());
        return false;
    }

    *value = it->second;
    return true;
}

template <class KeyType, class ValueType>
static bool GetKeyByValueFromMap(KeyType *key, const ValueType &value, const std::map<KeyType, ValueType> &map) {
    for (const auto &key_and_value : map) {
        if (key_and_value.second == value) {
            *key = key_and_value.first;
            return true;
        }
    }

    return false;
}

template <class ValueType>
static std::string GetKeys(const std::map<std::string, ValueType> &map) {
    std::string keys;

    for (const auto &key_and_value : map) {
        if (!keys.empty()) {
            keys += "; ";
        }

        keys += key_and_value.first;
    }

    return keys;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct DeviceOptions {
    UCHAR bits = FT_BITS_8;
    UCHAR parity = FT_PARITY_NONE;
    UCHAR stop_bits = FT_STOP_BITS_1;
    USHORT flow_control = FT_FLOW_RTS_CTS;
};

struct Options {
    std::vector<std::string> paths;
    int baud_rate = 115200;
    std::string device;
    bool open_by_serial_number = false;
    bool open_by_description = false;
    bool help = false;
    DeviceOptions device_options;
    //bool n7 = false;
    //bool odd = false;
    //bool even = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool GetDeviceList(std::vector<FT_DEVICE_LIST_INFO_NODE> *devices) {
    FT_STATUS status;

    DWORD num_devices;
    status = FT_CreateDeviceInfoList(&num_devices);
    if (status != FT_OK) {
        fprintf(stderr, "FATAL: FT_CreateDeviceInfoList failed: %d\n", (int)status);
        return false;
    }

    devices->resize(num_devices);
    status = FT_GetDeviceInfoList(devices->data(), &num_devices);
    if (status != FT_OK) {
        fprintf(stderr, "FATAL: FT_GetDeviceInfoList failed: %d\n", (int)status);
        return false;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static const FT_DEVICE_LIST_INFO_NODE *FindDeviceByCOMPortName(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &port_name) {
    FT_STATUS status;

    for (const FT_DEVICE_LIST_INFO_NODE &device : devices) {
        FT_HANDLE handle;
        status = FT_OpenEx((PVOID)device.SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &handle);
        if (status != FT_OK) {
            continue;
        }

        LONG port;
        status = FT_GetComPortNumber(handle, &port);

        FT_Close(handle), handle = nullptr;

        if (status != FT_OK) {
            continue;
        }

        std::string device_port_name = "COM" + std::to_string(port);
        if (strcasecmp(device_port_name.c_str(), port_name.c_str()) == 0) {
            return &device;
        }
    }

    return nullptr;
}

static const FT_DEVICE_LIST_INFO_NODE *FindDevice(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, size_t str_offset, const std::string &str) {
    for (const FT_DEVICE_LIST_INFO_NODE &device : devices) {
        const char *device_str = (const char *)&device + str_offset;

        if (strcasecmp(device_str, str.c_str()) == 0) {
            return &device;
        }
    }

    return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static bool DoCommandLine(int argc, char *argv[], Options *options) {
    CommandLineParser parser("send file over FTDI serial device", "[OPTIONS] DEVICE FILE0 [FILE1...]");

    std::string bits_str;
    GetKeyByValueFromMap(&bits_str, options->device_options.bits, g_ft_bits_by_name);

    std::string parity_str;
    GetKeyByValueFromMap(&parity_str, options->device_options.parity, g_ft_parity_by_name);

    std::string stop_bits_str;
    GetKeyByValueFromMap(&stop_bits_str, options->device_options.stop_bits, g_ft_stop_bits_by_name);

    std::string flow_control_str;
    GetKeyByValueFromMap(&flow_control_str, options->device_options.flow_control, g_ft_flow_control_by_name);

    parser.AddOption("baud").Meta("BAUD").Arg(&options->baud_rate).ShowDefault().Help("use baud rate BAUD");
    parser.AddOption("bits").Meta("BITS").Arg(&bits_str).ShowDefault().ShowDefaultStringUnquoted().Help("set bits to BITS (one of: " + GetKeys(g_ft_bits_by_name) + ")");
    parser.AddOption("parity").Meta("PARITY").Arg(&parity_str).ShowDefault().ShowDefaultStringUnquoted().Help("set parity to PARITY (one of: " + GetKeys(g_ft_parity_by_name) + ")");
    parser.AddOption("stop").Meta("BITS").Arg(&stop_bits_str).ShowDefault().ShowDefaultStringUnquoted().Help("set stop bits to BITS (one of: " + GetKeys(g_ft_stop_bits_by_name) + ")");
    parser.AddOption("flow").Meta("FLOW").Arg(&flow_control_str).ShowDefault().ShowDefaultStringUnquoted().Help("set flow control to FLOW (one of: " + GetKeys(g_ft_flow_control_by_name) + ")");
    parser.AddOption("serial-number").SetIfPresent(&options->open_by_serial_number).Help("DEVICE is device's serial number");
    parser.AddOption("description").SetIfPresent(&options->open_by_serial_number).Help("DEVICE is device's description");
    parser.AddHelpOption(&options->help);

    std::vector<std::string> other_args;
    if (!parser.Parse(argc, argv, &other_args)) {
        return false;
    }

    if (other_args.size() < 2) {
        fprintf(stderr, "FATAL: must specify DEVICE and FILE\n");
        return false;
    }

    if (!GetValueByName(&options->device_options.bits, g_ft_bits_by_name, bits_str, "bits")) {
        return false;
    }

    if (!GetValueByName(&options->device_options.stop_bits, g_ft_stop_bits_by_name, stop_bits_str, "stop bits")) {
        return false;
    }

    if (!GetValueByName(&options->device_options.parity, g_ft_parity_by_name, parity_str, "parity")) {
        return false;
    }

    if (!GetValueByName(&options->device_options.flow_control, g_ft_flow_control_by_name, flow_control_str, "flow control")) {
        return false;
    }

    options->device = other_args[0];

    options->paths = other_args;
    options->paths.erase(options->paths.begin());

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

static void PrintFailureMessage(FT_STATUS status, const std::string &call, const std::string &rest) {
    fprintf(stderr, "FATAL: %s failed (%ld; 0x%lx; %s)", call.c_str(), status, status, GetFT_STATUSEnumName(status));
    if (!rest.empty()) {
        fprintf(stderr, ": %s", rest.c_str());
    }
    fprintf(stderr, "\n");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[]) {
    FT_STATUS status;

    Options options;
    if (!DoCommandLine(argc, argv, &options)) {
        if (options.help) {
            return 0;
        } else {
            return 1;
        }
    }

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices;
    if (!GetDeviceList(&devices)) {
        return 1;
    }

    if (devices.empty()) {
        fprintf(stderr, "FATAL: no FTDI devices found\n");
        return 1;
    }

    const FT_DEVICE_LIST_INFO_NODE *device;
    if (options.open_by_description) {
        device = FindDevice(devices, offsetof(FT_DEVICE_LIST_INFO_NODE, Description), options.device);
    } else if (options.open_by_serial_number) {
        device = FindDevice(devices, offsetof(FT_DEVICE_LIST_INFO_NODE, SerialNumber), options.device);
    } else {
        device = FindDeviceByCOMPortName(devices, options.device);
    }

    if (!device) {
        fprintf(stderr, "FATAL: failed to find device: %s\n", options.device.c_str());
        return 1;
    }

    FT_HANDLE handle;
    status = FT_OpenEx((PVOID)device->SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &handle);
    if (status != FT_OK) {
        PrintFailureMessage(status, "FT_OpenEx", "device: " + options.device);
        return 1;
    }

    status = FT_SetBaudRate(handle, (DWORD)options.baud_rate);
    if (status != FT_OK) {
        PrintFailureMessage(status, "FT_SetBaudRate (" + std::to_string(options.baud_rate) + ")", "device: " + options.device);
        return 1;
    }

    status = FT_SetDataCharacteristics(handle, options.device_options.bits, options.device_options.stop_bits, options.device_options.parity);
    if (status != FT_OK) {
        PrintFailureMessage(status, "FT_SetDataCharacteristics", "device: " + options.device);
        return 1;
    }

    status = FT_SetFlowControl(handle, options.device_options.flow_control, 0, 0);
    if (status != FT_OK) {
        PrintFailureMessage(status, "FT_SetFlowControl", "device: " + options.device);
        return 1;
    }

    for (size_t path_index = 0; path_index < options.paths.size(); ++path_index) {
#if SYSTEM_WINDOWS
        // The paths are in the thread code page, but the file_io functions
        // assume the paths are UTF-8.
        const std::string &path = GetUTF8String(GetWideString(options.paths[path_index], CP_THREAD_ACP));
#else
        const std::string &path = options.paths[path._index];
#endif

        std::vector<uint8_t> data;
        if (!LoadFile(&data, path, nullptr)) {
            fprintf(stderr, "FATAL: failed to load file: %s\n", options.paths[path_index].c_str());
            return 1;
        }

        if (!data.empty()) {
            const uint8_t *p = data.data();
            size_t left = data.size();

            while (left > 0) {
                DWORD n;
                if (left > MAXDWORD) {
                    n = MAXDWORD;
                } else {
                    n = (DWORD)left;
                }
                DWORD num_written;
                status = FT_Write(handle, (LPVOID)p, n, &num_written);
                if (status != FT_OK) {
                    PrintFailureMessage(status, "FT_Write", "device: " + options.device);
                    return 1;
                }

                left -= num_written;
            }
        }
    }

    FT_Close(handle), handle = nullptr;
    return 0;
}