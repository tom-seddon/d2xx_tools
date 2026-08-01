#include <shared/system.h>
#include <shared/system_specific.h>
#include "d2xx_shared.h"
#include <shared/CommandLineParser.h>

#include <shared/enum_def.h>
#include "d2xx_shared.inl"
#include <shared/enum_end.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AddDeviceSpecCommandLineOptions(CommandLineParser *parser, DeviceSpec *device_spec) {
    parser->AddOption("serial-number").SetIfPresent(&device_spec->open_by_serial_number).Help("DEVICE is device's serial number");
    parser->AddOption("description").SetIfPresent(&device_spec->open_by_serial_number).Help("DEVICE is device's description");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void AddDeviceOptionsCommandLineOptions(CommandLineParser *parser, DeviceOptions *device_options) {
    parser->AddOption("baud").Meta("BAUD").Arg(&device_options->baud_rate).ShowDefault().Help("use baud rate BAUD");
    parser->AddOption("bits").Meta("BITS").EnumArg(&device_options->bits, GetFT_BITSEnumTraits()).ShowDefault().Help("set bits to BITS");
    parser->AddOption("parity").Meta("PARITY").EnumArg(&device_options->parity, GetFT_PARITYEnumTraits()).ShowDefault().Help("set parity to PARITY");
    parser->AddOption("stop").Meta("BITS").EnumArg(&device_options->stop_bits, GetFT_STOP_BITSEnumTraits()).ShowDefault().Help("set stop bits to BITS");
    parser->AddOption("flow").Meta("FLOW").EnumArg(&device_options->flow_control, GetFT_FLOWEnumTraits()).ShowDefault().Help("set flow control to FLOW");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool PrintFTD2xxError(FT_STATUS status, const char *what, const char *device) {
    fprintf(stderr, "FATAL: %s failed", what);
    if (device) {
        fprintf(stderr, " (device: %s)", device);
    }
    fprintf(stderr, ": %lu (%s)\n", status, GetFT_STATUSEnumName(status));

    return false;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

bool GetDeviceList(std::vector<FT_DEVICE_LIST_INFO_NODE> *devices) {
    FT_STATUS status;

    DWORD num_devices;
    status = FT_CreateDeviceInfoList(&num_devices);
    if (status != FT_OK) {
        return PrintFTD2xxError(status, "FT_CreateDeviceInfoList");
    }

    if (num_devices == 0) {
        fprintf(stderr, "FATAL: no FTDI devices found\n");
        return false;
    }

    devices->resize(num_devices);
    status = FT_GetDeviceInfoList(devices->data(), &num_devices);
    if (status != FT_OK) {
        return PrintFTD2xxError(status, "FT_GetDeviceInfoList");
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

const FT_DEVICE_LIST_INFO_NODE *FindDeviceByCOMPortName(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &port_name) {
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

const FT_DEVICE_LIST_INFO_NODE *FindDeviceBySerialNumber(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &serial_number) {
    const FT_DEVICE_LIST_INFO_NODE *device = FindDevice(devices, offsetof(FT_DEVICE_LIST_INFO_NODE, SerialNumber), serial_number);
    return device;
}

const FT_DEVICE_LIST_INFO_NODE *FindDeviceByDescription(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &description) {
    const FT_DEVICE_LIST_INFO_NODE *device = FindDevice(devices, offsetof(FT_DEVICE_LIST_INFO_NODE, Description), description);
    return device;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

FT_HANDLE OpenDevice(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &name, const DeviceSpec &spec, const DeviceOptions &options) {
    FT_STATUS status;

    const FT_DEVICE_LIST_INFO_NODE *device;
    if (spec.open_by_description) {
        device = FindDeviceByDescription(devices, name);
    } else if (spec.open_by_serial_number) {
        device = FindDeviceBySerialNumber(devices, name);
    } else {
        device = FindDeviceByCOMPortName(devices, name);
    }

    if (!device) {
        fprintf(stderr, "FATAL: failed to find device: %s\n", name.c_str());
        return nullptr;
    }

    FT_HANDLE handle;
    status = FT_OpenEx((PVOID)device->SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &handle);
    if (status != FT_OK) {
        PrintFTD2xxError(status, "FT_OpenEx", name.c_str());
        return nullptr;
    }

    status = FT_SetBaudRate(handle, (DWORD)options.baud_rate);
    if (status != FT_OK) {
        PrintFTD2xxError(status, "FT_SetBaudRate", name.c_str());
        FT_Close(handle);
        return nullptr;
    }

    status = FT_SetDataCharacteristics(handle, options.bits, options.stop_bits, options.parity);
    if (status != FT_OK) {
        PrintFTD2xxError(status, "FT_SetDataCharacteristics", name.c_str());
        FT_Close(handle);
        return nullptr;
    }

    status = FT_SetFlowControl(handle, options.flow_control, 0, 0);
    if (status != FT_OK) {
        PrintFTD2xxError(status, "FT_SetFlowControl", name.c_str());
        FT_Close(handle);
        return nullptr;
    }

    return handle;
}
