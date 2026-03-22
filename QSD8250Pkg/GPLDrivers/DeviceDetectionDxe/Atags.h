#ifndef __DEVICE_DETECTION_ATAGS_H__
#define __DEVICE_DETECTION_ATAGS_H__
VOID ParseAtags(UINT32 StartAddr, HtcDevice *mDevice);
BOOLEAN DetectCLK(char* cmdline);
DeviceType AtagsDetectDevice(char *cmdline);
#endif