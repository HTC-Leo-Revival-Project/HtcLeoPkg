/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#define DSB .byte 0x4F, 0xF0, 0x7F, 0xF5
#define ISB .byte 0x6F, 0xF0, 0x7F, 0xF5
#define WSPL_VADDR 0x80000000

.text
.globl _start
_start:
	B		_reset

//if EDK2 is loaded by wince spl add romhdr
.org 0x40
	.word	0x43454345
	.word	(romhdr-_start)+WSPL_VADDR	// virtual address of romhdr
	.word	romhdr-_start				// file address of romhdr

.org 0x00000900
romhdr:
	.word	0x02000000					// dllfirst
	.word	0x02000000					// dlllast
	.word	WSPL_VADDR					// physfirst
	.word	WSPL_VADDR+0x000F0000		// physlast (using hardoded FV size, fix later?)
	.word	0							// nummods (no TOCentry after ROMHDR)
	.word	WSPL_VADDR+0x000F0000		// ulRAMStart
	.word	WSPL_VADDR+0x000F0000		// ulRAMFree
	.word	WSPL_VADDR+0x001F0000		// ulRAMEnd
	.word	0							// ulCopyEntries
	.word	0							// ulCopyOffset
	.word	0							// ulProfileLen
	.word	0							// ulProfileOffset
	.word	0							// numfiles
	.word	0							// ulKernelFlags
	.word	0x80808080					// ulFSRamPercent
	.word	0							// ulDrivglobStart
	.word	0							// ulDrivglobLen
	.hword	0x01C2						// usCPUType
	.hword	0x0002						// usMiscFlags
	.word	0							// pExtensions
	.word	0							// ulTrackingStart
	.word	0							// ulTrackingLen
.org 0x00001000

_reset:
	/* do some cpu setup */
	// From PrimeG2Pkg
	/*
				3322 2222 2222 1111 1111 1100 0000 0000
				1098 7654 3210 9876 5432 1098 7654 3210
	Clear bits	---- ---- ---- ---- ?-V- --RS ---- -CAM
	Set bits	---- ---- ---- ---- ---I ---- ---- ----
	*/
	MRC		P15, 0, R0, CR1, CR0, 0		// read SCTLR
	BIC		R0, R0, #0x0000A300
	BIC		R0, R0, #0x00000007
	ORR		R0, R0, #0x00001000
	MCR		P15, 0, R0, CR1, CR0, 0		// write SCTLR

	// Martijn Stolk's code so kernel will not crash. aux control register
	MRC		P15, 0, R0, CR1, CR0, 1		// read ACTLR
	BIC		R0, R0, #0x40
	BIC		R0, R0, #0x200000
	MCR		P15, 0, R0, CR1, CR0, 1		// write ACTLR

	// Disable VFP
	MOV		R0, #0
	FMXR	FPEXC, R0
	
	// Invalidate the UTLB
	MOV		R0, #0
	MCR		P15, 0, R0, CR8, CR7, 0

	// Clean and invalidate cache - Ensure pipeline flush
	MOV		R0, #0
	DSB
	ISB

	/* see if we need to relocate */
	ADR		R0, _payload 	// load the payload adress into r0

	LDR		R1, uefibase 	// load the expected payload base into r1
	CMP		R0, R1		 	// compare the adresses
	BEQ		_entry			// jump to entry if the adress is the same
	LDR		R2, uefisize

_copyloop:
	/* memcpy */
	LDR		R3, [R0], #4
	STR		R3, [R1], #4
	SUBS	R2, R2, #4	// substract the size that got copied from fd size
	BGT		_copyloop		// branch if greater than zero

	/* we're relocated, jump to the right address */

_entry:
	LDR		R5, uefibase
	BX		R5

uefibase:
	.word	UEFI_BASE

uefisize:
	.word	UEFI_SIZE

.ltorg

.text
.align 4
_payload: