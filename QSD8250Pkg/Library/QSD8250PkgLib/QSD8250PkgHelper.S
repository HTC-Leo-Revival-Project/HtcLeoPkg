#
#  Copyright (c) 2018, Linaro Limited. All rights reserved.
#
#  This program and the accompanying materials
#  are licensed and made available under the terms and conditions of the BSD License
#  which accompanies this distribution.  The full text of the license may be found at
#  http://opensource.org/licenses/bsd-license.php
#
#  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
#  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
#
#

#include <AsmMacroIoLib.h>
#include <Library/ArmLib.h>

ASM_FUNC(ArmPlatformPeiBootAction)
  ldr   r2, =0xF81F         // Load the fill value into R2 (RGB565_MAGENTA)
  ldr   r1, =0x2E744000     // Load fb base into R1
  ldr   r3, =0x00266666     // Load the fill length into R3 (1/4 of the screen)

fill_loop:
  str   r2, [r1], #4       // Store the fill value to memory address pointed by R1, and increment R1 by 4 (assuming each memory location is 4 bytes)
  subs  r3, r3, #4         // Subtract 4 from R3 to track the remaining fill length
  bne   fill_loop          // Branch back to fill_loop if R3 is not zero
  bx    lr

//UINTN
//ArmPlatformGetCorePosition (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformGetCorePosition)
  and   r1, r0, #ARM_CORE_MASK
  and   r0, r0, #ARM_CLUSTER_MASK
  add   r0, r1, r0, LSR #7
  bx    lr

//UINTN
//ArmPlatformGetPrimaryCoreMpId (
//  VOID
//  );
ASM_FUNC(ArmPlatformGetPrimaryCoreMpId)
  MOV32  (r0, FixedPcdGet32 (PcdArmPrimaryCore))
  bx    lr

//UINTN
//ArmPlatformIsPrimaryCore (
//  IN UINTN MpId
//  );
ASM_FUNC(ArmPlatformIsPrimaryCore)
  MOV32  (r1, FixedPcdGet32 (PcdArmPrimaryCoreMask))
  and   r0, r0, r1
  MOV32  (r1, FixedPcdGet32 (PcdArmPrimaryCore))
  cmp   r0, r1
  moveq r0, #1
  movne r0, #0
  bx    lr
