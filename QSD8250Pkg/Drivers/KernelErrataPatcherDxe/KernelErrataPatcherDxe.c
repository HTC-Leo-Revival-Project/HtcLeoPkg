/** @file
  Patches Windows RT ntoskrnl KiSetFeatureBits to bypass 0x5D
  (UNSUPPORTED_PROCESSOR) bugcheck on Qualcomm Scorpion (QSD8250).

  Hooks ExitBootServices. When winload calls it, ntoskrnl is already
  loaded in memory. The driver
   1. NOPs the three KiSetFeatureBits branches that bugcheck 0x5D(2) on
      ID_MMFR2 TLB level and ID_MMFR3 broadcast / coherent walk.
   2. Rewrites, in ntoskrnl's executable sections, every CP15 op the
      Scorpion does not implement (UNDEF on a real HD2, CacheCoherencyTest)
      into its local equivalent; same 4-byte MCR, only the 2nd halfword
      changes.  The core is uniprocessor, so local == broadcast:
        TLBIALLIS  c8,c3,0 -> TLBIALL  c8,c7,0
        TLBIMVAIS  c8,c3,1 -> TLBIMVA  c8,c7,1
        TLBIASIDIS c8,c3,2 -> TLBIASID c8,c7,2
        TLBIMVAAIS c8,c3,3 -> TLBIALL  c8,c7,0  (MVA+any ASID: no local op;
        TLBIMVAA   c8,c7,3 -> TLBIALL  c8,c7,0   TLBIMVA misses non-global)
        ICIALLUIS  c7,c1,0 -> ICIALLU  c7,c5,0
        BPIALLIS   c7,c1,6 -> BPIALL   c7,c5,6
  Coherent walk itself works on the Scorpion with TTBR0.S=1, which Windows
  inherits from the firmware (KiSetFeatureBits keeps TTBR0 & 0x7F).

  Copyright (c) 2024, J0SH1X. All rights reserved.
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/CacheMaintenanceLib.h>

/* ------------------------------------------------------------------ */
/*  Byte patterns for the four failing KiSetFeatureBits checks.       */
/*  Each pattern is the MRC + AND + CMP that precedes the BCC.W we    */
/*  want to NOP.  The BCC.W itself is the 4 bytes right after.        */
/* ------------------------------------------------------------------ */

/*
 * Check 1 – ID_MMFR2[19:16] (TLB maintenance) >= 3
 *   MRC p15,0,R3,c0,c1,6   10 EE D1 3F
 *   AND.W R3,R3,#0xF0000   03 F4 70 23
 *   CMP.W R3,#0x30000      B3 F5 40 3F
 *   BCC.W <bugcheck>        xx xx xx xx  ← NOP this
 */
STATIC CONST UINT8 PatMmfr2Tlb[] = {
  0x10, 0xEE, 0xD1, 0x3F,
  0x03, 0xF4, 0x70, 0x23,
  0xB3, 0xF5, 0x40, 0x3F
};

/*
 * Check 2 – ID_MMFR3[15:12] (maintenance broadcast) >= 2
 *   MRC p15,0,R3,c0,c1,7   10 EE F1 3F
 *   AND.W R3,R3,#0xF000    03 F4 70 43
 *   CMP.W R3,#0x2000       B3 F5 00 5F
 *   BCC.W <bugcheck>        xx xx xx xx  ← NOP this
 */
STATIC CONST UINT8 PatMmfr3Bcast[] = {
  0x10, 0xEE, 0xF1, 0x3F,
  0x03, 0xF4, 0x70, 0x43,
  0xB3, 0xF5, 0x00, 0x5F
};

/*
 * Check 3 – ID_MMFR3[23:20] (coherent page-table walk) >= 1
 *   MRC p15,0,R3,c0,c1,7   10 EE F1 3F
 *   AND.W R3,R3,#0xF00000  03 F4 70 03
 *   CMP.W R3,#0x100000     B3 F5 80 1F
 *   BCC.W <bugcheck>        xx xx xx xx  ← NOP this
 */
STATIC CONST UINT8 PatMmfr3Walk[] = {
  0x10, 0xEE, 0xF1, 0x3F,
  0x03, 0xF4, 0x70, 0x03,
  0xB3, 0xF5, 0x80, 0x1F
};

/*
 * Check 4 - CTR[15:14] L1 I-cache policy must be 2 (VIPT) or 3 (PIPT);
 * Scorpion reports 1 (ASID-tagged VIVT) -> 0x5D(8, CTR).  Safe to accept:
 * KeSweepIcacheRange does a full ICIALLU whenever CTR is not PIPT.
 *   UBFX R3,R4,#14,#2   C4 F3 81 33
 *   CMP  R3,#2          02 2B
 *   BNE.W <check PIPT / bugcheck 8>   <- NOP this
 */
STATIC CONST UINT8 PatCtrL1Ip[] = { 0xC4, 0xF3, 0x81, 0x33, 0x02, 0x2B };

STATIC UINTN PatchKernelMcr (UINT8 *P);
STATIC VOID  InstallBeacon (UINT8 *KernelBase);

/* Two 16-bit Thumb NOPs = 4 bytes */
STATIC CONST UINT8 Nop4[] = { 0x00, 0xBF, 0x00, 0xBF };

/* ------------------------------------------------------------------ */
/*  Helper: validate that 4 bytes look like a Thumb-2 B<cond>.W       */
/* ------------------------------------------------------------------ */
STATIC
BOOLEAN
IsThumb2CondBranch (
  CONST UINT8  *P
  )
{
  /*
   * Thumb-2 B<cond>.W encoding (T3):
   *   First halfword:  1111 0 S cond(4) imm6(6)
   *   Second halfword: 10 J1 0 J2 imm11(11)
   *
   * In little-endian memory:
   *   P[1] bits [7:4] = 1111, bits [3:2] = 0 S, bits [1:0] = cond[3:2]
   *   P[3] bit  [7]   = 1, bit [6] = 0, bit [4] = 0
   *
   * We accept any condition code.
   */
  if ((P[1] & 0xF0) != 0xF0)
    return FALSE;
  if ((P[3] & 0xD0) != 0x80)
    return FALSE;
  return TRUE;
}

/* ------------------------------------------------------------------ */
/*  Scan a memory range and apply patches                             */
/* ------------------------------------------------------------------ */

/* Patch the 4-byte B<cond>.W right after Pat at Base+Off, if it is one */
STATIC
BOOLEAN
PatchAt (
  UINT8          *Base,
  UINTN          Off,
  UINTN          Size,
  CONST UINT8    *Pat,
  UINTN          PatLen,
  CONST CHAR8    *Name
  )
{
  UINT8  *Bcc;

  if ((Off + PatLen + 4 > Size) || (CompareMem (Base + Off, Pat, PatLen) != 0)) {
    return FALSE;
  }

  Bcc = Base + Off + PatLen;
  if (!IsThumb2CondBranch (Bcc)) {
    return FALSE;
  }

  CopyMem (Bcc, Nop4, sizeof (Nop4));
  WriteBackInvalidateDataCacheRange (Bcc, sizeof (Nop4));
  InvalidateInstructionCacheRange (Bcc, sizeof (Nop4));
  DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: patched %a at %p\n", Name, Bcc));
  return TRUE;
}

/* The three MMFR checks are back to back (16 bytes apart) in KiSetFeatureBits. */
STATIC
UINTN
PatchRange (
  UINT8  *Base,
  UINTN  Size
  )
{
  UINTN  Count = 0;
  UINTN  i;

  for (i = 0; i + sizeof (PatMmfr2Tlb) + 4 <= Size; i += 2) {
    Count += PatchAt (Base, i, Size, PatCtrL1Ip, sizeof (PatCtrL1Ip), "CTR L1Ip VIVT");

    if (CompareMem (Base + i, PatMmfr2Tlb, sizeof (PatMmfr2Tlb)) != 0) {
      continue;
    }

    /* MMFR2 check, then MMFR3 broadcast and walk, 16 bytes apart */
    if ((i + 48 > Size) ||
        (CompareMem (Base + i + 16, PatMmfr3Bcast, sizeof (PatMmfr3Bcast)) != 0) ||
        (CompareMem (Base + i + 32, PatMmfr3Walk, sizeof (PatMmfr3Walk)) != 0))
    {
      continue;
    }

    Count += PatchAt (Base, i, Size, PatMmfr2Tlb, sizeof (PatMmfr2Tlb), "MMFR2 TLB-maint");
    Count += PatchAt (Base, i + 16, Size, PatMmfr3Bcast, sizeof (PatMmfr3Bcast), "MMFR3 broadcast");
    Count += PatchAt (Base, i + 32, Size, PatMmfr3Walk, sizeof (PatMmfr3Walk), "MMFR3 coherent-walk");


    /* rewrite the UNDEF CP15 ops in the image holding this KiSetFeatureBits */
    Count += PatchKernelMcr (Base + i);
  }

  return Count;
}

/* ------------------------------------------------------------------ */
/*  UNDEF CP15 ops -> local equivalents                               */
/* ------------------------------------------------------------------ */

/*
  Thumb-2 MCR p15, 0, Rt, CRn, CRm, op2:
    hw1 = 0xEE00 | CRn            (op1 = 0, L = 0)
    hw2 = Rt<<12 | 0x0F10 | op2<<5 | CRm
*/
typedef struct {
  UINT16    Hw1;
  UINT16    Hw2;      /* without Rt */
  UINT16    NewHw2;   /* without Rt */
} MCR_FIX;

#define MCR_HW2(op2, crm)  (UINT16)(0x0F10 | ((op2) << 5) | (crm))

STATIC CONST MCR_FIX  mMcrFix[] = {
  { 0xEE08, MCR_HW2 (0, 3), MCR_HW2 (0, 7) },   /* TLBIALLIS  -> TLBIALL  */
  { 0xEE08, MCR_HW2 (1, 3), MCR_HW2 (1, 7) },   /* TLBIMVAIS  -> TLBIMVA  */
  { 0xEE08, MCR_HW2 (2, 3), MCR_HW2 (2, 7) },   /* TLBIASIDIS -> TLBIASID */
  { 0xEE08, MCR_HW2 (3, 3), MCR_HW2 (0, 7) },   /* TLBIMVAAIS -> TLBIALL  */
  { 0xEE08, MCR_HW2 (3, 7), MCR_HW2 (0, 7) },   /* TLBIMVAA   -> TLBIALL  */
  { 0xEE07, MCR_HW2 (0, 1), MCR_HW2 (0, 5) },   /* ICIALLUIS  -> ICIALLU  */
  { 0xEE07, MCR_HW2 (6, 1), MCR_HW2 (6, 5) },   /* BPIALLIS   -> BPIALL   */
};

STATIC
UINTN
RewriteMcr (
  UINT8  *Start,
  UINTN  Size
  )
{
  UINTN   Count = 0;
  UINTN   i;
  UINTN   f;
  UINT16  *H;

  for (i = 0; i + 4 <= Size; i += 2) {
    H = (UINT16 *)(Start + i);
    if ((H[0] & 0xFFF0) != 0xEE00) {
      continue;
    }

    for (f = 0; f < ARRAY_SIZE (mMcrFix); f++) {
      if ((H[0] == mMcrFix[f].Hw1) && ((H[1] & 0x0FFF) == mMcrFix[f].Hw2)) {
        H[1] = (H[1] & 0xF000) | mMcrFix[f].NewHw2;
        Count++;
        i += 2;                       /* skip the 2nd halfword */
        break;
      }
    }
  }

  return Count;
}

/*
  Find the PE image that contains P: walk back page by page to an 'MZ'
  header whose ARM PE covers P, then rewrite its executable sections.
*/
#define MAX_IMAGE_SIZE  0x02000000

STATIC
UINTN
PatchKernelMcr (
  UINT8  *P
  )
{
  UINTN                 Page;
  UINT8                 *Base = NULL;
  UINT32                PeOff;
  UINT16                NumSec, OptSize;
  UINT32                SizeOfImage;
  UINT8                 *Sec;
  UINTN                 s;
  UINTN                 Count = 0;
  UINT32                Va, VSize, Chars;

  for (Page = (UINTN)P & ~(UINTN)0xFFF; (UINTN)P - Page < MAX_IMAGE_SIZE; Page -= 0x1000) {
    UINT8  *M = (UINT8 *)Page;

    if ((M[0] != 'M') || (M[1] != 'Z')) {
      continue;
    }

    PeOff = ReadUnaligned32 ((UINT32 *)(M + 0x3C));
    if ((PeOff > 0x1000) || (ReadUnaligned32 ((UINT32 *)(M + PeOff)) != 0x00004550) ||
        (ReadUnaligned16 ((UINT16 *)(M + PeOff + 4)) != 0x01C4))
    {
      continue;
    }

    SizeOfImage = ReadUnaligned32 ((UINT32 *)(M + PeOff + 24 + 56));
    if ((UINTN)P - Page < SizeOfImage) {
      Base = M;
    }

    break;
  }

  if (Base == NULL) {
    DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: ntoskrnl PE header not found\n"));
    return 0;
  }

  NumSec  = ReadUnaligned16 ((UINT16 *)(Base + PeOff + 6));
  OptSize = ReadUnaligned16 ((UINT16 *)(Base + PeOff + 20));
  Sec     = Base + PeOff + 24 + OptSize;
  for (s = 0; s < NumSec; s++, Sec += 40) {
    VSize = ReadUnaligned32 ((UINT32 *)(Sec + 8));
    Va    = ReadUnaligned32 ((UINT32 *)(Sec + 12));
    Chars = ReadUnaligned32 ((UINT32 *)(Sec + 36));
    if (((Chars & 0x20000000) == 0) || (Va + VSize > SizeOfImage)) {
      continue;                         /* not IMAGE_SCN_MEM_EXECUTE */
    }

    Count += RewriteMcr (Base + Va, VSize);
    WriteBackInvalidateDataCacheRange (Base + Va, VSize);
    InvalidateInstructionCacheRange (Base + Va, VSize);
  }

  DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: ntoskrnl at %p, %u CP15 ops rewritten\n", Base, Count));

  InstallBeacon (Base);
  return Count;
}

/* ------------------------------------------------------------------ */
/*  Progress beacon v2: milestone stripes on the panel from early kernel */
/*  code, no boot graphics and no UART needed (the HD2 has none).        */
/*                                                                       */
/*  BeaconMark (leont/src/beacon/mark.c) borrows 16 PTEs of an unused    */
/*  kernel-image range through the Windows ARM page-table self-map      */
/*  (0xC0000000), points them at framebuffer phys 0x02A00000+id*64K,     */
/*  paints stripe <id> (34 rows, ids 0..22 top to bottom) and restores   */
/*  the PTEs.  A boot-path BL to a known function is retargeted to a     */
/*  stub that paints and then tail-branches to the original function,   */
/*  so the lowest stripe on screen is the last milestone reached.  The   */
/*  bugcheck hook paints stripe 22 and (BEACON_STALL) spins.             */
/* ------------------------------------------------------------------ */

#define BEACON_ENABLE      1
#define BEACON_STALL       1            /* bugcheck: paint then spin      */
#define BEACON_HOOK_RVA    0x001754DC   /* KiBugCheckDebugBreak entry     */
#define BEACON_BUGCHECK_ID 22
#define BEACON_CAVE_RVA    0x001BE23C   /* zero pad at end of .text       */
#define BEACON_BORROW_RVA  0x00440000   /* PAGEVRFY: 16 pages, never run  */

typedef struct {
  UINT32  Site;     /* RVA of a BL                          */
  UINT32  Target;   /* RVA it must call (checked)           */
} BEACON_MILESTONE;

/* Index = stripe id.  Phase 0 boot path, then a few Phase 1 points. */
STATIC CONST BEACON_MILESTONE  mMilestones[] = {
  { 0x0023C060, 0x0023DD04 },   /*  0 KiSystemStartup -> KiInitializeBootStructures */
  { 0x0023DE26, 0x0023E274 },   /*  1   -> KiSetFeatureBits                         */
  { 0x0023C06E, 0x0045F1CC },   /*  2 KiSystemStartup -> KdInitSystem               */
  { 0x0023C0A2, 0x0023E3A4 },   /*  3 KiSystemStartup -> KiInitializeKernel         */
  { 0x0023E51A, 0x004C2F94 },   /*  4 KiInitializeKernel -> KiInitSystem            */
  { 0x0023E5DE, 0x004C9598 },   /*  5 KiInitializeKernel -> InitBootProcessor       */
  { 0x004C95AA, 0x004C52EC },   /*  6 InitBootProcessor -> ExpValidateLoader        */
  { 0x004C9612, 0x004BA170 },   /*  7   -> Whea default config (before HalInitSystem 0) */
  { 0x004C9682, 0x004BAD5C },   /*  8   -> KeInitializeClock (HalInitSystem 0 returned) */
  { 0x004C97A2, 0x004B6278 },   /*  9   -> ExInitSystem                             */
  { 0x004C97BE, 0x004C3DC0 },   /* 10   -> MmInitSystem                             */
  { 0x004C97D2, 0x004BD8B8 },   /* 11   -> InbvDriverInitialize                     */
  { 0x004C998E, 0x004CA914 },   /* 12   -> ObInitSystem                             */
  { 0x004C99A0, 0x004C111C },   /* 13   -> PsInitSystem                             */
  { 0x0023E5E4, 0x0023E864 },   /* 14 KiInitializeKernel -> HalInitSystem(1) wrapper */
  { 0x004C7140, 0x004B6278 },   /* 15 Phase1InitializationDiscard -> ExInitSystem   */
  { 0x004C71E2, 0x004B7EE8 },   /* 16   -> SeInitSystem                             */
  { 0x004C6E76, 0x004BD8B8 },   /* 17   -> InbvDriverInitialize                     */
};

/*
  BeaconMark(id, borrowVA): compiled from leont/src/beacon/mark.c with
  clang --target=armv7a-none-eabi -mthumb -Os -ffreestanding -fno-builtin
  -fno-jump-tables; verified: no relocations, no calls, constants only.
*/
STATIC CONST UINT8  mBeaconMark[] = {
  0x2d,0xe9,0xf0,0x43,0x90,0xb0,0xef,0xf3,0x00,0x8c,0x72,0xb6,0x6a,0x46,0x92,0x0a,
  0x02,0xf1,0x40,0x42,0x22,0xf0,0x03,0x02,0x12,0x68,0x93,0x07,0x40,0xf1,0xa3,0x80,
  0x6f,0xf3,0x1f,0x32,0x16,0x28,0x28,0xbf,0x16,0x20,0x42,0xea,0x00,0x42,0x4f,0xf6,
  0xfc,0x78,0xee,0x46,0x42,0xf0,0x28,0x79,0xc0,0xf2,0x3f,0x08,0x00,0x24,0x4f,0xf0,
  0x40,0x45,0x76,0x46,0x0f,0x19,0x09,0xeb,0x04,0x02,0x04,0xf5,0x80,0x54,0x08,0xea,
  0x97,0x27,0xb4,0xf5,0x80,0x3f,0x7b,0x59,0x7a,0x51,0x07,0xf1,0x40,0x42,0x46,0xf8,
  0x04,0x3b,0x07,0xee,0x3a,0x2f,0xed,0xd1,0x00,0x22,0xbf,0xf3,0x4f,0x8f,0x8b,0x18,
  0x02,0xf5,0x80,0x52,0xb2,0xf5,0x80,0x3f,0x08,0xee,0x37,0x3f,0xf7,0xd1,0x25,0x22,
  0x01,0x24,0x42,0x43,0x40,0xf2,0x00,0x09,0x48,0xf6,0x89,0x06,0xcf,0xf6,0xff,0x79,
  0xc8,0xf6,0x88,0x06,0xa0,0xeb,0x12,0x23,0xdb,0xb2,0x5b,0x08,0x03,0xeb,0x12,0x22,
  0xd2,0xb2,0x92,0x08,0xa2,0xeb,0xc2,0x02,0x83,0x18,0x80,0x03,0x07,0x46,0xda,0x07,
  0x54,0xfa,0x83,0xf3,0x4f,0xf0,0x7f,0x42,0x04,0xbf,0x00,0x22,0xcf,0xf6,0xff,0x72,
  0x9c,0x07,0x48,0xbf,0x02,0xf5,0x7f,0x42,0x5b,0x07,0x48,0xbf,0xff,0x32,0xbf,0xf3,
  0x4f,0x8f,0x00,0x24,0xbf,0xf3,0x6f,0x8f,0xa7,0xfb,0x06,0x35,0x2b,0x0a,0x1b,0x01,
  0xa3,0xeb,0x15,0x23,0xa0,0xeb,0x43,0x13,0x23,0x44,0x3f,0x2b,0x07,0xd8,0x13,0x46,
  0xb4,0xf5,0xf0,0x7f,0x38,0xbf,0x4f,0xf0,0x7f,0x43,0x41,0xf8,0x24,0x30,0x01,0x34,
  0x01,0x37,0xb4,0xf5,0x80,0x4f,0xe7,0xd1,0x49,0x45,0x07,0xd2,0x01,0xf5,0x80,0x30,
  0x0a,0x46,0x07,0xee,0x3a,0x2f,0x20,0x32,0x82,0x42,0xfa,0xd3,0x6f,0xf0,0x03,0x00,
  0x00,0x22,0x00,0xea,0x91,0x20,0x4f,0xf0,0x40,0x44,0xbf,0xf3,0x4f,0x8f,0x85,0x18,
  0x5e,0xf8,0x02,0x30,0x05,0xea,0x08,0x05,0x04,0x32,0x2b,0x51,0x05,0xf1,0x40,0x43,
  0x40,0x2a,0x07,0xee,0x3a,0x3f,0xf2,0xd1,0x00,0x20,0xbf,0xf3,0x4f,0x8f,0x0a,0x18,
  0x00,0xf5,0x80,0x50,0xb0,0xf5,0x80,0x3f,0x08,0xee,0x37,0x2f,0xf7,0xd1,0xbf,0xf3,
  0x4f,0x8f,0xbf,0xf3,0x6f,0x8f,0x5f,0xea,0x0c,0x60,0x00,0xd4,0x62,0xb6,0x10,0xb0,
  0xbd,0xe8,0xf0,0x83

};

/* Encode a Thumb-2 32-bit branch (BL if Link, else B.W) at At -> Target. */
STATIC
VOID
ThumbBranch (
  UINT8    *At,
  UINT8    *Target,
  BOOLEAN  Link
  )
{
  INT32   Off = (INT32)((INTN)Target - ((INTN)At + 4));
  UINT32  S     = ((UINT32)Off >> 24) & 1;
  UINT32  I1    = ((UINT32)Off >> 23) & 1;
  UINT32  I2    = ((UINT32)Off >> 22) & 1;
  UINT32  Imm10 = ((UINT32)Off >> 12) & 0x3FF;
  UINT32  Imm11 = ((UINT32)Off >> 1) & 0x7FF;
  UINT32  J1    = ((~I1) & 1) ^ S;
  UINT32  J2    = ((~I2) & 1) ^ S;
  UINT16  Hw1   = (UINT16)(0xF000 | (S << 10) | Imm10);
  UINT16  Hw2   = (UINT16)((Link ? 0xD000 : 0x9000) | (J1 << 13) | (J2 << 11) | Imm11);

  At[0] = (UINT8)Hw1; At[1] = (UINT8)(Hw1 >> 8);
  At[2] = (UINT8)Hw2; At[3] = (UINT8)(Hw2 >> 8);
}

/* Encode Thumb-2 MOVW (Top=FALSE) / MOVT (Top=TRUE) Rd, #Imm16 at At. */
STATIC
VOID
ThumbMovwt (
  UINT8    *At,
  UINT32   Rd,
  UINT16   Imm,
  BOOLEAN  Top
  )
{
  UINT16  Hw1 = (UINT16)((Top ? 0xF2C0 : 0xF240) | (((Imm >> 11) & 1) << 10) | ((Imm >> 12) & 0xF));
  UINT16  Hw2 = (UINT16)((((Imm >> 8) & 7) << 12) | (Rd << 8) | (Imm & 0xFF));

  At[0] = (UINT8)Hw1; At[1] = (UINT8)(Hw1 >> 8);
  At[2] = (UINT8)Hw2; At[3] = (UINT8)(Hw2 >> 8);
}

/* Target of the Thumb-2 BL at At, or NULL if At does not hold a BL. */
STATIC
UINT8 *
ThumbBlTarget (
  UINT8  *At
  )
{
  UINT16  Hw1 = (UINT16)(At[0] | (At[1] << 8));
  UINT16  Hw2 = (UINT16)(At[2] | (At[3] << 8));
  UINT32  S, J1, J2, I1, I2, Imm;
  INT32   Off;

  if ((Hw1 & 0xF800) != 0xF000 || (Hw2 & 0xD000) != 0xD000) {
    return NULL;
  }
  S  = (Hw1 >> 10) & 1;
  J1 = (Hw2 >> 13) & 1;
  J2 = (Hw2 >> 11) & 1;
  I1 = (~(J1 ^ S)) & 1;
  I2 = (~(J2 ^ S)) & 1;
  Imm = (S << 24) | (I1 << 23) | (I2 << 22) | ((UINT32)(Hw1 & 0x3FF) << 12) | ((UINT32)(Hw2 & 0x7FF) << 1);
  Off = (INT32)(Imm << 7) >> 7;
  return At + 4 + Off;
}

/* movs r0,#Id ; movw/movt r1,#delta ; add r1,pc  (r1 = borrow VA) at At, 12 bytes */
STATIC
VOID
BeaconArgs (
  UINT8  *At,
  UINT8  *KernelBase,
  UINT32 Id
  )
{
  UINT32  Delta = (UINT32)((INTN)(KernelBase + BEACON_BORROW_RVA) - ((INTN)At + 10 + 4));

  ((UINT16 *)At)[0] = (UINT16)(0x2000 | (Id & 0xFF));
  ThumbMovwt (At + 2, 1, (UINT16)Delta, FALSE);
  ThumbMovwt (At + 6, 1, (UINT16)(Delta >> 16), TRUE);
  ((UINT16 *)At)[5] = 0x4479;                       /* add r1, pc */
}

STATIC
VOID
InstallBeacon (
  UINT8  *KernelBase
  )
{
 #if BEACON_ENABLE
  UINT8   *Cave  = KernelBase + BEACON_CAVE_RVA;
  UINT8   *Mark  = Cave;
  UINT8   *Tramp = Cave + ((sizeof (mBeaconMark) + 3) & ~3U);
  UINT8   *Stub  = Tramp + 20;
  UINT8   *Hook  = KernelBase + BEACON_HOOK_RVA;
  UINTN   Hooked = 0;
  UINTN   Index;

  CopyMem (Mark, mBeaconMark, sizeof (mBeaconMark));

  /*
    bugcheck trampoline (B.W from KiBugCheckDebugBreak):
      +0  movs/movw/movt/add  r0 = 22, r1 = borrow VA
      +12 bl   BeaconMark
      +16 b    .                 (BEACON_STALL)
    All offsets are image-relative, so the same bytes are right at the
    physical load address we patch and at the kernel VA it runs from.
  */
  BeaconArgs (Tramp, KernelBase, BEACON_BUGCHECK_ID);
  ThumbBranch (Tramp + 12, Mark, TRUE);
 #if BEACON_STALL
  ((UINT16 *)Tramp)[8] = 0xE7FE;                    /* b . */
 #endif
  ThumbBranch (Hook, Tramp, FALSE);
  WriteBackInvalidateDataCacheRange (Hook, 4);
  InvalidateInstructionCacheRange (Hook, 4);

  /*
    milestone stub (28 bytes), the hooked BL now calls it:
      +0  push.w {r0-r3, r12, lr}
      +4  movs/movw/movt/add  r0 = id, r1 = borrow VA
      +16 bl   BeaconMark
      +20 pop.w  {r0-r3, r12, lr}
      +24 b.w  original target       (returns straight to the hooked caller)
  */
  for (Index = 0; Index < ARRAY_SIZE (mMilestones); Index++, Stub += 28) {
    UINT8  *Site   = KernelBase + mMilestones[Index].Site;
    UINT8  *Target = KernelBase + mMilestones[Index].Target;

    if (ThumbBlTarget (Site) != Target) {
      DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: milestone %u: no BL to %x at %x, skipped\n",
              Index, mMilestones[Index].Target, mMilestones[Index].Site));
      continue;
    }
    ((UINT16 *)Stub)[0] = 0xE92D;  ((UINT16 *)Stub)[1] = 0x500F;   /* push.w {r0-r3,r12,lr} */
    BeaconArgs (Stub + 4, KernelBase, (UINT32)Index);
    ThumbBranch (Stub + 16, Mark, TRUE);
    ((UINT16 *)Stub)[10] = 0xE8BD; ((UINT16 *)Stub)[11] = 0x500F;  /* pop.w {r0-r3,r12,lr} */
    ThumbBranch (Stub + 24, Target, FALSE);
    ThumbBranch (Site, Stub, TRUE);
    WriteBackInvalidateDataCacheRange (Site, 4);
    InvalidateInstructionCacheRange (Site, 4);
    Hooked++;
  }

  WriteBackInvalidateDataCacheRange (Cave, (UINTN)(Stub - Cave));
  InvalidateInstructionCacheRange (Cave, (UINTN)(Stub - Cave));

  DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: beacon v2: %u/%u milestones, bugcheck hook %x, cave %p (%u bytes)\n",
          Hooked, ARRAY_SIZE (mMilestones), BEACON_HOOK_RVA, Cave, (UINT32)(Stub - Cave)));
 #endif
}

/* ------------------------------------------------------------------ */
/*  ExitBootServices hook                                             */
/* ------------------------------------------------------------------ */

STATIC EFI_EXIT_BOOT_SERVICES  gOrigExitBootServices;
STATIC BOOLEAN                 gPatchApplied = FALSE;

/*
  Static so the hook never allocates (that would invalidate winload's MapKey).
  UINT64[] for alignment: as UINT8[] it landed on an odd address and
  CoreGetMemoryMap's strd into the first descriptor alignment-faulted.
*/
STATIC UINT64  gMemMap[0x4000 / sizeof (UINT64)];

STATIC
VOID
RestoreExitBootServices (
  VOID
  )
{
  UINT32  Crc = 0;

  gBS->ExitBootServices = gOrigExitBootServices;
  gBS->Hdr.CRC32        = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &Crc);
  gBS->Hdr.CRC32 = Crc;
}

STATIC
EFI_STATUS
EFIAPI
ExitBootServicesHook (
  IN EFI_HANDLE  ImageHandle,
  IN UINTN       MapKey
  )
{
  EFI_STATUS             Status;
  UINTN                  MemMapSize;
  EFI_MEMORY_DESCRIPTOR  *Desc;
  UINTN                  DescSize;
  UINT32                 DescVer;
  UINTN                  Key;
  UINTN                  Entries;
  UINTN                  e;
  UINTN                  TotalPatched = 0;

  if (!gPatchApplied) {
    gPatchApplied = TRUE;
    MemMapSize    = sizeof (gMemMap);
    Status        = gBS->GetMemoryMap (&MemMapSize, (EFI_MEMORY_DESCRIPTOR *)gMemMap, &Key, &DescSize, &DescVer);
    if (!EFI_ERROR (Status)) {
      Desc    = (EFI_MEMORY_DESCRIPTOR *)gMemMap;
      Entries = MemMapSize / DescSize;
      for (e = 0; e < Entries; e++) {
        if (((Desc->Type == EfiLoaderData) || (Desc->Type == EfiLoaderCode)) &&
            (Desc->NumberOfPages > 1))
        {
          TotalPatched += PatchRange (
                            (UINT8 *)(UINTN)Desc->PhysicalStart,
                            EFI_PAGES_TO_SIZE ((UINTN)Desc->NumberOfPages)
                            );
        }

        Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescSize);
      }
    } else {
      DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: GetMemoryMap %r size %u\n", Status, MemMapSize));
    }

    DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: %u patches applied\n", TotalPatched));
  }

  RestoreExitBootServices ();
  return gOrigExitBootServices (ImageHandle, MapKey);
}

/* ------------------------------------------------------------------ */
/*  Driver entry point                                                */
/* ------------------------------------------------------------------ */

EFI_STATUS
EFIAPI
KernelErrataPatcherDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINT32  Crc = 0;

  DEBUG ((DEBUG_ERROR, "KernelErrataPatcher: loaded, hooking ExitBootServices\n"));

  gOrigExitBootServices = gBS->ExitBootServices;
  gBS->ExitBootServices = ExitBootServicesHook;
  gBS->Hdr.CRC32        = 0;
  gBS->CalculateCrc32 (gBS, gBS->Hdr.HeaderSize, &Crc);
  gBS->Hdr.CRC32 = Crc;

  return EFI_SUCCESS;
}
