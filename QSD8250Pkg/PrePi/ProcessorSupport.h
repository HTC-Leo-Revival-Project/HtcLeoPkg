#pragma once
#ifndef _ARM_PROC_SUPPORT_H_
#define _ARM_PROC_SUPPORT_H_

#define CACHE_LINE 64

extern void ArmDeInitialize(void);
extern void ArmCleanInvalidateCacheRange(addr_t start, size_t len);

#endif