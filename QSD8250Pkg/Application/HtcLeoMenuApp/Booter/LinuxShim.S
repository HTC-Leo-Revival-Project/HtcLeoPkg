#include <AsmMacroIoLib.h>
#include "LinuxShim.h"

ASM_FUNC(PrepareForLinux)    
    /* do some cpu setup */
	mrc		p15, 0, r0, c1, c0, 0
	bic		r0, r0, #(1<<15| 1<<13 | 1<<12)
	bic		r0, r0, #(1<<2 | 1<<0)
	orr		r0, r0, #(1<<1)
	mcr		p15, 0, r0, c1, c0, 0

	// Martijn Stolk's code so kernel will not crash. aux control register
	MRC p15, 0, r0, c1, c0, 1
	BIC r0, r0, #0x40
	BIC r0, r0, #0x200000
	MCR p15, 0, r0, c1, c0, 1

	MOV R0, #0

    // disable mmu
	MRC p15, 0, r0, c1, c0, 0
	BIC r0, r0, #(1<<0)
	MCR p15, 0, r0, c1, c0, 0
	ISB
	
	// Invalidate the UTLB
	MOV r0, #0
	MCR p15, 0, r0, c8, c7, 0

	// Clean and invalidate cache - Ensure pipeline flush
	MOV R0, #0
	DSB
	ISB

	// From PrimeG2Pkg
	// Disable Strict alignment checking & Enable Instruction cache
  	bic     r0, r0, #0x00002300     /* clear bits 13, 9:8 (--V- --RS) */
  	bic     r0, r0, #0x00000005     /* clear bits 0, 2 (---- -C-M) */
  	bic     r0, r0, #0x00000002     /* Clear bit 1 (Alignment faults) */
  	orr     r0, r0, #0x00001000     /* set bit 12 (I) enable I-Cache */
	mcr		p15, 0, r0, c1, c0, 0