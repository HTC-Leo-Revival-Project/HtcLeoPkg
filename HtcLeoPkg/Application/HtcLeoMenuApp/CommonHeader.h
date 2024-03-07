#pragma once

#include <Uefi.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>


typedef struct {
	int x, y;
} Point;

void PutCharX( IN int x, IN int y, IN CHAR16 ch );
BOOLEAN IsAlphanumeric( IN CHAR16 ch );
void SetCursorPos( int x, int y );
void SetTextColor( UINT8 color );