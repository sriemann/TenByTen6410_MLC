; Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
;
; THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
; ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
; THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
; PARTICULAR PURPOSE.

;-------------------------------------------------------------------------------
;  File: UAOSet.s 
;
;  This file implement UAOSet function. This implementaion should
;  work on most ARM based SoC. Note that newer silicon support more effective
;  test UnAllignment Operation(UAO).
;
;-------------------------------------------------------------------------------
        INCLUDE kxarm.h
        INCLUDE armmacros.s

        TEXTAREA
        
;-------------------------------------------------------------------------------
;
;  Function:  UAOEnable
;
;-------------------------------------------------------------------------------

   LEAF_ENTRY UAOEnable

        STMFD       sp!, {r0}

        MRC p15, 0, r0, c1, c0, 0
        orr r0,r0,#0x400000
        bic r0,r0,#0x2
        MCR p15, 0, r0, c1, c0, 0       
        LDMFD       sp!, {r0}
        mov     pc, lr
    ENTRY_END

;-------------------------------------------------------------------------------
;
;  Function:  UAODisable
;
;-------------------------------------------------------------------------------

   LEAF_ENTRY UAODisable

        STMFD       sp!, {r0}

        MRC p15, 0, r0, c1, c0, 0
        orr r0,r0,#0x2
        MCR p15, 0, r0, c1, c0, 0       
        LDMFD       sp!, {r0}
        mov     pc, lr
    ENTRY_END
    
    END
