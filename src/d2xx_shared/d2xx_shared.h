#ifndef HEADER_4303D89665584109AA25FE613937C5A5 // -*- mode:c++ -*-
#define HEADER_4303D89665584109AA25FE613937C5A5

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#if defined _WIN32 && !defined _WINDOWS_
#error Include windows.h
#endif
#include <ftd2xx.h>

#include <vector>

#include <shared/enum_decl.h>
#include "d2xx_shared.inl"
#include <shared/enum_end.h>

class CommandLineParser;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

struct DeviceOptions {
    int baud_rate = 115200;
    UCHAR bits = FT_BITS_8;
    UCHAR parity = FT_PARITY_NONE;
    UCHAR stop_bits = FT_STOP_BITS_1;
    USHORT flow_control = FT_FLOW_RTS_CTS;
};

void AddDeviceOptionsCommandLineOptions(CommandLineParser *parser, DeviceOptions *device_options);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// always returns false.
bool PrintFTD2xxError(FT_STATUS status, const char *what, const char *device = nullptr);

bool GetDeviceList(std::vector<FT_DEVICE_LIST_INFO_NODE> *devices);

#if SYSTEM_WINDOWS
const FT_DEVICE_LIST_INFO_NODE *FindDeviceByCOMPortName(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &port_name);
#endif

const FT_DEVICE_LIST_INFO_NODE *FindDeviceBySerialNumber(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &serial_number);
const FT_DEVICE_LIST_INFO_NODE *FindDeviceByDescription(const std::vector<FT_DEVICE_LIST_INFO_NODE> &devices, const std::string &description);

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#endif
