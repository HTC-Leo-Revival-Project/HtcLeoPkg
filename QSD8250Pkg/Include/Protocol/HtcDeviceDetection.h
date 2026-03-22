#ifndef __HTC_PROTOCOL_DEVICE_DETECTION_H__
#define __HTC_PROTOCOL_DEVICE_DETECTION_H__
#include <Device/DeviceType.h>

#define HTC_DEVICE_DETECTION_PROTOCOL_GUID                                             \
  {                                                                            \
    0x2c898318, 0x41c1, 0x4309,                                                \
    {                                                                          \
      0x89, 0x8a, 0x2f, 0x55, 0xc8, 0xcf, 0x0b, 0x88                           \
    }                                                                          \
  }

typedef struct _HTC_DEVICE_DETECTION_PROTOCOL HTC_DEVICE_DETECTION_PROTOCOL;

typedef HtcDevice(*device_detection_get_info_t)(VOID);


struct _HTC_DEVICE_DETECTION_PROTOCOL {
  device_detection_get_info_t GetInfo;
};

extern EFI_GUID gHtcDeviceDetectionProtocolGuid;

#endif