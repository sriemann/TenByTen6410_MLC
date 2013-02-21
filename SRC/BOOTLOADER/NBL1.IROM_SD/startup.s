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

    INCLUDE     kxarm.h
    INCLUDE     s3c6410.inc
    INCLUDE     image_cfg.inc
    INCLUDE     MemParam_mDDR.inc

    IMPORT      main                    ; C entrypoint for Steppingstone loader.
    
    
    STARTUPTEXT

;------------------------------------------------------------------------------
;
;    StartUp Entry
;
;    Main entry point for CPU initialization.
;
;------------------------------------------------------------------------------

    LEAF_ENTRY    StartUp

        b        ResetHandler
        b        .                ; HandlerUndef    (0x00000004)
        b        .                ; HandlerSWI        (0x00000008)
        b        .                ; HandlerPabort    (0x0000000C)
        b        .                ; HandlerDabort    (0x00000010)
        b        .                ; HandlerReserved    (0x00000014)
        b        .                ; HandlerIRQ        (0x00000018)
        b        .                ; HandlerFIQ        (0x0000001C)

;------------------------------------------------------------------------------
;    End of StartUp
;------------------------------------------------------------------------------


;------------------------------------------------------------------------------
;
;    ResetHandler Function
;
;    Reset Exception Handler
;
;------------------------------------------------------------------------------

ResetHandler

;------------------------------------
;    Enable Instruction Cache
;------------------------------------

        mov        r0, #0
        mcr        p15, 0, r0, c7, c7, 0            ; Invalidate Entire I&D Cache

        mrc        p15, 0, r0, c1, c0, 0            ; Enable I Cache
        orr        r0, r0, #R1_I
        mcr        p15, 0, r0, c1, c0, 0

;------------------------------------
;    Peripheral Port Setup
;------------------------------------

        ldr        r0, =0x70000013        ; Base Addres : 0x70000000, Size : 256 MB (0x13)
        mcr        p15,0,r0,c15,c2,4

;------------------------------------
;    Disable WatchDog Timer
;------------------------------------

        ldr        r0, =WTCON
        ldr        r1, =0x0
        str        r1, [r0]

;------------------------------------
;    Interrupt Disable
;------------------------------------

        ldr        r0, =VIC0INTENCLEAR
        ldr        r1, =0xFFFFFFFF;
        str        r1, [r0]

        ldr        r0, =VIC1INTENCLEAR
        ldr        r1, =0xFFFFFFFF;
        str        r1, [r0]
        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
;;-----------------------------------------
;;Set Clock Out Pad to clock out APLL CLK 
;; For Testing
;;---------------------------------------
;        ldr     r0, =0x7f0080a0
;        ldr     r1, [r0]
;        orr     r1, r1, #0x30000000
;        str     r1, [r0]
;
;        ldr     r0, =0x7f0080a8
;        ldr     r1, [r0]
;        bic     r1, r1, #0x03000000
;        str     r1, [r0]
;
;        ldr     r0, =0x7e00f02c
;        mov     r1, #0x10000
;        str     r1, [r0]
;

;----------------------------------------------------------
;   Set the mem1drvcon to raise drive strength  for steploader ecc error
;----------------------------------------------------------

        ldr        r0, =MEM1DRVCON
;        ldr        r1, =0xFFFFFFFF
        ldr        r1, =0x55555555
        str        r1, [r0]

;----------------------------------------------------------
;    Set Operation Mode to Sync Mode or Async Mode
;----------------------------------------------------------
        
    [ (SYNCMODE)
        IF    :DEF: _IROMBOOT_
            ; Enter the OSC mode 
            ldr        r0, =CLK_SRC
            ldr        r1, [r0]
            orr        r1, r1, #0x0            ; PLL  Clockout
            str        r1, [r0]
            
            nop
            nop
            nop
            nop
            nop
        ENDIF
            bl        System_SetSyncMode
        |
            bl        System_SetAsyncMode
        ]
    
;------------------------------------
;    Prepare to Change PLL
;------------------------------------

        ldr        r0, =CLK_DIV0
        ldr        r1, [r0]
        bic        r1, r1, #0xff00
        bic        r1, r1, #0xff
        ldr        r2, = ((Startup_PCLK_DIV<<12)+(Startup_HCLKx2_DIV<<9)+(Startup_HCLK_DIV<<8)+(MPLL_DIV<<4)+(Startup_APLL_DIV<<0))
        orr        r1, r1, r2
        str        r1, [r0]
      

;------------------------------------
;    Change PLL Value
;------------------------------------

        ldr        r1, =0xffff            ;Lock Time : 0x4b1 (100us @Fin12MHz) for APLL/MPLL
        ldr        r2, =0xE13            ; Lock Time : 0xe13 (300us @Fin12MHz) for EPLL

        ldr        r0, =APLL_LOCK
        str        r1, [r0]                ; APLL Lock Time
        str        r1, [r0, #0x4]            ; MPLL Lock Time
        str        r2, [r0, #0x8]            ; EPLL Lock Time

;------------------------------------
;    Set System Clock Divider
;------------------------------------
    [{FALSE}
        ldr        r0, =CLK_DIV0
        ldr        r1, [r0]
        bic        r1, r1, #0x30000
        bic        r1, r1, #0xff00
        bic        r1, r1, #0xff

        ldr        r2, =((Startup_PCLK_DIV<<12)+(Startup_HCLKx2_DIV<<9)+(Startup_HCLK_DIV<<8)+(MPLL_DIV<<4)+(Startup_APLL_DIV<<0))

        orr        r1, r1, r2
        str        r1, [r0]
    ]
;-------------------------------------
;    Set PMS Values
;-------------------------------------
        ldr        r0, =APLL_CON
        ldr        r1, =((1<<31)+(Startup_APLL_MVAL<<16)+(Startup_APLL_PVAL<<8)+(Startup_APLL_SVAL))
        str        r1, [r0]

        ldr        r0, =MPLL_CON
        ldr        r1, =((1<<31)+(MPLL_MVAL<<16)+(MPLL_PVAL<<8)+(MPLL_SVAL))
        str        r1, [r0]

        ;ldr        r0, =EPLL_CON1
       ; ldr        r1, =EPLL_KVAL
       ; str        r1, [r0]

        ;ldr        r0, =EPLL_CON0
        ;ldr        r1, =((1<<31)+(EPLL_MVAL<<16)+(EPLL_PVAL<<8)+(EPLL_SVAL))
        ;str        r1, [r0]
        
;------------------------------------
;    Enable PLL Clock Out
;------------------------------------

        ldr        r0, =CLK_SRC
        ldr        r1, [r0]
        orr        r1, r1, #0x7            ; PLL  Clockout
        str        r1, [r0]                ; System will be waiting for PLL unlocked after this instruction

;------------------------------------
;    Expand Memory Port 1 to x32
;------------------------------------

        ldr        r0, =MEM_SYS_CFG
        ldr        r1, [r0]
        bic        r1, r1, #0x80            ; ADDR_EXPAND to "0"
        str        r1, [r0]

;------------------------------------
;    CKE_INIT Configuration
;------------------------------------

        ldr        r0, =0x7F008880        ; SPCONSLP
        ldr        r1, [r0]
        orr        r1, r1, #0x10            ; SPCONSLP[4] = 1
        str        r1, [r0]

;------------------------------------
;    Initialize Dynamic Memory Controller
;------------------------------------

        bl        InitDMC

;--------------------------------------------------
;    Initialize Stack
;    Stack size and location information is in "image_cfg.inc"
;--------------------------------------------------

        mrs        r0, cpsr

        bic        r0, r0, #Mode_MASK
        orr        r1, r0, #Mode_IRQ | NOINT
        msr        cpsr_cxsf, r1                ; IRQMode
        ldr        sp, =IRQStack_PA            ; IRQStack

        bic        r0, r0, #Mode_MASK | NOINT
        orr        r1, r0, #Mode_SVC
        msr        cpsr_cxsf, r1                ; SVCMode
        ldr        sp, =SVCStack_PA            ; SVCStack

;------------------------------------
;    Power Management Routine
;    (WakeUp Processing)
;------------------------------------

        ldr        r0, =RST_STAT
        ldr        r1, [r0]
        and        r1, r1, #0x3F
        cmp        r1, #0x8
        bne        Normal_Boot_Sequence            ; Normal Booting (Not Wake Up)

        
        ldr        r0, =DRAM_BASE_PA_START    ; DRAM Base Physical Address
        add        r0, r0, #IMAGE_NK_OFFSET        ; NK Offset in DRAM
        mov        pc, r0                        ; Jump to StartUp address
        b        .

Normal_Boot_Sequence

;------------------------------------
;    Clear DRAM
;------------------------------------

    [ {TRUE}
        mov        r1, #0
        mov     r2, #0
        mov     r3, #0
        mov     r4, #0
        mov     r5, #0
        mov     r6, #0
        mov     r7, #0
        mov     r8, #0

        ldr        r0, =DRAM_BASE_PA_START    ; Start address (Physical 0x5000.0000)
        ldr        r9, =DRAM_SIZE                ; 128 MB of RAM
10
        stmia    r0!, {r1-r8}
        subs        r9, r9, #32
        bne        %B10
    ]

;------------------------------------
;    Jump to Main() "C" Routine
;------------------------------------

        bl        main
        b         .            ; Should not be here...

        ENTRY_END

;-----------------------------------------------------------------------------
;
;    CPU MODE Setting
;
;    SyncMode or AsyncMode
;
;-----------------------------------------------------------------------------
    ;---------------------------
    ;    Set to Synchronous Mode
    ;---------------------------
    LEAF_ENTRY System_SetSyncMode

        ldr        r0, =OTHERS
        ldr        r1, [r0]
        orr        r1, r1, #0x40            ; SyncMUXSEL = DOUT_APLL
        str        r1, [r0]

        nop
        nop
        nop
        nop
        nop

        ldr        r1, [r0]
        orr        r1, r1, #0x80            ; SyncReq = request Sync
        str        r1, [r0]

WaitForSync
        ldr        r1, [r0]                ; Read OTHERS
        and        r1, r1, #0xF00            ; Wait SYNCMODEACK = 0xF
        cmp        r1, #0xF00
        bne        WaitForSync

        mov        pc, lr

        ENTRY_END


    ;---------------------------
    ;    Set to Asynchronous Mode
    ;---------------------------
    LEAF_ENTRY System_SetAsyncMode

        ldr        r0, =OTHERS
        ldr        r1, [r0]
        bic        r1, r1, #0xC0
        orr        r1, r1, #0x40            ; SyncReq = Async, SyncMUX = Sync
        str        r1, [r0]

WaitForAsync
        ldr        r1, [r0]                ; Read OTHERS
        and        r1, r1, #0xF00            ; Wait SYNCMODEACK = 0x0
        cmp        r1, #0x0
        bne        WaitForAsync

        ldr        r0, =OTHERS
        ldr        r1, [r0]
        bic        r1, r1, #0x40            ; SyncMUX = Async
        str        r1, [r0]

        nop
        nop
        nop
        nop
        nop

        mov        pc, lr

        ENTRY_END



;------------------------------------------------------------------------------
;
;    InitDMC Function
;
;    Initialize DMC(Dynamic Memory Controller) and DRAM
;
;------------------------------------------------------------------------------

    LEAF_ENTRY InitDMC

;---------------------------
; Initialize DMC (mDDR)
;---------------------------

    [    USE_DMC1

        ldr        r0, =DMC1_BASE                    ; DMC1 base address

        ldr        r1, =0x4
        str        r1, [r0, #INDEX_MEMCCMD]            ; Enter the Config. Mode

    [    DVS_EN
        ldr        r1, =DMC_DDR_REFRESH_PRD_DVS    ; Refresh Rate is set to the lowest HCLK frequency when DVS is enabled
        str        r1, [r0, #INDEX_REFRESH]
    |
        ldr        r1, =DMC_DDR_REFRESH_PRD        ; Timing Para.
        str        r1, [r0, #INDEX_REFRESH]
    ]

        ldr        r1, =DMC_DDR_CAS_LATENCY
        str        r1, [r0, #INDEX_CASLAT]

        ldr        r1, =DMC_DDR_t_DQSS
        str        r1, [r0, #INDEX_T_DQSS]

        ldr        r1, =DMC_DDR_t_MRD
        str        r1, [r0, #INDEX_T_MRD]

        ldr        r1, =DMC_DDR_t_RAS
        str        r1, [r0, #INDEX_T_RAS]

        ldr        r1, =DMC_DDR_t_RC
        str        r1, [r0, #INDEX_T_RC]

        ldr        r1, =DMC_DDR_t_RCD
        ldr        r2, =DMC_DDR_schedule_RCD
        orr        r1, r1, r2
        str        r1, [r0, #INDEX_T_RCD]

        ldr        r1, =DMC_DDR_t_RFC
        ldr        r2, =DMC_DDR_schedule_RFC
        orr        r1, r1, r2
        str        r1, [r0, #INDEX_T_RFC]

        ldr        r1, =DMC_DDR_t_RP
        ldr        r2, =DMC_DDR_schedule_RP
        orr        r1, r1, r2
        str        r1, [r0, #INDEX_T_RP]

        ldr        r1, =DMC_DDR_t_RRD
        str        r1, [r0, #INDEX_T_RRD]

        ldr        r1, =DMC_DDR_t_WR
        str        r1, [r0, #INDEX_T_WR]

        ldr        r1, =DMC_DDR_t_WTR
        str        r1, [r0, #INDEX_T_WTR]

        ldr        r1, =DMC_DDR_t_XP
        str        r1, [r0, #INDEX_T_XP]

        ldr        r1, =DMC_DDR_t_XSR
        str        r1, [r0, #INDEX_T_XSR]

        ldr        r1, =DMC_DDR_t_ESR
        str        r1, [r0, #INDEX_T_ESR]
     [   BSP_TYPE = BSP_DRAM128
        ldr        r1, =DMC1_MEM_CFG
        str        r1, [r0, #INDEX_MEMCFG]
     ]

     [  BSP_TYPE = BSP_DRAM256
        ldr        r1, =DMC1_MEM_CFG
        str        r1, [r0, #INDEX_MEMCFG]
     ]

        ldr        r1, =DMC1_MEM_CFG2
        str        r1, [r0, #INDEX_MEMCFG2]

    [    USE_DMC1_CHIP0
      [   BSP_TYPE = BSP_DRAM128
        ldr        r1, =DMC1_CHIP0_CFG
        str        r1, [r0, #INDEX_CHIP0_CFG]
      ]
      [   BSP_TYPE = BSP_DRAM256
        ldr        r1, =DMC1_CHIP0_CFG
        str        r1, [r0, #INDEX_CHIP0_CFG]
      ]
    ]

    [    USE_DMC1_CHIP1
        ldr        r1, =DMC1_CHIP1_CFG
        str        r1, [r0, #INDEX_CHIP1_CFG]
    ]

        ldr        r1, =DMC1_USER_CFG
        str        r1, [r0, #INDEX_USER_CFG]

    ;---------------------------------------------
    ; DMC1 DDR Chip 0 configuration direct command reg
    ;---------------------------------------------
    [    USE_DMC1_CHIP0

        ; DMC1 DDR Chip 0 configuration direct command reg
        ; NOP
        ldr        r1, =DMC_NOP0
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Precharge All
        ldr        r1, =DMC_PA0
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Auto Refresh    2 time
        ldr        r1, =DMC_AR0
        str        r1, [r0, #INDEX_DIRECTCMD]
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; EMRS
        ldr        r1, =DMC_mDDR_EMR0            ; DS:Full, PASR:Full Array
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Mode Reg (MRS, CAS3, BL4)
        ldr        r1, =DMC_mDDR_MR0
        str        r1, [r0, #INDEX_DIRECTCMD]

    ] ; USE_DMC1_CHIP0

    ;---------------------------------------------
    ; DMC1 DDR Chip 1 configuration direct command reg
    ;---------------------------------------------
    [    USE_DMC1_CHIP1

        ; DMC1 DDR Chip 1 configuration direct command reg
        ; NOP
        ldr        r1, =DMC_NOP1
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Precharge All
        ldr        r1, =DMC_PA1
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Auto Refresh    2 time
        ldr        r1, =DMC_AR1
        str        r1, [r0, #INDEX_DIRECTCMD]
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; EMRS
        ldr        r1, =DMC_mSDR_EMR1            ; DS:Full, PASR:Full Array
        str        r1, [r0, #INDEX_DIRECTCMD]

        ; Mode Reg (MRS, CAS3, BL4)
        ldr        r1, =DMC_mDDR_MR1
        str        r1, [r0, #INDEX_DIRECTCMD]

    ] ; USE_DMC1_CHIP1

        ; Enable DMC1
        mov        r1, #0x0
        str        r1, [r0, #INDEX_MEMCCMD]

Wait_for_DMC1Ready

        ldr        r1, [r0, #INDEX_MEMSTAT]
        mov        r2, #0x3
        and        r1, r1, r2
        cmp        r1, #0x1
        bne        Wait_for_DMC1Ready

    ] ; USE_DMC1

        NOP

        mov     pc, lr

        ENTRY_END

        END

