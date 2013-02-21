; Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
;
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
; ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
; PARTICULAR PURPOSE.

;code only support 32 byte aligned data (8 words) 
	CODE32

	GBLL	ENABLING_BURST
ENABLING_BURST 	SETL	{TRUE}

	AREA	|.text|, CODE, ARM
	EXPORT	|_gl_asm_memset|
	EXPORT	|_gl_asm_memcpy_3l|
	EXPORT	|_gl_asm_memcpy_4l|
	EXPORT  |_gl_asm_convertPixels_ABGR2ARGB|
	EXPORT	|_gl_asm_nop|
	EXPORT	|INTLOCK|
	EXPORT	|INTFREE|	
 IF ENABLING_BURST	
	EXPORT  |_gl_asm_send_hostfifo2|
	EXPORT  |_gl_asm_send_hostfifo3|
	EXPORT  |_gl_asm_send_hostfifo4|
       EXPORT  |_gl_asm_send_hostfifo5|
       EXPORT  |_gl_asm_send_hostfifo6|
       EXPORT  |_gl_asm_send_hostfifo7|
	EXPORT  |_gl_asm_send_hostfifo8|
	EXPORT  |_gl_asm_send_hostfifo2_aux|	
	EXPORT  |_gl_asm_send_hostfifo3_aux|
	EXPORT  |_gl_asm_send_hostfifo4_aux|
 ENDIF

FIFOADDR                        EQU     0xB080C000


_gl_asm_memset
	stmfd		sp!,{r4 - r11, lr} ; r0 -> dst, r1 -> c, r2 -> num
	;destination
	;ldr          		r1, [sp]
	;color
	;ldr          		r4, [sp, #0x4]
	;count
	;ldr			r2, [sp, #0x8]
	mov          	r4, r1
	mov          	r5, r1
	mov          	r6, r1
	mov          	r7, r1
	mov          	r8, r1
	mov          	r9, r1
	mov          	r10, r1
	mov          	r11, r1
loopcpy
;	cmp          	r2, #0             
;	beq          	done
	stmia        	r0!, {r4-r11}
	subs         	r2, r2, #8
	bne          	loopcpy
done
	ldmfd		sp!, {r4 - r11, pc}
	bx			lr
	ENDP  ; |_gl_asm_memset|


_gl_asm_memcpy_3l
	stmfd		sp!,{r3 - r5} ; r0 -> dst, r1 -> src, r2 -> size
loopcpy3
	ldmia           r1!, {r3-r5}
	stmia        	r0!, {r3-r5}
	subs         	r2, r2, #1
	bne          	loopcpy3
	ldmfd		sp!, {r3 - r5}
	bx			lr
	ENDP  ; |_gl_asm_memcpy_3l|


_gl_asm_memcpy_4l
	stmfd		sp!,{r3 - r6} ; r0 -> dst, r1 -> src, r2 -> size
loopcpy4
	ldmia           r1!, {r3-r6}
	stmia        	r0!, {r3-r6}
	subs         	r2, r2, #1
	bne          	loopcpy4
	ldmfd		sp!, {r3 - r6}
	bx			lr
	ENDP  ; |_gl_asm_memcpy_4l|


_gl_asm_convertPixels_ABGR2ARGB
	stmfd		sp!,{r3 - r7} ; r0 -> dst, r1 -> src, r2 -> size

	LDR     r6, =0xff00ff00
	LDR     r7, =0x00ff00ff
loop_conv1

	LDR 	r3,[r1],#4  		; load SRC r3

	AND 	r4, r3, r6  		; r4 = A0C0
	AND		r5, r3, r7	    	; r5 = 0B0D
	MOV		r5, r5, ROR #16		; r5 = 0D0B
	ORR		r4, r4, r5
	STR		r4, [r0],#4 		; store r5 into destination
	subs    r2, r2, #1
	bne     loop_conv1

	ldmfd	sp!, {r3 - r7}
	bx		lr
	ENDP  ; |_gl_asm_convertPixels_ABGR2ARGB|


_gl_asm_nop
	stmfd		sp!,{r0 - r12, lr}
	nop 
	ldmfd		sp!, {r0 - r12, pc}
	bx			lr
	ENDP  ; |_gl_asm_nop|	

 IF ENABLING_BURST	
_gl_asm_send_hostfifo2
	stmfd		sp!,{r0 - r3, lr}

	ldmfd		r1!, {r2-r3}
 	stmia             r0!, {r2-r3}
 
	ldmfd		sp!, {r0 - r3, pc}
	bx			lr
       ENDP
       
_gl_asm_send_hostfifo3
	stmfd		sp!,{r0 - r4, lr}

	ldmfd		r1!, {r2-r4}
	stmia             r0!, {r2-r4}

	ldmfd		sp!, {r0 - r4, pc}
	bx			lr
       ENDP

_gl_asm_send_hostfifo4
	stmfd		sp!,{r0 - r5, lr}

	ldmfd		r1!, {r2-r5}
	stmia             r0!, {r2-r5}

	ldmfd		sp!, {r0 - r5, pc}
	bx			lr
       ENDP
       
_gl_asm_send_hostfifo5
	stmfd		sp!,{r0 - r6, lr}

	ldmfd		r1!, {r2-r6}
	stmia             r0!, {r2-r6}		

	ldmfd		sp!, {r0 - r6, pc}
	bx			lr
       ENDP
       
_gl_asm_send_hostfifo6
	stmfd		sp!,{r0 - r7, lr}

	ldmfd		r1!, {r2-r7}
	stmia             r0!, {r2-r7}		

	ldmfd		sp!, {r0 - r7, pc}
	bx			lr
       ENDP
       
_gl_asm_send_hostfifo7
	stmfd		sp!,{r0 - r8, lr}

	ldmfd		r1!, {r2-r8}
	stmia             r0!, {r2-r8}		

	ldmfd		sp!, {r0 - r8, pc}
	bx			lr
       ENDP

_gl_asm_send_hostfifo8
	stmfd		sp!,{r0 - r9, lr}

	ldmfd		r1!, {r2-r9}
	stmia             r0!, {r2-r9}		

	ldmfd		sp!, {r0 - r9, pc}
	bx			lr
       ENDP  

_gl_asm_send_hostfifo2_aux
	stmfd		sp!,{r0 - r2, lr}

	ldr			r2, =0xB080C000
	stmia             r2!, {r0-r1}

	ldmfd		sp!, {r0 - r2, pc}
	bx			lr
       ENDP
       
_gl_asm_send_hostfifo3_aux
	stmfd		sp!,{r0 - r3, lr}

	ldr			r3, =0xB080C000
	stmia             r3!, {r0-r2}

	ldmfd		sp!, {r0 - r3, pc}
	bx			lr
       ENDP

_gl_asm_send_hostfifo4_aux
	stmfd		sp!,{r0 - r4, lr}

 	ldr			r4, =0xB080C000
	stmia             r4!, {r0-r3}

	ldmfd		sp!, {r0 - r4, pc}
	bx			lr
       ENDP
 ENDIF     


INTLOCK
       stmfd		sp!,{r0 - r1, lr}
       mrs     r0, cpsr                        ; (r0) = current status
       bic     r1, r0, #0x80                   ; clear interrupt disable bit
       msr     cpsr, r1                        ; update status register
       ldmfd		sp!, {r0 - r1, lr}
       ENDP                                  ; return to caller


INTFREE
        stmfd		sp!,{r0 - r1, lr}
        mrs     r0, cpsr                        ; (r0) = current status
        orr     r1, r0, #0x80                   ; set interrupt disable bit
        msr     cpsr, r1                        ; update status register
        ldmfd		sp!, {r0 - r1, lr}
       ENDP                                  ; return to caller        
	END
	
