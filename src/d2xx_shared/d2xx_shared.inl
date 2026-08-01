//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_STATUS
NBEGIN()
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_BITS
NBEGIN_DERIVED(UCHAR)
NN(FT_BITS_8)
EUI_NAME("8")
NN(FT_BITS_7)
EUI_NAME("7")
NEND()
#undef ENAME

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_PARITY
NBEGIN_DERIVED(UCHAR)
NN(FT_PARITY_NONE)
EUI_NAME("none")
NN(FT_PARITY_ODD)
EUI_NAME("odd")
NN(FT_PARITY_EVEN)
EUI_NAME("even")
NN(FT_PARITY_MARK)
EUI_NAME("mark")
NN(FT_PARITY_SPACE)
EUI_NAME("space")
NEND()
#undef ENAME

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_STOP_BITS
NBEGIN_DERIVED(UCHAR)
NN(FT_STOP_BITS_1)
EUI_NAME("1")
NN(FT_STOP_BITS_2)
EUI_NAME("2")
NEND()
#undef ENAME

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_FLOW
NBEGIN_DERIVED(USHORT)
NN(FT_FLOW_NONE)
EUI_NAME("none")
NN(FT_FLOW_RTS_CTS)
EUI_NAME("rts_cts")
NN(FT_FLOW_DTR_DSR)
EUI_NAME("dtr_dsr")
NN(FT_FLOW_XON_XOFF)
EUI_NAME("xon_xoff")
NEND()
#undef ENAME

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#define ENAME FT_DEVICE
NBEGIN()
NN(FT_DEVICE_BM)
NN(FT_DEVICE_AM)
NN(FT_DEVICE_100AX)
NN(FT_DEVICE_UNKNOWN)
NN(FT_DEVICE_2232C)
NN(FT_DEVICE_232R)
NN(FT_DEVICE_2232H)
NN(FT_DEVICE_4232H)
NN(FT_DEVICE_232H)
NN(FT_DEVICE_X_SERIES)
NN(FT_DEVICE_4222H_0)
NN(FT_DEVICE_4222H_1_2)
NN(FT_DEVICE_4222H_3)
NN(FT_DEVICE_4222_PROG)
NN(FT_DEVICE_900)
NN(FT_DEVICE_930)
NN(FT_DEVICE_UMFTPD3A)
NN(FT_DEVICE_2233HP)
NN(FT_DEVICE_4233HP)
NN(FT_DEVICE_2232HP)
NN(FT_DEVICE_4232HP)
NN(FT_DEVICE_233HP)
NN(FT_DEVICE_232HP)
NN(FT_DEVICE_2232HA)
NN(FT_DEVICE_4232HA)
NN(FT_DEVICE_232RN)
NN(FT_DEVICE_2233HPN)
NN(FT_DEVICE_4233HPN)
NN(FT_DEVICE_2232HPN)
NN(FT_DEVICE_4232HPN)
NN(FT_DEVICE_233HPN)
NN(FT_DEVICE_232HPN)
NN(FT_DEVICE_BM_A)
NEND()
#undef ENAME
