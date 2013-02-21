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
;   Kernel startup routine for Samsung SMDK6410 board. Hardware is
;   initialized in boot loader - so there isn't much code at all.
;
;------------------------------------------------------------------------------

        INCLUDE    kxarm.h
        INCLUDE    s3c6410.inc
        INCLUDE    image_cfg.inc

        IMPORT    OALClearUTLB
        IMPORT    OALClearITLB
        IMPORT    OALClearDTLB
        IMPORT    OALFlushICache
        IMPORT    OALFlushDCache

        IMPORT    System_EnableICache
        IMPORT    System_SetSyncMode
        IMPORT    System_SetAsyncMode
        IMPORT    System_DisableVIC
        IMPORT    System_EnableBP

        IMPORT    System_WaitForInterrupt

        IMPORT    KernelStart

        STARTUPTEXT

;------------------------------------------------------------------------------
;
;    Macro for Sleep Code
;
;------------------------------------------------------------------------------

SYSCTL_SBZ_MASK        EQU        (0xCC1A0000)
SYSCTL_SBO_MASK        EQU        (0x00000070)

;MMUTTB_SBZ_MASK        EQU        (0x00003FE0)        ; for 16KB Boundary Size of TTB0
MMUTTB_SBZ_MASK        EQU        (0x00001FE0)        ; for 8KB Boundary Size of TTB0
;MMUTTB_SBZ_MASK        EQU        (0x00000FE0)        ; for 4KB Boundary Size of TTB0
;MMUTTB_SBZ_MASK        EQU        (0x000007E0)        ; for 2KB Boundary Size of TTB0

;------------------------------------------------------------------------------
;    Macro For VFP
;------------------------------------------------------------------------------
VFPEnable                           EQU        (0x40000000)

;------------------------------------------------------------------------------
;
;    Macro for LED on SMDK Board (GPN[15:12])
;
;    LED_ON for physical address domain
;    VLED_ON for virtual address domain
;
;------------------------------------------------------------------------------


;------------------------------------------------------------------------------
;    End of Macro
;------------------------------------------------------------------------------


;------------------------------------------------------------------------------
;
;    StartUp Entry
;
;    Main entry point for CPU initialization.
;
;------------------------------------------------------------------------------

    LEAF_ENTRY      StartUp

        b        ResetHandler                ; Jump over Power-Off code

HandlerUndef
        b        HandlerUndef

HandlerSWI
        b        HandlerSWI

HandlerPabort
        b        HandlerPabort

HandlerDabort
        b        HandlerDabort

HandlerReserved
        b        HandlerReserved

HandlerIRQ
        b        HandlerIRQ

HandlerFIQ
        b        HandlerFIQ

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
        bl        System_EnableICache            ; Enable I Cache

;------------------------------------
;    Peripheral Port Setup
;------------------------------------

        ldr        r0, =0x70000013        ; Base Addres : 0x70000000, Size : 256 MB (0x13)
        mcr        p15,0,r0,c15,c2,4

;------------------------------------
;    Interrupt Disable
;------------------------------------

        ldr        r0, =VIC0INTENCLEAR
        ldr        r1, =0xFFFFFFFF;
        str        r1, [r0]

        ldr        r0, =VIC1INTENCLEAR
        ldr        r1, =0xFFFFFFFF;
        str        r1, [r0]

;------------------------------------
;    Disable WatchDog Timer
;------------------------------------

        ldr        r0, =WTCON
        ldr        r1, =0x0
        str        r1, [r0]

    [ CHANGE_PLL_CLKDIV_ON_KERNEL

;-----------------------------------------------
;    Change Operation Mode to Sync Mode or Async Mode
;-----------------------------------------------

        ldr        r0, =OTHERS
        ldr        r1, [r0]
        and        r1, r1, #0x40

        cmp        r1, #0x40            ; OTHERS[6] = 0:AsyncModde 1:SyncMode

        [ (SYNCMODE) 
        bne        System_SetSyncMode
        |
        beq        System_SetAsyncMode
        ]


;---------------------------------------
;    Check PLL and CLKDIV
;---------------------------------------

        ldr        r3, =0x83FF3F07        ; Mask for APLL_CON/MPLL_CON
        ldr        r4, =0x80FF3F07        ; Mask for EPLL_CON0
        ldr        r5, =0x0000FFFF        ; Mask for EPLL_CON1
        ldr        r6, =0x0003FF17        ; Mask for CLKDIV0

        ldr        r0, =APLL_CON        ; Check APLL
        ldr        r1, [r0]
        and        r1, r1, r3
        ldr        r2, =((1<<31)+(APLL_MVAL<<16)+(APLL_PVAL<<8)+(APLL_SVAL))    ; APLL_CON value to configure
        cmp        r1, r2
        bne        PLL_NeedToConfigure

        ldr        r0, =MPLL_CON        ; Check MPLL
        ldr        r1, [r0]
        and        r1, r1, r3
        ldr        r2, =((1<<31)+(MPLL_MVAL<<16)+(MPLL_PVAL<<8)+(MPLL_SVAL))    ; MPLL_CON value to configure
        cmp        r1, r2
        bne        PLL_NeedToConfigure

        ldr        r0, =EPLL_CON0        ; Check EPLL_CON0
        ldr        r1, [r0]
        and        r1, r1, r4
        ldr        r2, =((1<<31)+(EPLL_MVAL<<16)+(EPLL_PVAL<<8)+(EPLL_SVAL))        ; EPLL_CON0 value to configure
        cmp        r1, r2
        bne        PLL_NeedToConfigure

        ldr        r0, =EPLL_CON1        ; Check EPLL_CON1
        ldr        r1, [r0]
        and        r1, r1, r5
        ldr        r2, =EPLL_KVAL        ; EPLL_CON1 value to configure
        cmp        r1, r2
        bne        PLL_NeedToConfigure

        ldr        r0, =CLK_DIV0        ; Check CLKDIV0
        ldr        r1, [r0]
        and        r1, r1, r6

        ldr        r2, =((PCLK_DIV<<12)+(HCLKx2_DIV<<9)+(HCLK_DIV<<8)+(MPLL_DIV<<4)+(APLL_DIV<<0))        ; CLKDIV0 value to configure
        
        cmp        r1, r2
        bne        CLKDIV_NeedToConfigure

        b        PLL_CLKDIV_AlreadyConfigured    ; APLL/MPLL/EPLL and CLKDIV0 is already configured

;------------------------------------
;    Prepare to Change PLL
;------------------------------------

PLL_NeedToConfigure

;------------------------------------
;    Disable PLL Clock Out
;------------------------------------

        ldr        r0, =CLK_SRC
        ldr        r1, [r0]
        bic        r1, r1, #0x7            ; FIN out
        str        r1, [r0]                
        
        ldr        r0, =CLK_DIV0
        ldr        r1, [r0]
        bic        r1, r1, #0xff00
        bic        r1, r1, #0xff
        ldr        r2, = ((PCLK_DIV<<12)+(HCLKx2_DIV<<9)+(HCLK_DIV<<8)+(MPLL_DIV<<4)+(APLL_DIV<<0))
        orr        r1, r1, r2
        str        r1, [r0]


;------------------------------------
;    Change PLL Value
;------------------------------------

        ldr        r1, =0x4B1            ; Lock Time : 0x4b1 (100us @Fin12MHz) for APLL/MPLL
        ldr        r2, =0xE13            ; Lock Time : 0xe13 (300us @Fin12MHz) for EPLL

        ldr        r0, =APLL_LOCK
        str        r1, [r0]                ; APLL Lock Time
        str        r1, [r0, #0x4]            ; MPLL Lock Time
        str        r2, [r0, #0x8]            ; EPLL Lock Time

        ldr        r0, =APLL_CON
        ldr        r1, =((1<<31)+(APLL_MVAL<<16)+(APLL_PVAL<<8)+(APLL_SVAL))
        str        r1, [r0]

        ldr        r0, =MPLL_CON
        ldr        r1, =((1<<31)+(MPLL_MVAL<<16)+(MPLL_PVAL<<8)+(MPLL_SVAL))
        str        r1, [r0]

        ldr        r0, =EPLL_CON1
        ldr        r1, =EPLL_KVAL
        str        r1, [r0]

        ldr        r0, =EPLL_CON0
        ldr        r1, =((1<<31)+(EPLL_MVAL<<16)+(EPLL_PVAL<<8)+(EPLL_SVAL))
        str        r1, [r0]

;------------------------------------
;    Set System Clock Divider
;------------------------------------

CLKDIV_NeedToConfigure

        ldr        r0, =CLK_DIV0
        ldr        r1, [r0]
        bic        r1, r1, #0x30000
        bic        r1, r1, #0xff00
        bic        r1, r1, #0xff

        ldr        r2, =((PCLK_DIV<<12)+(HCLKx2_DIV<<9)+(HCLK_DIV<<8)+(MPLL_DIV<<4)+(APLL_DIV<<0))        ; CLKDIV0 value to configure
        
        orr        r1, r1, r2
        str        r1, [r0]

;------------------------------------
;    Enable PLL Clock Out
;------------------------------------

        ldr        r0, =CLK_SRC
        ldr        r1, [r0]
        orr        r1, r1, #0x7            ; PLL  Clockout
        str        r1, [r0]                ; System will be waiting for PLL unlocked after this instruction

PLL_CLKDIV_AlreadyConfigured

    ]    ; CHANGE_PLL_CLKDIV_ON_KERNEL

;------------------------------------
;    Expand Memory Port 1 to x32
;------------------------------------

        ldr        r0, =MEM_SYS_CFG
        ldr        r1, [r0]
        bic        r1, r1, #0x80            ; ADDR_EXPAND to "0"
        str        r1, [r0]

;------------------------------------
;    Store BSP Data
;------------------------------------

        ldr        r0, =INFORM0
        ldr        r1, =0x64107618        ; June 18, 2007
        str        r1, [r0]
;------------------------------------
; Enable VFP via Coprocessor Access Cotrol Register
;------------------------------------
        mrc        p15, 0, r0, c1, c0, 2
        orr        r0, r0, #0x00F00000
        mcr        p15, 0, r0, c1, c0, 2
;------------------------------------
; Add following: SISO added
; Enable FPEXC enable bit to enable VFP
;------------------------------------
        MOV        r1, #0
        MCR        p15, 0, r1, c7, c5, 4
        MOV        r0,#VFPEnable
        FMXR       FPEXC, r0       ; FPEXC = r0
        nop
        nop
        nop
        nop
        nop

;------------------------------------
;    Power Management Routine
;    (WakeUp Processing)
;------------------------------------

    [    {TRUE}

        ldr        r0, =RST_STAT
        ldr        r1, [r0]
        and        r1, r1, #0x3F
        cmp        r1, #0x8
        bne        BringUp_WinCE_from_Reset            ; Normal Mode Booting


        ;-------------------------------
        ; Calculate CheckSum of Sleep Data

        ldr        r3, =IMAGE_SLEEP_DATA_PA_START    ; Base of Sleep Data Area
        ldr        r2, =0x0                            ; CheckSum is in r2
        ldr        r0, =(SLEEPDATA_SIZE-1)            ; Size of Sleep Data Area (in words)

ReCheckSum_Loop

        ldr        r1, [r3], #4
        and        r1, r1, #0x1
        mov        r1, r1, LSL #31
        orr        r1, r1, r1, LSR #1
        add        r2, r2, r1                            ; CheckSum is in r2
        subs        r0, r0, #1
        bne        ReCheckSum_Loop

        ldr        r0, =INFORM1
        ldr        r1, [r0]
        cmp        r1, r2                            ; Compare CheckSum Recalculated and Value in DRAM
        bne        CheckSum_Corrupted

CheckSum_Granted

        ;-------------------------------
        ; Restore CP15 Register

        ldr        r10, =IMAGE_SLEEP_DATA_PA_START    ; Base of Sleep Data Area
        ldr        r6,    [r10, #SleepState_MMUDOMAIN]    ; Domain Access Control Register
        ldr        r5,    [r10, #SleepState_MMUTTBCTL]    ; TTB Control Register
        ldr        r4,    [r10, #SleepState_MMUTTB1]    ; TTB Register1
        ldr        r3,    [r10, #SleepState_MMUTTB0]    ; TTB Register0
        ldr        r2,    [r10, #SleepState_SYSCTL]        ; System Control Register
        ldr        r1,    [r10, #SleepState_WakeAddr]    ; Return Address
        nop
        nop
        nop
        nop
        nop

        mcr        p15, 0, r6, c3, c0, 0        ; Restore Domain Access Control Register
        mcr        p15, 0, r5, c2, c0, 2        ; Restore TTB Control Register
        mcr        p15, 0, r4, c2, c0, 1        ; Restore TTB Register1
        mcr        p15, 0, r3, c2, c0, 0        ; Restore TTB Register0

        mov        r0, #0x0
        mcr        p15, 0, r0, c8, c7, 0           ; Invalidate I & D TLB

        mcr        p15, 0, r2, c1, c0, 0        ; Restore System Control Register (MMU Control)

        nop
        nop
        nop
        nop
        nop

        ;-------------------------------
        ; Return to WakeUp_Address

        mov        pc, r1                    ; Jump to Virtual Return Address
        b        .

CheckSum_Corrupted

        ;--------------------------------
        ; Bad News... CheckSum is Corrupted

        ldr        r0, =DRAM_BASE_PA_START        ; DRAM Base Physical Address
        add        r0, r0, #IMAGE_NK_OFFSET            ; NK Offset in DRAM
        mov        pc, r0                            ; Jump to StartUp address
    ]

;------------------------------------
;    End of Power Management Routine
;------------------------------------

BringUp_WinCE_from_Reset

;------------------------------------
;    Clear DRAM
;------------------------------------

    [ CLEAR_DRAM_ON_KERNEL

        mov        r1, #0
        mov     r2, #0
        mov     r3, #0
        mov     r4, #0
        mov     r5, #0
        mov     r6, #0
        mov     r7, #0
        mov     r8, #0

        ldr        r0, =IMAGE_NK_PA_START   ; Start address (physical 0x5200.0000)
        ldr        r9, =IMAGE_NK_SIZE       ; 80 MB of RAM (1MB + 31MB + 80MB + 16MB)
10
        stmia    r0!, {r1-r8}
        subs        r9, r9, #32
        bne        %B10
    ]

;------------------------------------
;    Flush TLB, Invalidate ICache, DCache
;------------------------------------

        mov     r0, #0
        mcr     p15, 0, r0, c8, c7, 0           ; flush both TLB
        mcr     p15, 0, r0, c7, c5, 0           ; invalidate instruction cache
        mcr     p15, 0, r0, c7, c6, 0           ; invalidate data cache

;------------------------------------
;    Disable VIC
;------------------------------------

        bl        System_DisableVIC

;------------------------------------
;    Enable Branch Prediction
;------------------------------------

        bl         System_EnableBP

;------------------------------------
;    MMU Option (for ARM1176, Just for Test)
;------------------------------------

      [    {FALSE}
        ; Set MMU options
        mrc        p15, 0, r0, c1, c0, 0
        ;bic        r0, r0, #(1 :SHL: 23)        ; Clear the XP bit to enable subpage AP bit
        ;bic        r0, r0, #(1 :SHL: 15)        ; Clear the L4 bit to enable arm V6
        ;orr        r0, r0, #(1 :SHL: 1)        ; Set the A bit to enable alignment checking.
        orr        r0, r0, #(1 :SHL: 22)        ; Set the U bit to enable unaligned access
        ;orr        r0, r0, #(1 :SHL: 3)        ; Write buffer wnable
        mcr        p15, 0, r0, c1, c0, 0
    ]

;------------------------------------
;    Jump to KernelStart
;------------------------------------

        add        r0, pc, #g_oalAddressTable - (. + 8)
        bl        KernelStart
        b        .                    ; Should not be here...

        ENTRY_END

;------------------------------------------------------------------------------
;    End of ResetHandler
;------------------------------------------------------------------------------

;------------------------------------------------------------------------------
;    Place OemAddressTable here, This area is Text area
;------------------------------------------------------------------------------
        INCLUDE    oemaddrtab_cfg.inc        


;------------------------------------------------------------------------------
;
;    OALCPUPowerOff Function
;
;    S3C6410 Sleep mode entering function
;
;------------------------------------------------------------------------------

    LEAF_ENTRY    OALCPUPowerOff

;------------------------------------
;    1. Push SVC Register into our Stack
;------------------------------------

        stmdb    sp!, {r4-r12}
        stmdb    sp!, {lr}

;------------------------------------------------
;    2. Save CP15 Register into Sleep Data Area in DRAM
;------------------------------------------------

        ldr        r3, =IMAGE_SLEEP_DATA_UA_START    ; Sleep Data Area Base Address

        ;----------------------
        ; WakeUp Routine Address

        ldr        r2, =WakeUp_Address        ; Virtual Address of WakeUp Routine
        str        r2, [r3], #4                ; [SleepState_WakeAddr]

        ;--------------------------
        ; CP15 System Control Register

        mrc        p15, 0, r2, c1, c0, 0        ; load r2 with System Control Register
        ldr        r0, =SYSCTL_SBZ_MASK    ; Should Be Zero Mask for System Control Register
        bic        r2, r2, r0
        ldr        r0, =SYSCTL_SBO_MASK    ; Should Be One Mask for System Control Register
        orr        r2, r2, r0
        str        r2, [r3], #4                ; [SleepState_SYSCTL]

        ;---------------------------------
        ; CP15 Translation Table Base Register0

        mrc         p15, 0, r2, c2, c0, 0        ; load r2 with TTB Register0
        ldr        r0, =MMUTTB_SBZ_MASK    ; Should Be Zero Mask for TTB Register0
        bic        r2, r2, r0
        str        r2, [r3], #4                ; [SleepState_MMUTTB0]

        ;---------------------------------
        ; CP15 Translation Table Base Register1

        mrc        p15, 0, r2, c2, c0, 1        ; load r2 with TTB Register1
        str        r2, [r3], #4                ; [SleepState_MMUTTB1]

        ;---------------------------------------
        ; CP15 Translation Table Base Control Register

        mrc         p15, 0, r2, c2, c0, 2        ; load r2 with TTB Control Register
        str        r2, [r3], #4                ; [SleepState_MMUTTBCTL]

        ;---------------------------------
        ; CP15 Domain Access Control Register

        mrc        p15, 0, r2, c3, c0, 0        ; load r2 with Domain Access Control Register
        str        r2, [r3], #4                ; [SleepState_MMUDOMAIN]

;-----------------------------------------------
;    3. Save CPU Register into Sleep Data Area in DRAM
;-----------------------------------------------

        ;---------------------------
        ; Supervisor mode CPU Register

        str        sp, [r3], #4                ; [SleepState_SVC_SP]

        mrs        r2, spsr                    ; Status Register
        str        r2, [r3], #4                ; [SleepState_SVC_SPSR]

        ;----------------------
        ; FIQ mode CPU Registers

        mov        r1, #Mode_FIQ | NOINT        ; Enter FIQ mode, no interrupts
        msr        cpsr, r1
        mrs        r2, spsr                    ; Status Register
        stmia    r3!, {r2, r8-r12, sp, lr}        ; Store FIQ mode registers [SleepState_FIQ_SPSR~SleepState_FIQ_LR]

        ;----------------------
        ; Abort mode CPU Registers

        mov        r1, #Mode_ABT | NOINT    ; Enter ABT mode, no interrupts
        msr        cpsr, r1
        mrs        r0, spsr                    ; Status Register
        stmia    r3!, {r0, sp, lr}            ; Store ABT mode Registers [SleepState_ABT_SPSR~SleepState_ABT_LR]

        ;----------------------
        ; IRQ mode CPU Registers

        mov        r1, #Mode_IRQ | NOINT    ; Enter IRQ mode, no interrupts
        msr        cpsr, r1
        mrs        r0, spsr                    ; Status Register
        stmia    r3!, {r0, sp, lr}            ; Store the IRQ Mode Registers [SleepState_IRQ_SPSR~SleepState_IRQ_LR]

        ;---------------------------
        ; Undefined mode CPU Registers

        mov        r1, #Mode_UND | NOINT    ; Enter UND mode, no interrupts
        msr        cpsr, r1
        mrs        r0, spsr                    ; Status Register
        stmia    r3!, {r0, sp, lr}            ; Store the UND mode Registers [SleepState_UND_SPSR~SleepState_UND_LR]

        ;------------------------------
        ; System(User) mode CPU Registers

        mov        r1, #Mode_SYS | NOINT    ; Enter SYS mode, no interrupts
        msr        cpsr, r1
        stmia    r3!, {sp, lr}                ; Store the SYS mode Registers [SleepState_SYS_SP, SleepState_SYS_LR]

        ;----------------------------------------------------
        ;Add following : SISO added
        ; 3-1. Save VFP Register into Sleep Data Area in DRAM
        ;----------------------------------------------------

        ;--------------------------------------
        ;    Floating Point Status and Control Register using FMRX
        ;    FMRX{cond} Rd, VFPsysreg     VFPsysreg -> Rd
        ; FMXR{cond} VFPsysreg, Rd         Rd -> VFPsysreg
        ;      FPSCR
        fmrx    r2, fpscr
        str    r2, [r3], #4     ;    [SleepState_VFP_FPSCR]

        ;------------------------------------------
        ;    Floating Point Exception Register
        fmrx    r2, fpexc
        str    r2, [r3], #4       ;    [SleepState_VFP_FPEXC]

        ;-----------------------------------------
        ;VFP Register File (using FLDMX, FSTMX)
        ; FLDM<addressmode><precision>{cond} Rn,{!} VFPregisters
        ;FSTM<addressmode><precision>{cond} Rn,{!} VFPregisters
        ; <addressmode>
        ;IA : Incremental address After each transfer
        ;DB : Decremental address Before each transfer
        ;EA : Empty Ascending stack operation, this is the same as DB for loads, and the same as IA for saves.
        ;FD : Full Descending stack operation, this is the same as IA for loads, and the same as DB for saves.
        ; <precision>
        ;      S : for single-precision
        ;        D : for double-precision
        ;        X : for unspecified precision
        ;    example::
        ;        FLDMIAS    r2, {s1-s5}
        ;        FSTMFDD    r13!, {d3-d6}
        ;        FSTMFDX    r13!, {d0-d3}
        ;        FLDMFDX    r13!, {d0-d3}
        fstmiax    r3!,    {d0-d15}

        ;------------------------------
        ; Return to SVC mode

        mov        r1, #Mode_SVC | NOINT    ; Back to SVC mode, no interrupts
        msr        cpsr, r1

;-----------------------------------------------------
;    4. Calculate CheckSum of Sleep Data
;-----------------------------------------------------

        ldr        r3, =IMAGE_SLEEP_DATA_UA_START    ; Base of Sleep Data Area
        ldr        r2, =0x0
        ldr        r0, =(SLEEPDATA_SIZE-1)            ; Size of Sleep Data Area (in words)

CheckSum_Loop

        ldr        r1, [r3], #4
        and        r1, r1, #0x1
        mov        r1, r1, LSL #31
        orr        r1, r1, r1, LSR #1
        add        r2, r2, r1
        subs        r0, r0, #1
        bne        CheckSum_Loop

        ldr        r0, =vINFORM1
        str        r2, [r0]                            ; Store CheckSum in INFORM1 Register (in SysCon)

;-----------------------------------------------------
;    5. Clear TLB and Flush Cache
;-----------------------------------------------------

        bl        OALClearDTLB
        bl        OALClearITLB
        bl        OALFlushDCache
        bl        OALFlushICache

;-----------------------------------------------------
;    6. Set Oscillation pad and Power Stable Counter
;-----------------------------------------------------

        ldr     r0, =vOSC_STABLE
        ldr     r1, =0x1
        str     r1, [r0]

        ldr     r0, =vPWR_STABLE
        ldr     r1, =0x1
        str     r1, [r0]

;-----------------------------------------------------
;    7. Set Power Mode to Sleep
;-----------------------------------------------------


        ldr        r0, =vPWR_CFG
        ldr        r2, [r0]
        bic        r2, r2, #0x60            ; Clear STANDBYWFI
        orr        r2, r2, #0x60            ; Enter SLEEP mode
        str        r2, [r0]

        ldr        r0, =vSLEEP_CFG
        ldr        r2, [r0]
        bic        r2, r2, #0x61            ; Disable OSC_EN (Disable X-tal Osc Pad in Sleep mode)
        str        r2, [r0]

;-----------------------------------------------------
;    8. Set Power Mode to Sleep
;-----------------------------------------------------

        bl        System_WaitForInterrupt
        b        .

;------------------------------------------------------------------------------
;    Now CPU is in Sleep Mode
;------------------------------------------------------------------------------

WakeUp_Address

;-----------------------------------------------------
;    1. Restore CPU Register from Sleep Data Area in DRAM
;-----------------------------------------------------


        ldr        r3, =IMAGE_SLEEP_DATA_UA_START    ; Sleep Data Area Base Address

        ;----------------------
        ; FIQ mode CPU Registers

        mov        r1, #Mode_FIQ | NOINT                ; Enter FIQ mode, no interrupts
        msr        cpsr, r1

        ldr        r0,    [r3, #SleepState_FIQ_SPSR]
        msr        spsr, r0
        ldr        r8,    [r3, #SleepState_FIQ_R8]
        ldr        r9,    [r3, #SleepState_FIQ_R9]
        ldr        r10,    [r3, #SleepState_FIQ_R10]
        ldr        r11,    [r3, #SleepState_FIQ_R11]
        ldr        r12,    [r3, #SleepState_FIQ_R12]
        ldr        sp,    [r3, #SleepState_FIQ_SP]
        ldr        lr,    [r3, #SleepState_FIQ_LR]

        ;-----------------------
        ; Abort mode CPU Registers

        mov        r1, #Mode_ABT | I_Bit                ; Enter ABT mode, no IRQ - FIQ is available
        msr        cpsr, r1

        ldr        r0,    [r3, #SleepState_ABT_SPSR]
        msr        spsr, r0
        ldr        sp,    [r3, #SleepState_ABT_SP]
        ldr        lr,    [r3, #SleepState_ABT_LR]

        ;----------------------
        ; IRQ mode CPU Registers

        mov        r1, #Mode_IRQ | I_Bit                ; Enter IRQ mode, no IRQ - FIQ is available
        msr        cpsr, r1

        ldr        r0,    [r3, #SleepState_IRQ_SPSR]
        msr        spsr, r0
        ldr        sp,    [r3, #SleepState_IRQ_SP]
        ldr        lr,    [r3, #SleepState_IRQ_LR]

        ;---------------------------
        ; Undefined mode CPU Registers

        mov        r1, #Mode_UND | I_Bit                ; Enter UND mode, no IRQ - FIQ is available
        msr        cpsr, r1

        ldr        r0,    [r3, #SleepState_UND_SPSR]
        msr        spsr, r0
        ldr        sp,    [r3, #SleepState_UND_SP]
        ldr        lr,    [r3, #SleepState_UND_LR]

        ;------------------------------
        ; System(User) mode CPU Registers

        mov        r1, #Mode_SYS | I_Bit                ; Enter SYS mode, no IRQ - FIQ is available
        msr        cpsr, r1

        ldr        sp,    [r3, #SleepState_SYS_SP]
        ldr        lr,    [r3, #SleepState_SYS_LR]

        ;----------------------------
        ; Supervisor mode CPU Registers

        mov        r1, #Mode_SVC | I_Bit                ; Enter SVC mode, no IRQ - FIQ is available
        msr        cpsr, r1

        ldr        r0, [r3, #SleepState_SVC_SPSR]
        msr        spsr, r0
        ldr        sp, [r3, #SleepState_SVC_SP]

        ;----------------------------------------
             ; Add following: SISO added
           ; 1-1 Restore VFP system control registers
           ;----------------------------------------

        ;--------------------------------------
        ;FMRX{cond} Rd, VFPsysreg     VFPsysreg -> Rd
        ; FMXR{cond} VFPsysreg, Rd         Rd -> VFPsysreg
        ;      FPSCR
        ldr        r0, [r3, #SleepState_VFP_FPSCR]
        fmxr    fpscr, r0

        ;------------------------------------------
        ;    Floating Point Exception Register
        ldr        r0, [r3, #SleepState_VFP_FPEXC]
        fmxr    fpexc, r0

;----------------------------------
;    2. Pop SVC Register from our Stack
;----------------------------------

        ldr        lr, [sp], #4
        ldmia    sp!, {r4-r12}

;--------------------------------------
;    3. Return to Caller of OALCPUPowerOff()
;--------------------------------------

        mov     pc, lr                          ; and now back to our sponsors

        ENTRY_END


;------------------------------------------------------------------------------
;    End of OALCPUPowerOff
;------------------------------------------------------------------------------

        END

