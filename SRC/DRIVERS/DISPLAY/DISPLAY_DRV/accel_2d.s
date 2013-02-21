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
;------------------------------------------------------------------------------
;
;   File:  accel_2d.s
;
;   To improve 2D performance
;
;    1. perpixel alpha blend
;
;------------------------------------------------------------------------------

        INCLUDE kxarm.h

        TEXTAREA

;--------------------------------------------------------------------------------------------
; void MultiplyAlphaBit_ASM(DWORD *pdwStartAddress, DWORD dwBufferLength, DWORD AlphaConstant)
;--------------------------------------------------------------------------------------------    
        LEAF_ENTRY MultiplyAlphaBit_ASM

        stmfd          sp!,{r3 - r12}    

1    
        pld            [r0, #0x20]
        ldmia        r0, {r3 - r10}

        ;pixel 1
        mov            r12, r3, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r3, r3, #0xff<<24
        add            r3, r3, r12, lsl #24

        ;pixel 2
        mov            r12, r4, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r4, r4, #0xff<<24
        add            r4, r4, r12, lsl #24

        ;pixel 3
        mov            r12, r5, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r5, r5, #0xff<<24
        add            r5, r5, r12, lsl #24

        ;pixel 4
        mov            r12, r6, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r6, r6, #0xff<<24
        add            r6, r6, r12, lsl #24

        ;pixel 5
        mov            r12, r7, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r7, r7, #0xff<<24
        add            r7, r7, r12, lsl #24

        ;pixel 6
        mov            r12, r8, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r8, r8, #0xff<<24
        add            r8, r8, r12, lsl #24

        ;pixel 7
        mov            r12, r9, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r9, r9, #0xff<<24
        add            r9, r9, r12, lsl #24

        ;pixel 8
        mov            r12, r10, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r10, r10, #0xff<<24
        add            r10, r10, r12, lsl #24
        
        stmia       r0!, {r3 - r10}

        subs        r1, r1, #8
        cmp            r1, #8
        bge            %B1
2
        cmp            r1, #0
        beq            returnfrom_MultiplyAlphaBit_ASM

        subs        r1, r1, #1

        ldr            r12, [r0]
        mov            r12, r10, lsr #24
        mul            r12, r2, r12
        mov            r12, r12, lsr #8
        bic            r10, r10, #0xff<<24
        add            r10, r10, r12, lsl #24
        str            r12, [r0], #4

        b            %B2

returnfrom_MultiplyAlphaBit_ASM

        ldmfd          sp!, {r3 - r12}

    IF Interworking :LOR: Thumbing
        bx             lr
    ELSE
        mov            pc, lr    ; return
    ENDIF  

;--------------------------------------------------------------------------------------------
; void PerPixelAlpha_ASM(DWORD dwSrcAddr, DWORD dwDstAddr, DWORD MinWidth)
;--------------------------------------------------------------------------------------------    
        LEAF_ENTRY PerPixelAlpha_ASM

        stmfd       sp!,{r3 - r12}    

        mov            r10, #0xff000000
        orr            r10, r10, #0xff00        ;r10 = 0xff00ff00

10    
        ldmia        r0!, {r4,r5}            ;for src ARGB value
        ldmia        r1, {r6,r7}                ;for dst ARGB value

        ;; pixel 1
        mov            r11, r6, lsr #8
        bic            r11, r11, r10            ;r11 = DST 0A0G
        bic            r6, r6, r10                ;r6 = DST 0R0B

        mov            r12, r4, lsr #8
        bic            r12 , r12, r10            ;r12 = SRC 0A0G
        bic         r4, r4, r10                ;r4 = SRC 0R0B
        
        mov            r9, #0xff
        sub            r9, r9, r12, lsr #16    ;r9 = 0xff - SRC Alpha

        mov            r8, #0x00800000
        orr            r8, r8, #0x80

        mla            r11, r9, r11, r8        ;r11 = D2
        mla            r6, r9, r6, r8

        mov            r8, r11                    ;r8 => D2_aaaagggg
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D3_00aa00gg
        add            r11, r11, r8
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D4_00aa00gg

        mov            r8, r6                    ;r8 => D2_rrrrbbbb
        and         r6, r6, r10
        mov            r6, r6, lsr #8            ;r6 => D3_00rr00bb
        add            r6, r6, r8
        and            r6, r6, r10
        mov            r6, r6, lsr #8            ;r6 => D4_00rr00bb

        add            r11, r11, r12            ;r11 => D5_00aa00gg
        add            r6, r6, r4                ;r6 => D5_00rr00bb

        ; Saturate the ARGB Values.
        and            r12, r11, #0xff000000
        cmp            r12, #0
        orrne        r11, r11, #0xff0000
        and            r12, r11, #0xff00
        cmp            r12, #0
        orrne        r11, r11, #0xff
        bic            r11, r11, r10
        
        and            r12, r6, #0xff000000
        cmp            r12, #0
        orrne        r6, r6, #0xff0000
        and            r12, r6, #0xff00
        cmp            r12, #0
        orrne        r6, r6, #0xff
        bic         r6, r6, r10

        add            r6, r6, r11, lsl #8        ;r6 => DST ARGB Value

        ;; pixel 2
        mov            r11, r7, lsr #8
        bic            r11, r11, r10            ;r11 = DST 0A0G
        bic            r7, r7, r10                ;r7 = DST 0R0B

        mov            r12, r5, lsr #8
        bic            r12 , r12, r10            ;r12 = SRC 0A0G
        bic         r5, r5, r10                ;r5 = SRC 0R0B
        
        mov            r9, #0xff
        sub            r9, r9, r12, lsr #16    ;r9 = 0xff - SRC Alpha

        mov            r8, #0x00800000
        orr            r8, r8, #0x80

        mla            r11, r9, r11, r8        ;r11 = D2
        mla            r7, r9, r7, r8

        mov            r8, r11                    ;r8 => D2_aaaagggg
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D3_00aa00gg
        add            r11, r11, r8
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D4_00aa00gg

        mov            r8, r7                    ;r8 => D2_rrrrbbbb
        and         r7, r7, r10
        mov            r7, r7, lsr #8            ;r7 => D3_00rr00bb
        add            r7, r7, r8
        and            r7, r7, r10
        mov            r7, r7, lsr #8            ;r7 => D4_00rr00bb

        add            r11, r11, r12            ;r11 => D5_00aa00gg
        add            r7, r7, r5                ;r7 => D5_00rr00bb

        ; Saturate the ARGB Values.
        and            r12, r11, #0xff000000
        cmp            r12, #0
        orrne        r11, r11, #0xff0000
        and            r12, r11, #0xff00
        cmp            r12, #0
        orrne        r11, r11, #0xff
        bic            r11, r11, r10
        
        and            r12, r7, #0xff000000
        cmp            r12, #0
        orrne        r7, r7, #0xff0000
        and            r12, r7, #0xff00
        cmp            r12, #0
        orrne        r7, r7, #0xff
        bic         r7, r7, r10

        add            r7, r7, r11, lsl #8        ;r7 => DST ARGB Value

        stmia        r1!, {r6, r7}

        subs        r2, r2, #2
        cmp            r2, #2
        bge            %B10


        cmp            r2, #0
        beq            returnfrom_PerPixelAlpha_ASM
        
        ; if there is any spare pixel to process, it will be only one pixel.
        ; extra pixel process
        ldr            r4, [r0], #4
        ldr            r6, [r1]
        mov            r11, r6, lsr #8
        bic            r11, r11, r10            ;r11 = DST 0A0G
        bic            r6, r6, r10                ;r6 = DST 0R0B

        mov            r12, r4, lsr #8
        bic            r12 , r12, r10            ;r12 = SRC 0A0G
        bic         r4, r4, r10                ;r4 = SRC 0R0B
        
        mov            r9, #0xff
        sub            r9, r9, r12, lsr #16    ;r9 = 0xff - SRC Alpha

        mov            r8, #0x00800000
        orr            r8, r8, #0x80

        mla            r11, r9, r11, r8        ;r11 = D2
        mla            r6, r9, r6, r8

        mov            r8, r11                    ;r8 => D2_aaaagggg
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D3_00aa00gg
        add            r11, r11, r8
        and            r11, r11, r10
        mov            r11, r11, lsr #8        ;r11 => D4_00aa00gg

        mov            r8, r6                    ;r8 => D2_rrrrbbbb
        and         r6, r6, r10
        mov            r6, r6, lsr #8            ;r6 => D3_00rr00bb
        add            r6, r6, r8
        and            r6, r6, r10
        mov            r6, r6, lsr #8            ;r6 => D4_00rr00bb

        add            r11, r11, r12            ;r11 => D5_00aa00gg
        add            r6, r6, r4                ;r6 => D5_00rr00bb

        ; Saturate the ARGB Values.
        and            r12, r11, #0xff000000
        cmp            r12, #0
        orrne        r11, r11, #0xff0000
        and            r12, r11, #0xff00
        cmp            r12, #0
        orrne        r11, r11, #0xff
        bic            r11, r11, r10
        
        and            r12, r6, #0xff000000
        cmp            r12, #0
        orrne        r6, r6, #0xff0000
        and            r12, r6, #0xff00
        cmp            r12, #0
        orrne        r6, r6, #0xff
        bic         r6, r6, r10

        add            r6, r6, r11, lsl #8        ;r6 => DST ARGB Value
        str            r6, [r1], #4

returnfrom_PerPixelAlpha_ASM

        ldmfd       sp!, {r3 - r12}

    IF Interworking :LOR: Thumbing
        bx          lr
    ELSE
        mov         pc, lr    ; return
    ENDIF  


    END
