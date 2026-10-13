#include <Base.h>
#include <Library/TimerLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/GptTimerLib.h>

#define GPT_REG(off)        (((UINTN)PcdGet32(PcdMsmGptBase)) + (off))

#define GPT_MATCH_VAL        GPT_REG((UINTN)PcdGet32(PcdMsmGptMatchValOffset))
#define GPT_COUNT_VAL        GPT_REG((UINTN)PcdGet32(PcdMsmGptCountValOffset))
#define GPT_ENABLE           GPT_REG((UINTN)PcdGet32(PcdMsmGptEnableOffset))
#define GPT_CLEAR            GPT_REG((UINTN)PcdGet32(PcdMsmGptClearOffset))

#define GPT_ENABLE_CLR_ON_MATCH_EN        2
#define GPT_ENABLE_EN                     1

RETURN_STATUS
EFIAPI
TimerEarlyInit (
  VOID
  )
{
  // disable counter
  MmioWrite32(GPT_CLEAR, 0);
  MmioWrite32(GPT_ENABLE, 0);
  while (MmioRead32(GPT_COUNT_VAL) != 0);

  // enable counter
  MmioWrite32(GPT_ENABLE, GPT_ENABLE_EN);

  return RETURN_SUCCESS;
}

UINTN
EFIAPI
MicroSecondDelay (
  IN      UINTN                     MicroSeconds
  )
{
  volatile UINTN Count;
  UINT64 InitCount;
  UINT64 Timeout;
  UINT64 Ticks;

  // calculate number of ticks we have to wait
  Ticks = ((UINT64)MicroSeconds * 33 + 1000 - 33) / 1000;

  // get current counter value
  Count = MmioRead32(GPT_COUNT_VAL);
  InitCount = Count;

  // Calculate timeout = cnt + ticks (mod 2^56)
  // to account for timer counter wrapping
  Timeout = (Count + Ticks) & (UINTN)(0xFFFFFFFF);

  // Wait out till the counter wrapping occurs
  // in cases where there is a wrapping.
  while (Timeout < Count && InitCount <= Count)
    Count = MmioRead32(GPT_COUNT_VAL);

  // Wait till the number of ticks is reached
  while (Timeout > Count)
    Count = MmioRead32(GPT_COUNT_VAL);

  return MicroSeconds;
}

UINTN
EFIAPI
NanoSecondDelay (
  IN      UINTN                     NanoSeconds
  )
{
  UINTN  MicroSeconds;

  // Round up to 1us Tick Number
  MicroSeconds = NanoSeconds / 1000;
  MicroSeconds += ((NanoSeconds % 1000) == 0) ? 0 : 1;

  MicroSecondDelay (MicroSeconds);

  return NanoSeconds;
}

UINT64
EFIAPI
GetPerformanceCounter (
  VOID
  )
{
  return (UINT64)MmioRead32(GPT_COUNT_VAL);
}

UINT64
EFIAPI
GetPerformanceCounterProperties (
  OUT      UINT64                    *StartValue,  OPTIONAL
  OUT      UINT64                    *EndValue     OPTIONAL
  )
{
  if (StartValue != NULL) {
    *StartValue = 0;
  }

  if (EndValue != NULL) {
    *EndValue = 0xFFFFFFFF;
  }

  return (UINT64)32765;
}

UINT64
EFIAPI
GetTimeInNanoSecond (
  IN      UINT64                     Ticks
  )
{
  UINT64  NanoSeconds;
  UINT32  Remainder;
  UINT64  Frequency;

  Frequency = GetPerformanceCounterProperties(NULL, NULL);

  //
  //          Ticks
  // Time = --------- x 1,000,000,000
  //        Frequency
  //
  NanoSeconds = MultU64x32 (DivU64x32Remainder (Ticks, Frequency, &Remainder), 1000000000u);

  //
  // Frequency < 0x100000000, so Remainder < 0x100000000, then (Remainder * 1,000,000,000)
  // will not overflow 64-bit.
  //
  NanoSeconds += DivU64x32 (MultU64x32 ((UINT64) Remainder, 1000000000u), Frequency);

  return NanoSeconds;
}