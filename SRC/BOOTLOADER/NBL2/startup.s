;------------------------------------------------------------------------------
;
;  Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
;  Use of this source code is subject to the terms of the Microsoft end-user
;  license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
;  If you did not accept the terms of the EULA, you are not authorized to use
;  this source code. For a copy of the EULA, please see the LICENSE.RTF on your
;  install media.
;
;------------------------------------------------------------------------------
;
;   File:  startup.s
;
;   Hardware startup routine for Samsung SMDK6410 board.
;
;------------------------------------------------------------------------------

		INCLUDE kxarm.h
		INCLUDE armmacros.s
		INCLUDE s3c6410.inc

;-------------------------------------------------------------------------------

MemoryMap       EQU     0x2a4
BANK_SIZE       EQU     0x00100000      ; 1MB per bank in MemoryMap array
BANK_SHIFT      EQU     20


;   Define RAM space for the Page Tables:
;
PHYBASE	 EQU     0x50000000      ; physical start
PTs	     EQU     0x50010000      ; 1st level page table address (PHYBASE + 0x10000)
					; save room for interrupt vectors.

;-------------------------------------------------------------------------------

	TEXTAREA

	IMPORT  main

; Set up the MMU and Dcache for bootloader.
;
; This routine will initialize the first-level page table based up the contents
; of the MemoryMap array and enable the MMU and caches.
;
; Copy the image to RAM if it's not already running there.
;
; Include Files 

;---------------------------------------------------------------------------
;	4 LED light function
;	The LEDs are located below AMD Flash ROM

;---------------------------------------------------------------------------


;-------------------------------------------------------------------------------
;   Function: Startup
;
;   Main entry point for CPU initialization.
;

	STARTUPTEXT
	LEAF_ENTRY      StartUp
    
	; Shouldn't get here.
	b       %F20

	INCLUDE oemaddrtab_cfg.inc
 

	; Compute physical address of the OEMAddressTable.
20      add     r11, pc, #g_oalAddressTable - (. + 8)
	ldr     r10, =PTs		; (r10) = 1st level page table


	; Setup 1st level page table (using section descriptor)     
	; Fill in first level page table entries to create "un-mapped" regions
	; from the contents of the MemoryMap array.
	;
	;   (r10) = 1st level page table
	;   (r11) = ptr to MemoryMap array

	add     r10, r10, #0x2000       ; (r10) = ptr to 1st PTE for "unmapped space"
	mov     r0, #0x0E	       ; (r0) = PTE for 0: 1MB cachable bufferable
	orr     r0, r0, #0x400	  ; set kernel r/w permission
25      mov     r1, r11		 ; (r1) = ptr to MemoryMap array

	
30      ldr     r2, [r1], #4	    ; (r2) = virtual address to map Bank at
	ldr     r3, [r1], #4	    ; (r3) = physical address to map from
	ldr     r4, [r1], #4	    ; (r4) = num MB to map

	cmp     r4, #0		  ; End of table?
	beq     %f40

	ldr     r5, =0x1FF00000
	and     r2, r2, r5	      ; VA needs 512MB, 1MB aligned.		

	ldr     r5, =0xFFF00000
	and     r3, r3, r5	      ; PA needs 4GB, 1MB aligned.

	add     r2, r10, r2, LSR #18
	add     r0, r0, r3	      ; (r0) = PTE for next physical page

35      str     r0, [r2], #4
	add     r0, r0, #0x00100000     ; (r0) = PTE for next physical page
	sub     r4, r4, #1	      ; Decrement number of MB left 
	cmp     r4, #0
	bne     %b35		    ; Map next MB

	bic     r0, r0, #0xF0000000     ; Clear Section Base Address Field
	bic     r0, r0, #0x0FF00000     ; Clear Section Base Address Field
	b       %b30		    ; Get next element
	
40      tst     r0, #8
	bic     r0, r0, #0x0C	   ; clear cachable & bufferable bits in PTE
	add     r10, r10, #0x0800       ; (r10) = ptr to 1st PTE for "unmapped uncached space"
	bne     %b25		    ; go setup PTEs for uncached space
	sub     r10, r10, #0x3000       ; (r10) = restore address of 1st level page table

	; Setup mmu to map (VA == 0) to (PA == 0x50000000).
	ldr     r0, =PTs		; PTE entry for VA = 0
	ldr     r1, =0x5000040E	 ; uncache/unbuffer/rw, PA base == 0x50000000
	str     r1, [r0]

	; uncached area.
	add     r0, r0, #0x0800	 ; PTE entry for VA = 0x0200.0000 , uncached     
	ldr     r1, =0x50000402	 ; uncache/unbuffer/rw, base == 0x50000000
	str     r1, [r0]
	
	; Comment:
	; The following loop is to direct map RAM VA == PA. i.e. 
	;   VA == 0x50XXXXXX => PA == 0x50XXXXXX for S3C6410
	; Fill in 8 entries to have a direct mapping for DRAM
	;
	ldr     r10, =PTs	       ; restore address of 1st level page table
	ldr     r0,  =PHYBASE

	add     r10, r10, #(0x5000 / 4) ; (r10) = ptr to 1st PTE for 0x50000000

	add     r0, r0, #0x1E	   ; 1MB cachable bufferable
	orr     r0, r0, #0x400	  ; set kernel r/w permission
	mov     r1, #0 
	mov     r3, #64
45      mov     r2, r1		  ; (r2) = virtual address to map Bank at
	cmp     r2, #0x20000000:SHR:BANK_SHIFT
	add     r2, r10, r2, LSL #BANK_SHIFT-18
	strlo   r0, [r2]
	add     r0, r0, #0x00100000     ; (r0) = PTE for next physical page
	subs    r3, r3, #1
	add     r1, r1, #1
	bgt     %b45

	ldr     r10, =PTs	       ; (r10) = restore address of 1st level page table

	; The page tables and exception vectors are setup.
	; Initialize the MMU and turn it on.
	mov     r1, #1
	mcr     p15, 0, r1, c3, c0, 0   ; setup access to domain 0
	mcr     p15, 0, r10, c2, c0, 0

	mcr		p15, 0, r0, c8, c7, 0	; flush I+D TLBs

	mrc		p15, 0, r1, c1, c0, 0
	orr		r1, r1, #0x0071			; Enable: MMU
	orr		r1, r1, #0x0004		; Enable the cache

	ldr     r0, =VirtualStart

	cmp     r0, #0		  ; make sure no stall on "mov pc,r0" below
	mcr     p15, 0, r1, c1, c0, 0
	mov     pc, r0		  ;  & jump to new virtual address
	nop

	; MMU & caches now enabled.
	;   (r10) = physcial address of 1st level page table
	;

VirtualStart

;	mov     sp, #0x80000000	; have to be modefied. refer oemaddrtab_cfg.inc, DonGo
;	add     sp, sp, #0x30000	; arbitrary initial super-page stack pointer
;	mov     sp, #0x80000000	; have to be modefied. refer oemaddrtab_cfg.inc, DonGo
;	add     sp, sp, #0x1200000	; arbitrary initial super-page stack pointer
;	add     sp, sp, #0x42000	; arbitrary initial super-page stack pointer
	ldr		sp, =0x83000000

	b       main

	END

