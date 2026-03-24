#ifndef __DEVICE_TYPE_H__
#define __DEVICE_TYPE_H__
#include <Library/Atags.h>

typedef enum {
    LEO,
    SCHUBERT,
    BRAVO,
    PASSION,
    UNKNOWN
} DeviceType;

typedef struct {
    DeviceType type;
    BOOLEAN cLK;
    BOOLEAN nandBoot;
    CHAR8 *cmdline;
    CHAR8* Imei;
    CHAR8* Cid;
    CHAR8* SerialNumber;
} HtcDevice;

#endif