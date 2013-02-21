;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
;
; Use of this source code is subject to the terms of the Microsoft end-user
; license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
; If you did not accept the terms of the EULA, you are not authorized to use
; this source code. For a copy of the EULA, please see the LICENSE.RTF on your
; install media.
;
;******************************************************************************
;*
;* System On Chip(SOC)
;*
;* Copyright (c) 2002 Software Center, Samsung Electronics, Inc.
;* All rights reserved.
;*
;* This software is the confidential and proprietary information of Samsung
;* Electronics, Inc("Confidential Information"). You Shall not disclose such
;* Confidential Information and shall use it only in accordance with the terms
;* of the license agreement you entered into Samsung.
;*
;******************************************************************************

    INCLUDE kxarm.h

PHY_RAM_START	EQU	0x50000000
VIR_RAM_START	EQU	0x80000000

	TEXTAREA

	LEAF_ENTRY Launch

	ldr	r2, = PhysicalStart
	ldr     r3, = (VIR_RAM_START - PHY_RAM_START)

	sub     r2, r2, r3


		mov		r1, #0
		mcr		p15, 0, r1, c7, c5, 0	; Invalidate Entire Instruction Cache
		mcr		p15, 0, r1, c7, c14, 0	; Clean and Invalidate Entire Data Cache

		mrc		p15, 0, r1, c1, c0, 0
		bic		r1, r1, #0x0005		; Disable MMU and Data Cache
		mcr		p15, 0, r1, c1, c0, 0

	nop
	mov     pc, r2                  ; Jump to PStart
	nop

	; MMU & caches now disabled.

PhysicalStart

	mov     r2, #0
	mcr     p15, 0, r2, c8, c7, 0   ; Flush the TLB
	mov     pc, r0			; Jump to program we are launching.


	;++
	;  Routine:
	;
	;      ShowLights
	;
	;  Description:
	;
	;      Set the Hexadecimal LED array to the values specified
	;
	;  Arguments:
	;
	;      r0 = word containing 8 nibble values to write to the Hexadecimal LED
	;
	;
	;--

	LEAF_ENTRY ShowLights

	mov	pc, lr

	END
