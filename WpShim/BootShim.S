#define DSB .byte 0x4f, 0xf0, 0x7f, 0xf5
#define ISB .byte 0x6f, 0xf0, 0x7f, 0xf5
#define WSPL_VADDR 0x80500000

.text
.globl _start
_start:
        b  _Payload
	b  paint_fb_red
	b   _relocate

//if EDK2 is loaded by wince spl add romhdr
.org 0x40
	.word 0x43454345
	.word (romhdr-_start)+WSPL_VADDR  // virtual address of romhdr
	.word romhdr-_start               // file address of romhdr

.org 0x00000900
romhdr:
	.word 0x2000000             // dllfirst
	.word 0x2000000             // dlllast
	.word WSPL_VADDR            // physfirst
	.word WSPL_VADDR+0x000F0000 // physlast (using hardoded FV size, fix later?)
	.word 0                     // nummods (no TOCentry after ROMHDR)
	.word WSPL_VADDR+0x000F0000 // ulRAMStart
	.word WSPL_VADDR+0x000F0000 // ulRAMFree
	.word WSPL_VADDR+0x001F0000 // ulRAMEnd
	.word 0                     // ulCopyEntries
	.word 0                     // ulCopyOffset
	.word 0                     // ulProfileLen
	.word 0                     // ulProfileOffset
	.word 0                     // numfiles
	.word 0                     // ulKernelFlags
	.word 0x80808080            // ulFSRamPercent
	.word 0                     // ulDrivglobStart
	.word 0                     // ulDrivglobLen
	.hword 0x1C2                // usCPUType
	.hword 0x2                  // usMiscFlags
	.word 0                     // pExtensions
	.word 0                     // ulTrackingStart
	.word 0                     // ulTrackingLen
.org 0x00001000

.code 32
.text
find_fb:
	ldr r0, =0xAA290008     @ Load the memory address to read from into R0
   	ldr r1, [R0]            @ Load the value at the memory address into R1

paint_fb_red:
	ldr r2, =0xf800         @ Load the fill value into R2

paint_fb:
	ldr r1, FbBase
   	ldr r3, FbSize     	@ Load the fill length into R3

fill_loop:
   	str r2, [r1], #4        @ Store the fill value to memory address pointed by R1, and increment R1 by 4 (assuming each memory location is 4 bytes)
   	subs r3, r3, #4         @ Subtract 4 from R3 to track the remaining fill length
   	bne fill_loop           @ Branch back to fill_loop if R3 is not zero

FbBase:
	.word 0x2E744000

FbSize:
	.word 0x00C00000

.ltorg

.code 32
.text
uefi_setup:
	// From PrimeG2Pkg
	// Disable Strict alignment checking & Enable Instruction cache
  	mrc     p15, 0, r0, c1, c0, 0
  	bic     r0, r0, #0x00002300     /* clear bits 13, 9:8 (--V- --RS) */
  	bic     r0, r0, #0x00000005     /* clear bits 0, 2 (---- -C-M) */
  	bic     r0, r0, #0x00000002     /* Clear bit 1 (Alignment faults) */
  	orr     r0, r0, #0x00001000     /* set bit 12 (I) enable I-Cache */
  	mcr     p15, 0, r0, c1, c0, 0

_relocate:
	/* see if we need to relocate */
	adr	r0, _Payload 	// load the payload adress into r0

	ldr	r1, UefiBase 	// load the expected payload base into r1
	cmp	r0, r1		 	// compare the adresses
	beq	_Entry			// jump to entry if the adress is the same
	ldr	r2, UefiSize

_CopyLoop:
	/* memcpy */
	ldr	r3, [r0], #4
	str	r3, [r1], #4
	subs	r2, r2, #4	// substract the size that got copied from fd size
	bgt	_CopyLoop		// branch if greater than zero

	/* we're relocated, jump to the right address */

_Entry:
	/* about to jump, paint screen green */
	ldr r2, =0x07e0
	b       paint_fb
	ldr		r5, UefiBase
	bx		r5

UefiBase:
	.word UEFI_BASE

UefiSize:
	.word UEFI_SIZE

.ltorg

.text
.align 4
_Payload:
