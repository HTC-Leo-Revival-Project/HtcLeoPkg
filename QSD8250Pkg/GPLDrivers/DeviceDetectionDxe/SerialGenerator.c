#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#define SERIAL_LEN 16

STATIC
UINT32
HashString(CONST CHAR8 *Str) {
  UINT32 Hash = 5381;
  while (*Str != '\0') {
    Hash = ((Hash << 5) + Hash) + (UINT8)(*Str); // djb2
    Str++;
  }
  return Hash;
}

STATIC
UINT32
SeedFromInputs(CONST CHAR8 *Cid, CONST CHAR8 *Imei) {
  return HashString(Cid) ^ HashString(Imei);
}

STATIC
UINT32
LcgNext(UINT32 *Seed) {
  *Seed = (*Seed * 1664525u) + 1013904223u;
  return *Seed;
}

CHAR8*
GenerateAndroidSerial(CONST CHAR8 *Cid, CONST CHAR8 *Imei) {
  STATIC CONST CHAR8 Charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

  if (Cid == NULL || Imei == NULL) {
    return NULL;
  }

  UINT32 Seed = SeedFromInputs(Cid, Imei);

  CHAR8 *Serial = AllocateZeroPool(SERIAL_LEN + 1);
  if (Serial == NULL) {
    return NULL;
  }

  for (UINTN i = 0; i < SERIAL_LEN; i++) {
    UINT32 Rnd = LcgNext(&Seed);
    Serial[i] = Charset[Rnd % (sizeof(Charset) - 1)];
  }

  Serial[SERIAL_LEN] = '\0';
  return Serial;
}