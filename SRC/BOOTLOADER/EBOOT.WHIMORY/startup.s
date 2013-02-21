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

        INCLUDE     kxarm.h
        INCLUDE     s3c6410.inc
        INCLUDE     image_cfg.inc

        IMPORT      OALClearUTLB
        IMPORT      OALFlushICache
        IMPORT      OALFlushDCache

        IMPORT      System_DisableVIC
        IMPORT      System_EnableIRQ
        IMPORT      System_SetSyncMode
        IMPORT      System_SetAsyncMode
        IMPORT      System_EnableICache

;-------------------------------------------------------------------------------
;     Definition for MMU table initialization
;-------------------------------------------------------------------------------

PT_1ST_BASE         EQU     (DRAM_BASE_PA_START+0x10000)        ; 1st level Page Table Base Address (PHYBASE + 0x10000) save room for interrupt vectors
PT_1ST_ENTRY_CNB    EQU     (DRAM_BASE_PA_START+0x40E)          ; Cached Area Page Table Entry (Cache/Unbuffer/RW), PA base = 0x50000000
PT_1ST_ENTRY_NCNB   EQU     (DRAM_BASE_PA_START+0x402)          ; Uncached Area Page Table Entry (Uncache/Unbuffer/RW), PA base = 0x50000000
PTR_1ST_PTE         EQU     ((DRAM_BASE_PA_START>>16)/4)        ; Ptr to 1st PTE for 0x50000000
BANK_SHIFT          EQU     (20)

;------------------------------------------------------------------------------
;
;     Macro for LED on SMDK Board (GPN[15:12])
;
;     LED_ON for physical address domain
;     VLED_ON for virtual address domain
;
;------------------------------------------------------------------------------

    MACRO

        LED_ON     $data

        ldr          r10, =GPNPUD
        ldr          r11, [r10]
        bic          r11, r11, #0xFF000000     ; Pull-Up-Down Disable
        str          r11, [r10]

        ldr          r10, =GPNDAT
        ldr          r11, [r10]
        bic          r11, r11, #0xF000
        ldr          r12, =$data
        mov          r12, r12, lsl #12     ; [15:12]
        orr          r11, r11, r12
        str          r11, [r10]

        ldr          r10, =GPNCON
        ldr          r11, [r10]
        bic          r11, r11, #0xFF000000
        orr          r11, r11, #0x55000000     ; GPN[15:12] Output .
        str          r11, [r10]

    MEND


    MACRO

        VLED_ON     $data

        ldr          r10, =vGPNPUD
        ldr          r11, [r10]
        bic          r11, r11, #0xFF000000     ; Pull-Up-Down Disable
        str          r11, [r10]

        ldr          r10, =vGPNDAT
        ldr          r11, [r10]
        bic          r11, r11, #0xF000
        ldr          r12, =$data
        mov          r12, r12, lsl #12     ; [15:12]
        orr          r11, r11, r12
        str          r11, [r10]

        ldr          r10, =vGPNCON
        ldr          r11, [r10]
        bic          r11, r11, #0xFF000000
        orr          r11, r11, #0x55000000     ; GPN[15:12] Output .
        str          r11, [r10]

    MEND

;------------------------------------------------------------------------------
;     End of Macro
;------------------------------------------------------------------------------

        TEXTAREA

        IMPORT  main

        INCLUDE     oemaddrtab_cfg.inc

;------------------------------------------------------------------------------
;
;     StartUp Entry
;
;     Main entry point for CPU initialization.
;
;------------------------------------------------------------------------------
    LEAF_ENTRY      StartUp

        b          ResetHandler
        b          .                    ; HandlerUndef
        b          .                    ; HandlerSWI
        b          .                    ; HandlerPabort
        b          .                    ; HandlerDabort
        b          .                    ; HandlerReserved
        b          .                    ; HandlerIRQ
        b          .                    ; HandlerFIQ

;------------------------------------------------------------------------------
;
;     ResetHandler Function
;
;     Reset Exception Handler
;
;------------------------------------------------------------------------------

ResetHandler

        LED_ON 0x1

;------------------------------------
;     Enable Instruction Cache
;------------------------------------

        mov         r0, #0
        mcr         p15, 0, r0, c7, c7, 0   ; Invalidate Entire I&D Cache
        bl          System_EnableICache     ; Enable I Cache

;------------------------------------
;     Peripheral Port Setup
;------------------------------------

        ldr         r0, =0x70000013         ; Base Addres : 0x70000000, Size : 256 MB (0x13)
        mcr         p15,0,r0,c15,c2,4

;------------------------------------
;     Interrupt Disable
;------------------------------------

        ldr         r0, =VIC0INTENCLEAR
        ldr         r1, =0xFFFFFFFF;
        str         r1, [r0]

        ldr         r0, =VIC1INTENCLEAR
        ldr         r1, =0xFFFFFFFF;
        str         r1, [r0]

;------------------------------------
;     Disable WatchDog Timer
;------------------------------------

        ldr         r0, =WTCON
        ldr         r1, =0x0
        str         r1, [r0]

;   CLKDIV & PLL Change code was here.


;------------------------------------
;     Expand Memory Port 1 to x32
;------------------------------------

        ldr          r0, =MEM_SYS_CFG
        ldr          r1, [r0]
        bic          r1, r1, #0x80               ; ADDR_EXPAND to "0"
        str          r1, [r0]

;------------------------------------
;     Disable VIC
;------------------------------------

        bl          System_DisableVIC

;------------------------------------
;     Enable IRQ
;------------------------------------

        bl          System_EnableIRQ

;------------------------------------
;     Clear DRAM
;------------------------------------

    [ CLEAR_DRAM_ON_EBOOT

        mov          r1, #0
        mov      r2, #0
        mov      r3, #0
        mov      r4, #0
        mov      r5, #0
        mov      r6, #0
        mov      r7, #0
        mov      r8, #0

        ldr          r0, =IMAGE_NK_PA_START                    ; Start address (Physical 0x5010.0000)
        ldr          r9, =(DRAM_SIZE-IMAGE_NK_OFFSET)     ; 127 MB of RAM (1MB + 127MB)
10
        stmia     r0!, {r1-r8}
        subs          r9, r9, #32
        bne          %B10
    ]

;------------------------------------
;     Initialize MMU Table
;------------------------------------

    ;----------------------------
    ; Compute physical address of the OEMAddressTable.

20
        add          r11, pc, #g_oalAddressTable -(. + 8)
        ldr          r10, =PT_1ST_BASE                    ; (r10) = 1st level page table

    ;----------------------------
    ; Setup 1st level page table (using section descriptor)
    ; Fill in first level page table entries to create "un-mapped" regions
    ; from the contents of the MemoryMap array.
    ;
    ; (r10) = 1st level page table
    ; (r11) = ptr to MemoryMap array

        add          r10, r10, #0x2000          ; (r10) = ptr to 1st PTE for "unmapped space"
        mov          r0, #0x0E               ; (r0) = PTE for 0: 1MB cachable bufferable
        orr          r0, r0, #0x400          ; set kernel r/w permission
25
        mov          r1, r11                    ; (r1) = ptr to MemoryMap array

30
        ldr          r2, [r1], #4               ; (r2) = virtual address to map Bank at
        ldr          r3, [r1], #4               ; (r3) = physical address to map from
        ldr          r4, [r1], #4               ; (r4) = num MB to map

        cmp          r4, #0                    ; End of table?
        beq          %F40

        ldr          r5, =0x1FF00000
        and          r2, r2, r5                    ; VA needs 512MB, 1MB aligned.

        ldr          r5, =0xFFF00000
        and          r3, r3, r5                    ; PA needs 4GB, 1MB aligned.

        add          r2, r10, r2, LSR #18
        add          r0, r0, r3                    ; (r0) = PTE for next physical page

35
        str          r0, [r2], #4
        add          r0, r0, #0x00100000     ; (r0) = PTE for next physical page
        sub          r4, r4, #1               ; Decrement number of MB left
        cmp          r4, #0
        bne          %B35                    ; Map next MB

        bic          r0, r0, #0xF0000000     ; Clear Section Base Address Field
        bic          r0, r0, #0x0FF00000     ; Clear Section Base Address Field
        b          %B30                    ; Get next element

40
        tst          r0, #8
        bic          r0, r0, #0x0C               ; clear cachable & bufferable bits in PTE
        add          r10, r10, #0x0800          ; (r10) = ptr to 1st PTE for "unmapped uncached space"
        bne          %B25                    ; go setup PTEs for uncached space
        sub          r10, r10, #0x3000          ; (r10) = restore address of 1st level page table

    ;----------------------------------------------
    ; Setup mmu to map (VA == 0) to (PA == 0x30000000).

        ; cached area
        ldr          r0, =PT_1ST_BASE               ; PTE entry for VA = 0
        ldr          r1, =PT_1ST_ENTRY_CNB     ; Cache/Unbuffer/RW
        str          r1, [r0]

        ; uncached area.
        add          r0, r0, #0x0800               ; PTE entry for VA = 0x02000000
        ldr          r1, =PT_1ST_ENTRY_NCNB     ; Uncache/Unbuffer/RW
        str          r1, [r0]

        ; Comment:
        ; The following loop is to direct map RAM VA == PA. i.e.
        ;   VA == 0x50XXXXXX => PA == 0x50XXXXXX for S3C6410
        ; Fill in 8 entries to have a direct mapping for DRAM

        ldr          r10, =PT_1ST_BASE          ; Restore address of 1st level page table
        ldr          r0,  =DRAM_BASE_PA_START

        add          r10, r10, #PTR_1ST_PTE     ; (r10) = ptr to 1st PTE for 0x50000000

        add          r0, r0, #0x1E               ; 1MB cachable bufferable
        orr          r0, r0, #0x400          ; set kernel r/w permission
        mov          r1, #0
;          mov          r3, #64                    ; 64MB DRAM
	[   BSP_TYPE = BSP_DRAM128
        mov          r3, #128                    ; 128MB DRAM
	]
	[ BSP_TYPE = BSP_DRAM256
        mov          r3, #256                    ; 256MB DRAM
       ]   
45
        mov          r2, r1                    ; (r2) = virtual address to map Bank at
        cmp          r2, #0x20000000:SHR:BANK_SHIFT
        add          r2, r10, r2, LSL #BANK_SHIFT-18
        strlo          r0, [r2]
        add          r0, r0, #0x00100000     ; (r0) = PTE for next physical page
        subs          r3, r3, #1
        add          r1, r1, #1
        bgt          %B45

        ldr          r10, =PT_1ST_BASE     ; (r10) = restore address of 1st level page table

        ; The page tables and exception vectors are setup.
        ; Initialize the MMU and turn it on.
        mov          r1, #1
        mcr          p15, 0, r1, c3, c0, 0     ; setup access to domain 0
        mcr          p15, 0, r10, c2, c0, 0

        mcr          p15, 0, r0, c8, c7, 0     ; flush I+D TLBs

        mrc          p15, 0, r1, c1, c0, 0
        orr          r1, r1, #0x0071          ; Enable MMU
        orr          r1, r1, #0x0004          ; Enable the Data Cache

        ldr          r0, =VirtualStart

        cmp          r0, #0                    ; make sure no stall on "mov pc,r0" below
        mcr          p15, 0, r1, c1, c0, 0
        mov          pc, r0                    ; & jump to new virtual address
        nop

        ; MMU & caches now enabled.
        ;   (r10) = physcial address of 1st level page table

;-----------------------------------------------
;     MMU Enabled and Virtual Address is Valid from here
;-----------------------------------------------

VirtualStart

;--------------------------------------------------
;     Initialize Stack
;     Stack size and location information is in "image_cfg.inc"
;--------------------------------------------------

        mrs          r0, cpsr

        bic          r0, r0, #Mode_MASK
        orr          r1, r0, #Mode_IRQ | NOINT
        msr          cpsr_cxsf, r1                    ; IRQMode
        ldr          sp, =IRQStack_VA               ; IRQStack

        bic          r0, r0, #Mode_MASK | NOINT
        orr          r1, r0, #Mode_SVC
        msr          cpsr_cxsf, r1                    ; SVCMode
        ldr          sp, =SVCStack_VA               ; SVCStack

;------------------------------------
;     Jump to Main() "C" Routine
;------------------------------------

        b          main
        b          .          ; Should no be here...

    ENTRY_END

;------------------------------------------------------------------------------
;     End of StartUp
;------------------------------------------------------------------------------


;------------------------------------------------------------------------------
;
;     Launch Function
;
;     Launch OS Image from DRAM
;
;------------------------------------------------------------------------------

    LEAF_ENTRY     Launch

        ldr          r2, = PhysicalStart
        ldr          r3, =(DRAM_BASE_CA_START - DRAM_BASE_PA_START)

        sub          r2, r2, r3                    ; Calculate Address Offset between Virtual and Physical

        mov          r1, #0
        mcr          p15, 0, r1, c7, c5, 0     ; Invalidate Entire Instruction Cache
        mcr          p15, 0, r1, c7, c14, 0     ; Clean and Invalidate Entire Data Cache

        mrc          p15, 0, r1, c1, c0, 0
        bic          r1, r1, #0x0005          ; Disable MMU and Data Cache
        mcr          p15, 0, r1, c1, c0, 0

        nop
        mov          pc, r2                    ; Jump to PStart
        nop

        ; MMU & caches now disabled.

PhysicalStart

        mov          r1, #0
        mcr          p15, 0, r1, c8, c7, 0     ; Flush the TLB

        mov          pc, r0                    ; Jump to program we are launching.

    ENTRY_END


    LEAF_ENTRY      ChangeCLKDIV

        [ {FALSE}   ;CHANGE_PLL_CLKDIV_ON_EBOOT

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
;     Check PLL and CLKDIV
;---------------------------------------

        ldr          r3, =0x83FF3F07          ; Mask for APLL_CON/MPLL_CON
        ldr          r4, =0x80FF3F07          ; Mask for EPLL_CON0
        ldr          r5, =0x0000FFFF          ; Mask for EPLL_CON1
        ldr          r6, =0x0003FF17          ; Mask for CLKDIV0

        ldr          r0, =APLL_CON          ; Check APLL
        ldr          r1, [r0]
        and          r1, r1, r3
        ldr          r2, =((1<<31)+(Startup_APLL_MVAL<<16)+(Startup_APLL_PVAL<<8)+(Startup_APLL_SVAL))     ; APLL_CON value to configure
        cmp          r1, r2
        bne          PLL_NeedToConfigure

        ldr          r0, =MPLL_CON          ; Check MPLL
        ldr          r1, [r0]
        and          r1, r1, r3
        ldr          r2, =((1<<31)+(MPLL_MVAL<<16)+(MPLL_PVAL<<8)+(MPLL_SVAL))     ; MPLL_CON value to configure
        cmp          r1, r2
        bne          PLL_NeedToConfigure

        ldr          r0, =EPLL_CON0          ; Check EPLL_CON0
        ldr          r1, [r0]
        and          r1, r1, r4
        ldr          r2, =((1<<31)+(EPLL_MVAL<<16)+(EPLL_PVAL<<8)+(EPLL_SVAL))          ; EPLL_CON0 value to configure
        cmp          r1, r2
        bne          PLL_NeedToConfigure

        ldr          r0, =EPLL_CON1          ; Check EPLL_CON1
        ldr          r1, [r0]
        and          r1, r1, r5
        ldr          r2, =EPLL_KVAL          ; EPLL_CON1 value to configure
        cmp          r1, r2
        bne          PLL_NeedToConfigure

        ldr          r0, =CLK_DIV0          ; Check CLKDIV0
        ldr          r1, [r0]
        and          r1, r1, r6
        
        ldr          r2, =((Startup_PCLK_DIV<<12)+(Startup_HCLKx2_DIV<<9)+(Startup_HCLK_DIV<<8)+(MPLL_DIV<<4)+(Startup_APLL_DIV<<0))          ; CLKDIV0 value to configure

        cmp          r1, r2
        bne          CLKDIV_NeedToConfigure

        b          PLL_CLKDIV_AlreadyConfigured     ; APLL/MPLL/EPLL and CLKDIV0 is already configured

;------------------------------------
;     Prepare to Change PLL
;------------------------------------

PLL_NeedToConfigure

;------------------------------------
;     Disable PLL Clock Out
;------------------------------------

        ldr          r0, =CLK_SRC
        ldr          r1, [r0]
        bic          r1, r1, #0x7               ; FIN out
        str          r1, [r0]                    

        ldr          r0, =CLK_DIV0
        ldr          r1, [r0]
        bic          r1, r1, #0xff00
        bic          r1, r1, #0xff
        ldr        r2, = ((Startup_PCLK_DIV<<12)+(Startup_HCLKx2_DIV<<9)+(Startup_HCLK_DIV<<8)+(MPLL_DIV<<4)+(Startup_APLL_DIV<<0))
        orr        r1, r1, r2
        str        r1, [r0]


;------------------------------------
;     Change PLL Value
;------------------------------------

        ldr          r1, =0x4B1               ; Lock Time : 0x4b1 (100us @Fin12MHz) for APLL/MPLL
        ldr          r2, =0xE13               ; Lock Time : 0xe13 (300us @Fin12MHz) for EPLL

        ldr          r0, =APLL_LOCK
        str          r1, [r0]                    ; APLL Lock Time
        str          r1, [r0, #0x4]               ; MPLL Lock Time
        str          r2, [r0, #0x8]               ; EPLL Lock Time

        ldr          r0, =APLL_CON
        ldr          r1, =((1<<31)+(Startup_APLL_MVAL<<16)+(Startup_APLL_PVAL<<8)+(Startup_APLL_SVAL))
        str          r1, [r0]

        ldr          r0, =MPLL_CON
        ldr          r1, =((1<<31)+(MPLL_MVAL<<16)+(MPLL_PVAL<<8)+(MPLL_SVAL))
        str          r1, [r0]

        ldr          r0, =EPLL_CON1
        ldr          r1, =EPLL_KVAL
        str          r1, [r0]

        ldr          r0, =EPLL_CON0
        ldr          r1, =((1<<31)+(EPLL_MVAL<<16)+(EPLL_PVAL<<8)+(EPLL_SVAL))
        str          r1, [r0]

;------------------------------------
;     Set System Clock Divider
;------------------------------------

CLKDIV_NeedToConfigure

        ldr          r0, =CLK_DIV0
        ldr          r1, [r0]
        bic          r1, r1, #0x30000
        bic          r1, r1, #0xff00
        bic          r1, r1, #0xff
        ldr          r2, =((Startup_PCLK_DIV<<12)+(Startup_HCLKx2_DIV<<9)+(Startup_HCLK_DIV<<8)+(MPLL_DIV<<4)+(Startup_APLL_DIV<<0))
        orr          r1, r1, r2
        str          r1, [r0]

;------------------------------------
;     Enable PLL Clock Out
;------------------------------------

        ldr          r0, =CLK_SRC
        ldr          r1, [r0]
        orr          r1, r1, #0x7               ; PLL  Clockout
        str          r1, [r0]                    ; System will be waiting for PLL unlocked after this instruction

PLL_CLKDIV_AlreadyConfigured

    ]     ; CHANGE_PLL_CLKDIV_ON_EBOOT

    ENTRY_END

;------------------------------------------------------------------------------
;     End of Launch
;------------------------------------------------------------------------------

    END

