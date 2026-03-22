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
    char *cmdline;
} HtcDevice;

#endif