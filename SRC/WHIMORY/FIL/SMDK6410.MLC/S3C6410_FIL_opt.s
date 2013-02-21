;///////////////////////////////////////////////////////////////
;//
;//	MODULE		: FIL
;//	NAME		: S3C6410X Flash Interface Layer
;//	FILE			: S3C6410X_NAND.s
;//	PURPOSE		: Assembly Optimized NF Data register access code
;//
;///////////////////////////////////////////////////////////////
;//
;//		COPYRIGHT 2003-2006 SAMSUNG ELECTRONICS CO., LTD.
;//					ALL RIGHTS RESERVED
;//
;//	Permission is hereby granted to licensees of Samsung Electronics
;//	Co., Ltd. products to use or abstract this computer program for the
;//	sole purpose of implementing a product based on Samsung
;//	Electronics Co., Ltd. products. No other rights to reproduce, use,
;//	or disseminate this computer program, whether in part or in whole,
;//	are granted.
;//
;//	Samsung Electronics Co., Ltd. makes no representation or warranties
;//	with respect to the performance of this computer program, and
;//	specifically disclaims any responsibility for any damages,
;//	special or consequential, connected with the use of this program.
;//
;///////////////////////////////////////////////////////////////
;//
;//	REVISION HISTORY
;//
;//	2006.10.19	dodan2(gabjoo.lim@samsung.com)
;//				Draft Version
;//
;///////////////////////////////////////////////////////////////

		INCLUDE kxarm.h
		TEXTAREA

;/////////////////////////////////////////////////////
;//
;//	void _Read_512Byte(unsigned char *pBuf)
;//
;//	Read 512 bytes (1 Sector) word-alined buffer
;//	Buffer (r0) must be word-aligned
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY  _Read_512Byte

		stmfd	sp!, {r1 - r11}

		ldr		r1, =0xb0200010	; NFDATA
		mov		r2, #512			; 512 byte count
1
		ldr		r4, [r1]    		; Load 1st word
		ldr		r5, [r1]			; Load 2nd word
		ldr		r6, [r1]			; Load 3rd word
		ldr		r7, [r1]			; Load 4th word
		ldr		r8, [r1]			; Load 5th word
		ldr		r9, [r1]			; Load 6th word
		ldr		r10,[r1]			; Load 7th word
		ldr		r11,[r1]			; Load 8th word
		stmia	r0!,  {r4 - r11}	; Store 8 words (32 byte)

		subs		r2, r2, #32
		bne		%B1

		ldmfd	sp!,  {r1 - r11}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//	void _Read_512Byte_Unaligned(unsigned char *pBuf)
;//
;//	Read 512 bytes (1 Sector) NOT word-alined buffer
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY _Read_512Byte_Unaligned

		stmfd	sp!, {r1 - r12}

		ldr		r1, =0xb0200010	;NFDATA
		mov		r2, #480

	; Calculate number of unaligned bytes to read (r12 = 4 - (r0 & 3))
		and		r12, r0, #3
		rsb		r12, r12, #4
		mov		r3, r12

rd_unalign1						; Read unaligned bytes
        	ldrb		r4, [r1]
		strb		r4, [r0]
		add		r0, r0, #1
		subs		r3, r3, #1
		bne		rd_unalign1

rd_main							; Read 480 bytes (32 x 15)
		ldr		r4, [r1]    		; Load 1st word
		ldr		r5, [r1]			; Load 2nd word
		ldr		r6, [r1]			; Load 3rd word
		ldr		r7, [r1]			; Load 4th word
		ldr		r8, [r1]			; Load 5th word
		ldr		r9, [r1]			; Load 6th word
		ldr		r10,[r1]			; Load 7th word
		ldr		r11,[r1]			; Load 8th word
		stmia	r0!, {r4 - r11}	; Store 8 words (32 byte)

		subs		r2, r2, #32
		bne		rd_main

		ldr		r4, [r1]			; Read 28 bytes
		ldr		r5, [r1]
		ldr		r6, [r1]
		ldr		r7, [r1]
		ldr		r8, [r1]
		ldr		r9, [r1]
		ldr		r10,[r1]
		stmia	r0!, {r4 - r10}

		rsbs		r12, r12, #4		; Read trailing unaligned bytes
		beq		rd_exit

rd_unalign2
		ldrb		r4, [r1]
		strb		r4, [r0]
		add		r0, r0, #1
		subs		r12, r12, #1
		bne		rd_unalign2

rd_exit
		ldmfd    sp!, {r1 - r12}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//	void _Write_512Byte(unsigned char *pBuf)
;//
;//	Write 512 bytes (1 Sector) word-alined buffer
;//	Buffer (r0) must be word-aligned
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY    _Write_512Byte

		stmfd	sp!,{r1 - r11}

		ldr		r1, =0xb0200010  ;NFDATA
		mov		r2, #512
1
		ldmia	r0!, {r4 - r11}
		str		r4, [r1]
		str		r5, [r1]
		str		r6, [r1]
		str		r7, [r1]
		str		r8, [r1]
		str		r9, [r1]
		str		r10,[r1]
		str		r11,[r1]

		subs		r2, r2, #32
		bne		%B1

		ldmfd	sp!, {r1 - r11}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//	void _Write_512Byte_Unaligned(unsigned char *pBuf)
;//
;//	Write 512 bytes (1 Sector) NOT word-alined buffer
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY	_Write_512Byte_Unaligned
		stmfd	sp!,{r1 - r11}

		ldr		r1, =0xb0200010  ;NFDATA
		mov		r2, #480

	; Calculate number of unaligned bytes to read (r12 = 4 - (r0 & 3))
		and		r12, r0, #3
		rsb		r12, r12, #4
		mov		r3, r12

wr_unalign1
        ; Write unaligned bytes
		ldrb		r4, [r0]
		strb		r4, [r1]
		add		r0, r0, #1
		subs		r3, r3, #1
		bne		wr_unalign1

wr_main
        ; Write 480 bytes (32 x 15)
		ldmia	r0!, {r4 - r11}
		str		r4, [r1]
		str		r5, [r1]
		str		r6, [r1]
		str		r7, [r1]
		str		r8, [r1]
		str		r9, [r1]
		str		r10,[r1]
		str		r11,[r1]

		subs		r2, r2, #32
		bne		wr_main

        ; Write 28 bytes
		ldmia	r0!, {r4 - r10}
		str		r4, [r1]
		str		r5, [r1]
		str		r6, [r1]
		str		r7, [r1]
		str		r8, [r1]
		str		r9, [r1]
		str		r10,[r1]

        ; Write trailing unaligned bytes
		rsbs		r12, r12, #4
		beq		wr_exit

wr_unalign2
		ldrb		r4, [r0]
		strb		r4, [r1]
		add		r0, r0, #1
		subs		r12, r12, #1
		bne		wr_unalign2

wr_exit
		ldmfd	sp!, {r1 - r11}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//	void _Write_Dummy_468Byte_AllFF(void)
;//
;//	Write Dummy 468 bytes 0xFF
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY    _Write_Dummy_468Byte_AllFF

		stmfd	sp!,{r1 - r2}

		ldr		r0, =0xFFFFFFFF
		ldr		r1, =0xb0200010  ;NFDATA

		str		r0, [r1]			; write 20 bytes
		str		r0, [r1]
		str		r0, [r1]
		str		r0, [r1]
		str		r0, [r1]

		mov		r2, #448			; 468-20 byte count
1
		str		r0, [r1]			; 1
		str		r0, [r1]			; 2
		str		r0, [r1]			; 3
		str		r0, [r1]			; 4
		str		r0, [r1]			; 5
		str		r0, [r1]			; 6
		str		r0, [r1]			; 7
		str		r0, [r1]			; 8

		subs		r2, r2, #32
		bne		%B1

		ldmfd	sp!, {r1 - r2}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//	void _Write_Dummy_428Byte_AllFF(void)
;//
;//	Write Dummy 428 bytes 0xFF
;//
;/////////////////////////////////////////////////////

		LEAF_ENTRY    _Write_Dummy_428Byte_AllFF

		stmfd	sp!,{r1 - r2}

		ldr		r0, =0xFFFFFFFF
		ldr		r1, =0xb0200010  ;NFDATA

		str		r0, [r1]			; write 12 bytes
		str		r0, [r1]
		str		r0, [r1]

		mov		r2, #416			; 428-12 byte count
1
		str		r0, [r1]			; 1
		str		r0, [r1]			; 2
		str		r0, [r1]			; 3
		str		r0, [r1]			; 4
		str		r0, [r1]			; 5
		str		r0, [r1]			; 6
		str		r0, [r1]			; 7
		str		r0, [r1]			; 8

		subs		r2, r2, #32
		bne		%B1

		ldmfd	sp!, {r1 - r2}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF


;/////////////////////////////////////////////////////
;//
;//    void _Write_Dummy_364Byte_AllFF(void)
;//
;//    Write Dummy 364 bytes 0xFF
;//
;/////////////////////////////////////////////////////

        LEAF_ENTRY    _Write_Dummy_364Byte_AllFF

        stmfd    sp!,{r1 - r2}

        ldr        r0, =0xFFFFFFFF
        ldr        r1, =0xb0200010  ;NFDATA
        str        r0, [r1]            ; write 12 bytes
        str        r0, [r1]
        str        r0, [r1]

        mov        r2, #352            ; 364-12 byte count
1
        str        r0, [r1]            ; 1
        str        r0, [r1]            ; 2
        str        r0, [r1]            ; 3
        str        r0, [r1]            ; 4
        str        r0, [r1]            ; 5
        str        r0, [r1]            ; 6
        str        r0, [r1]            ; 7
        str        r0, [r1]            ; 8

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r2}

    IF Interworking :LOR: Thumbing
        bx        lr                ; Return with Thumb mode
    ELSE
        mov        pc, lr            ; Return
    ENDIF

;/////////////////////////////////////////////////////
;//
;//    void _Write_Dummy_500Byte_AllFF(void)
;//
;//    Write Dummy 500 bytes 0xFF
;//
;/////////////////////////////////////////////////////

        LEAF_ENTRY    _Write_Dummy_500Byte_AllFF

        stmfd    sp!,{r1 - r2}

        ldr        r0, =0xFFFFFFFF
        ldr        r1, =0xb0200010  ;NFDATA
        str        r0, [r1]            ; write 20 bytes
        str        r0, [r1]
        str        r0, [r1]
        str        r0, [r1]
        str        r0, [r1]

        mov        r2, #480            ; 500-20 byte count
1
        str        r0, [r1]            ; 1
        str        r0, [r1]            ; 2
        str        r0, [r1]            ; 3
        str        r0, [r1]            ; 4
        str        r0, [r1]            ; 5
        str        r0, [r1]            ; 6
        str        r0, [r1]            ; 7
        str        r0, [r1]            ; 8

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r2}

    IF Interworking :LOR: Thumbing
        bx        lr                ; Return with Thumb mode
    ELSE
        mov        pc, lr            ; Return
    ENDIF

;/////////////////////////////////////////////////////
;//
;//    void _Write_Dummy_492Byte_AllFF(void)
;//
;//    Write Dummy 492 bytes 0xFF
;//
;/////////////////////////////////////////////////////

        LEAF_ENTRY    _Write_Dummy_492Byte_AllFF

        stmfd    sp!,{r1 - r2}

        ldr        r0, =0xFFFFFFFF
        ldr        r1, =0xb0200010  ;NFDATA
        str        r0, [r1]            ; write 12 bytes
        str        r0, [r1]
        str        r0, [r1]

        mov        r2, #480            ; 492-12 byte count
1
        str        r0, [r1]            ; 1
        str        r0, [r1]            ; 2
        str        r0, [r1]            ; 3
        str        r0, [r1]            ; 4
        str        r0, [r1]            ; 5
        str        r0, [r1]            ; 6
        str        r0, [r1]            ; 7
        str        r0, [r1]            ; 8

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r2}

    IF Interworking :LOR: Thumbing
        bx        lr                ; Return with Thumb mode
    ELSE
        mov        pc, lr            ; Return
    ENDIF

;/////////////////////////////////////////////////////
;//
;//    void _Write_Dummy_480Byte_AllFF(void)
;//
;//    Write Dummy 480 bytes 0xFF
;//
;/////////////////////////////////////////////////////

        LEAF_ENTRY    _Write_Dummy_480Byte_AllFF

        stmfd    sp!,{r1 - r2}

        ldr        r0, =0xFFFFFFFF
        ldr        r1, =0xb0200010  ;NFDATA

        mov        r2, #480            ; 480 byte count
1
        str        r0, [r1]            ; 1
        str        r0, [r1]            ; 2
        str        r0, [r1]            ; 3
        str        r0, [r1]            ; 4
        str        r0, [r1]            ; 5
        str        r0, [r1]            ; 6
        str        r0, [r1]            ; 7
        str        r0, [r1]            ; 8

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r2}

    IF Interworking :LOR: Thumbing
        bx        lr                ; Return with Thumb mode
    ELSE
        mov        pc, lr            ; Return
    ENDIF

;/////////////////////////////////////////////////////
;//
;//    void _Write_Dummy_448Byte_AllFF(void)
;//
;//    Write Dummy 448 bytes 0xFF
;//
;/////////////////////////////////////////////////////

        LEAF_ENTRY    _Write_Dummy_448Byte_AllFF

        stmfd    sp!,{r1 - r2}

        ldr        r0, =0xFFFFFFFF
        ldr        r1, =0xb0200010  ;NFDATA

        mov        r2, #448            ; 448 byte count
1
        str        r0, [r1]            ; 1
        str        r0, [r1]            ; 2
        str        r0, [r1]            ; 3
        str        r0, [r1]            ; 4
        str        r0, [r1]            ; 5
        str        r0, [r1]            ; 6
        str        r0, [r1]            ; 7
        str        r0, [r1]            ; 8

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r2}

    IF Interworking :LOR: Thumbing
        bx        lr                ; Return with Thumb mode
    ELSE
        mov        pc, lr            ; Return
    ENDIF

		LEAF_ENTRY   _STOP_FOR_BREAK

		stmfd	sp!,{r1 - r2}
		b .
		orr		r0,r0,r0

		ldmfd	sp!, {r1 - r2}

	IF Interworking :LOR: Thumbing
		bx		lr				; Return with Thumb mode
	ELSE
		mov		pc, lr			; Return
	ENDIF

    END

