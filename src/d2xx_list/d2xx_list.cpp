#include <shared/system.h>
#include <shared/system_specific.h>
#include <stdio.h>
#include <vector>
#include <inttypes.h>
#include <d2xx_shared.h>

int main() {
    FT_STATUS status;

    DWORD num_devices;
    status = FT_CreateDeviceInfoList(&num_devices);
    if (status != FT_OK) {
        fprintf(stderr, "FATAL: FT_CreateDeviceInfoList failed: %d\n", (int)status);
        return 1;
    }

    std::vector<FT_DEVICE_LIST_INFO_NODE> devices(num_devices);
    status = FT_GetDeviceInfoList(devices.data(), &num_devices);
    if (status != FT_OK) {
        fprintf(stderr, "FATAL: FT_GetDeviceInfoList failed: %d\n", (int)status);
        return 1;
    }

    printf("%zu devices:\n", devices.size());
    for (size_t device_index = 0; device_index < devices.size(); ++device_index) {
        const FT_DEVICE_LIST_INFO_NODE *device = &devices[device_index];

        printf("Device %zu:\n", device_index);

#if SYSTEM_WINDOWS
        FT_HANDLE handle = nullptr;
        status = FT_OpenEx((PVOID)device->SerialNumber, FT_OPEN_BY_SERIAL_NUMBER, &handle);
        if (status == FT_OK) {
            LONG com_port;
            status = FT_GetComPortNumber(handle, &com_port);
            if (status == FT_OK) {
                printf(" COM port: COM%ld\n", com_port);
            }
            FT_Close(handle);
            handle = nullptr;
        }
#endif

        printf(" Description: \"%s\"\n", device->Description);

        printf(" SerialNumber: %s\n", device->SerialNumber);

        printf(" (Type: 0x%x (%s); ", device->Type, GetFT_DEVICEEnumName(device->Type));

        // 1=OPENED; 2=HISPEED
        printf("Flags: 0x%x", device->Flags);
        {
            const char *sep = " - ";

            if (device->Flags & FT_FLAGS_OPENED) {
                printf("%sopened", sep);
                sep = ", ";
            }

            if (device->Flags & FT_FLAGS_HISPEED) {
                printf("%shi-speed", sep);
                sep = ", ";
            }
        }
        printf("; ");

        printf("ID: 0x%x; ", device->ID);

        printf("LocId: 0x%x)\n", device->LocId);

        // always seems to be NULL?
        //printf(" ftHandle: %p\n", device->ftHandle);
    }
}
