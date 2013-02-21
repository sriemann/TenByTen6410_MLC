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
;
; NAND Flash Related Routines
;

    INCLUDE kxarm.h

    TEXTAREA

;----------------------------------------------
NFDATA        EQU        (0xb0200010)
;----------------------------------------------

;----------------------------------------------
; Read one sector.  Buffer (r0) must be aligned.
;
    LEAF_ENTRY RdPage512

        stmfd    sp!,{r1 - r11}

        ldr        r1, =NFDATA
        mov        r2, #0x200
1
        ldr        r4, [r1]
        ldr        r5, [r1]
        ldr        r6, [r1]
        ldr        r7, [r1]
        ldr        r8, [r1]
        ldr        r9, [r1]
        ldr        r10, [r1]
        ldr        r11, [r1]

        stmia    r0!, {r4 - r11}
        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r11}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF

;----------------------------------------------
; Read one sector.  Handles case where buffer (r0) is unaligned.
;
    LEAF_ENTRY RdPage512Unalign

        stmfd    sp!,{r1 - r12}

        ldr        r1, =NFDATA
        mov        r2, #480

        ; Calculate number of unaligned bytes to read (r12 = 4 - (r0 & 3))
        and        r12, r0, #3
        rsb        r12, r12, #4
        mov        r3, r12

rd_unalign1
        ; Read unaligned bytes
        ldrb        r4, [r1]
        strb        r4, [r0]
        add        r0, r0, #1
        subs        r3, r3, #1
        bne        rd_unalign1

rd_main
        ; Read 480 bytes (32 x 15)
        ldr        r4, [r1]
        ldr        r5, [r1]
        ldr        r6, [r1]
        ldr        r7, [r1]
        ldr        r8, [r1]
        ldr        r9, [r1]
        ldr        r10, [r1]
        ldr        r11, [r1]

        stmia    r0!,{r4-r11}
        subs        r2, r2, #32
        bne        rd_main

        ; Read 28 bytes
        ldr        r4, [r1]
        ldr        r5, [r1]
        ldr        r6, [r1]
        ldr        r7, [r1]
        ldr        r8, [r1]
        ldr        r9, [r1]
        ldr        r10, [r1]
        stmia    r0!, {r4 - r10}

        ; Read trailing unaligned bytes
        rsbs        r12, r12, #4
        beq        rd_exit

rd_unalign2
        ldrb        r4, [r1]
        strb        r4, [r0]
        add        r0, r0, #1
        subs        r12, r12, #1
        bne        rd_unalign2
rd_exit
        ldmfd    sp!, {r1 - r12}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF


;----------------------------------------------
; Write one sector.  Buffer (r0) must be aligned.
;

    LEAF_ENTRY WrPage512

        stmfd    sp!,{r1 - r11}

        ldr        r1, =NFDATA
        mov        r2, #0x200
1
        ldmia    r0!, {r4 - r11}

        str        r4, [r1]
        str        r5, [r1]
        str        r6, [r1]
        str        r7, [r1]
        str        r8, [r1]
        str        r9, [r1]
        str        r10, [r1]
        str        r11, [r1]

        subs        r2, r2, #32
        bne        %B1

        ldmfd    sp!, {r1 - r11}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF

;----------------------------------------------
; Writes one sector.  Handles case where buffer (r0) is unaligned.
;

    LEAF_ENTRY WrPage512Unalign

        stmfd    sp!,{r1 - r11}

        ldr        r1, =NFDATA
        mov        r2, #480

        ; Calculate number of unaligned bytes to read (r12 = 4 - (r0 & 3))
        and        r12, r0, #3
        rsb        r12, r12, #4
        mov        r3, r12

wr_unalign1
        ; Write unaligned bytes
        ldrb        r4, [r0]
        strb        r4, [r1]
        add        r0, r0, #1
        subs        r3, r3, #1
        bne        wr_unalign1

wr_main
        ; Write 480 bytes (32 x 15)
        ldmia    r0!, {r4-r11}

        str        r4, [r1]
        str        r5, [r1]
        str        r6, [r1]
        str        r7, [r1]
        str        r8, [r1]
        str        r9, [r1]
        str        r10, [r1]
        str        r11, [r1]

        subs        r2, r2, #32
        bne        wr_main

        ; Write 28 bytes
        ldmia    r0!, {r4 - r10}
        str        r4, [r1]
        str        r5, [r1]
        str        r6, [r1]
        str        r7, [r1]
        str        r8, [r1]
        str        r9, [r1]
        str        r10, [r1]

        ; Write trailing unaligned bytes
        rsbs        r12, r12, #4
        beq        wr_exit

wr_unalign2
        ldrb        r4, [r0]
        strb        r4, [r1]
        add        r0, r0, #1
        subs        r12, r12, #1
        bne        wr_unalign2

wr_exit
        ldmfd    sp!, {r1 - r11}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF

;----------------------------------------------
; Read page/sector information.  This includes logical sector number
; and block status flags (RO, OEM-defined, etc.).
;

    LEAF_ENTRY RdPageInfo

        stmfd    sp!, {r1 - r4}

        ldr        r1, =NFDATA
        ldr        r3, [r1]
        ldr        r4, [r1]
        stmia    r0!, {r3 - r4}
        ldmfd    sp!, {r1 - r4}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF

;----------------------------------------------
; Store page/sector information.  This includes logical sector number
; and block status flags (RO, OEM-defined, etc.).
;

    LEAF_ENTRY WrPageInfo

        stmfd    sp!, {r1 - r3}

        ldr        r1, =NFDATA
        ldmia    r0!, {r2 - r3}
        str        r2, [r1]
        str        r3, [r1]
        ldmfd    sp!, {r1 - r3}

    IF Interworking :LOR: Thumbing
        bx        lr
    ELSE
        mov        pc, lr    ; return
    ENDIF

    END

