;
; Copyright (c) Microsoft Corporation.  All rights reserved.
;
; Use of this sample source code is subject to the terms of the 
; Software License Agreement (SLA) under which you licensed this software product.
; If you did not accept the terms of the license agreement, 
; you are not authorized to use this sample source code. 
; THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
;
;//------------------------------------------------------------------------------
;//  NAMING CONVENTION
;//
;//  The IMAGE_ naming convention ...
;//
;//  IMAGE_<NAME>_<SECTION>_<MEMORY_DEVICE>_[OFFSET|SIZE|START|END]
;//
;//      <NAME>          - WINCE, BOOT, SHARE
;//      <SECTION>       - section name: user defined
;//      <MEMORY_DEVICE> - the memory device the block resides on
;//      OFFSET          - number of bytes from memory device start address
;//      SIZE            - maximum size of the block
;//      START           - start address of block    (device address + offset)
;//      END             - end address of block      (start address  + size - 1)
;//
;//------------------------------------------------------------------------------

;// DRAM Base Address
DRAM_BASE_PA_START      EQU     (0x50000000)
DRAM_BASE_CA_START      EQU     (0x80000000)
DRAM_BASE_UA_START      EQU     (0xA0000000)
        [ BSP_TYPE=BSP_DRAM256
DRAM_SIZE               EQU     (0x10000000)
        |
DRAM_SIZE               EQU     (0x08000000)
        ]
;//------------------------------------------------------------------------------

;// Steploader Area
IMAGE_STEPLOADER_PA_START    EQU    (0x00000000)
IMAGE_STEPLOADER_SIZE        EQU    (0x00002000)

;//------------------------------------------------------------------------------

;// Eboot Area
IMAGE_EBOOT_OFFSET          EQU    (0x00030000)
IMAGE_EBOOT_PA_START        EQU    (DRAM_BASE_PA_START+IMAGE_EBOOT_OFFSET)
IMAGE_EBOOT_CA_START        EQU    (DRAM_BASE_CA_START+IMAGE_EBOOT_OFFSET)
IMAGE_EBOOT_UA_START        EQU    (DRAM_BASE_UA_START+IMAGE_EBOOT_OFFSET)
IMAGE_EBOOT_SIZE            EQU    (0x00080000)

EBOOT_BINFS_BUFFER_OFFSET       EQU    (0x000C0000)
EBOOT_BINFS_BUFFER_PA_START     EQU    (DRAM_BASE_PA_START+EBOOT_BINFS_BUFFER_OFFSET)
EBOOT_BINFS_BUFFER_CA_START     EQU    (DRAM_BASE_CA_START+EBOOT_BINFS_BUFFER_OFFSET)
EBOOT_BINFS_BUFFER_UA_START     EQU    (DRAM_BASE_UA_START+EBOOT_BINFS_BUFFER_OFFSET)
EBOOT_BINFS_BUFFER_SIZE         EQU    (0x00030000)
EBOOT_USB_BUFFER_OFFSET         EQU    (0x03000000)
EBOOT_USB_BUFFER_PA_START       EQU    (DRAM_BASE_PA_START+EBOOT_USB_BUFFER_OFFSET)
EBOOT_USB_BUFFER_CA_START       EQU    (DRAM_BASE_CA_START+EBOOT_USB_BUFFER_OFFSET)
EBOOT_USB_BUFFER_UA_START       EQU    (DRAM_BASE_UA_START+EBOOT_USB_BUFFER_OFFSET)

;//------------------------------------------------------------------------------

;// NK Area
IMAGE_NK_OFFSET             EQU    (0x00100000)
IMAGE_NK_PA_START           EQU    (DRAM_BASE_PA_START+IMAGE_NK_OFFSET)
IMAGE_NK_CA_START           EQU    (DRAM_BASE_CA_START+IMAGE_NK_OFFSET)
IMAGE_NK_UA_START           EQU    (DRAM_BASE_UA_START+IMAGE_NK_OFFSET)
IMAGE_NK_SIZE               EQU    (0x04F00000)  ; Set Max size, This will be tailored automatically.

;//------------------------------------------------------------------------------

;// BSP ARGs Area
IMAGE_SHARE_ARGS_OFFSET      EQU    (0x00020800)
IMAGE_SHARE_ARGS_PA_START    EQU    (DRAM_BASE_PA_START+IMAGE_SHARE_ARGS_OFFSET)
IMAGE_SHARE_ARGS_CA_START    EQU    (DRAM_BASE_CA_START+IMAGE_SHARE_ARGS_OFFSET)
IMAGE_SHARE_ARGS_UA_START    EQU    (DRAM_BASE_UA_START+IMAGE_SHARE_ARGS_OFFSET)
IMAGE_SHARE_ARGS_SIZE        EQU    (0x00000800)

;//------------------------------------------------------------------------------

;// Sleep Data Area
IMAGE_SLEEP_DATA_OFFSET      EQU    (0x00028000)
IMAGE_SLEEP_DATA_PA_START    EQU    (DRAM_BASE_PA_START+IMAGE_SLEEP_DATA_OFFSET)
IMAGE_SLEEP_DATA_CA_START    EQU    (DRAM_BASE_CA_START+IMAGE_SLEEP_DATA_OFFSET)
IMAGE_SLEEP_DATA_UA_START    EQU    (DRAM_BASE_UA_START+IMAGE_SLEEP_DATA_OFFSET)
IMAGE_SLEEP_DATA_SIZE        EQU    (0x00002000)

;//------------------------------------------------------------------------------


;------------------------------------------------------------------------------
;    OEM Stack Layout
; EBOOT, STEPLOADER also use this value
;------------------------------------------------------------------------------

TOP_OF_STACKS_PHYSICAL          EQU    (DRAM_BASE_PA_START+IMAGE_NK_OFFSET)    ; Stack Top is in front of NK Image
TOP_OF_STACKS_VIRTUAL           EQU    (DRAM_BASE_CA_START+IMAGE_NK_OFFSET)

; Stack Size of Each Mode
FIQStackSize    EQU    256
IRQStackSize    EQU    256
AbortStackSize  EQU    256
UndefStackSize  EQU    256
SVCStackSize    EQU    1024
;UserStackSize   EQU    2048

; Stack Location of Each Mode (in Physical Address)
FIQStack_PA     EQU    (TOP_OF_STACKS_PHYSICAL    - 0x0)
IRQStack_PA     EQU    (FIQStack_PA        - FIQStackSize)
AbortStack_PA   EQU    (IRQStack_PA        - IRQStackSize)
UndefStack_PA   EQU    (AbortStack_PA        - AbortStackSize)
SVCStack_PA     EQU    (UndefStack_PA        - UndefStackSize)
UserStack_PA    EQU    (SVCStack_PA        - SVCStackSize)

; Stack Location of Each Mode (in Virtual Address)
FIQStack_VA     EQU    (TOP_OF_STACKS_VIRTUAL    - 0x0)
IRQStack_VA     EQU    (FIQStack_VA        - FIQStackSize)
AbortStack_VA   EQU    (IRQStack_VA        - IRQStackSize)
UndefStack_VA   EQU    (AbortStack_VA        - AbortStackSize)
SVCStack_VA     EQU    (UndefStack_VA        - UndefStackSize)
UserStack_VA    EQU    (SVCStack_VA        - SVCStackSize)

;------------------------------------------------------------------------------
;    Sleep Data Layout
;------------------------------------------------------------------------------
SleepState_Data_Start   EQU    (0)
SleepState_WakeAddr     EQU    (SleepState_Data_Start)
SleepState_SYSCTL       EQU    (SleepState_WakeAddr    + WORD_SIZE )
SleepState_MMUTTB0      EQU    (SleepState_SYSCTL    + WORD_SIZE )
SleepState_MMUTTB1      EQU    (SleepState_MMUTTB0    + WORD_SIZE )
SleepState_MMUTTBCTL    EQU    (SleepState_MMUTTB1    + WORD_SIZE )
SleepState_MMUDOMAIN    EQU    (SleepState_MMUTTBCTL    + WORD_SIZE )
SleepState_SVC_SP       EQU    (SleepState_MMUDOMAIN    + WORD_SIZE )
SleepState_SVC_SPSR     EQU    (SleepState_SVC_SP    + WORD_SIZE )
SleepState_FIQ_SPSR     EQU    (SleepState_SVC_SPSR    + WORD_SIZE )
SleepState_FIQ_R8       EQU    (SleepState_FIQ_SPSR    + WORD_SIZE )
SleepState_FIQ_R9       EQU    (SleepState_FIQ_R8    + WORD_SIZE )
SleepState_FIQ_R10      EQU    (SleepState_FIQ_R9    + WORD_SIZE )
SleepState_FIQ_R11      EQU    (SleepState_FIQ_R10    + WORD_SIZE )
SleepState_FIQ_R12      EQU    (SleepState_FIQ_R11    + WORD_SIZE )
SleepState_FIQ_SP       EQU    (SleepState_FIQ_R12    + WORD_SIZE )
SleepState_FIQ_LR       EQU    (SleepState_FIQ_SP    + WORD_SIZE )
SleepState_ABT_SPSR     EQU    (SleepState_FIQ_LR    + WORD_SIZE )
SleepState_ABT_SP       EQU    (SleepState_ABT_SPSR    + WORD_SIZE )
SleepState_ABT_LR       EQU    (SleepState_ABT_SP    + WORD_SIZE )
SleepState_IRQ_SPSR     EQU    (SleepState_ABT_LR    + WORD_SIZE )
SleepState_IRQ_SP       EQU    (SleepState_IRQ_SPSR    + WORD_SIZE )
SleepState_IRQ_LR       EQU    (SleepState_IRQ_SP    + WORD_SIZE )
SleepState_UND_SPSR     EQU    (SleepState_IRQ_LR    + WORD_SIZE )
SleepState_UND_SP       EQU    (SleepState_UND_SPSR    + WORD_SIZE )
SleepState_UND_LR       EQU    (SleepState_UND_SP    + WORD_SIZE )
SleepState_SYS_SP       EQU    (SleepState_UND_LR    + WORD_SIZE )
SleepState_SYS_LR       EQU    (SleepState_SYS_SP    + WORD_SIZE )
SleepState_VFP_FPSCR    EQU    (SleepState_SYS_LR    +    WORD_SIZE)
SleepState_VFP_FPEXC    EQU    (SleepState_VFP_FPSCR    +    WORD_SIZE)
SleepState_VFP_FPINST   EQU    (SleepState_VFP_FPEXC    +    WORD_SIZE)
SleepState_VFP_FPINST2  EQU    (SleepState_VFP_FPINST    +    WORD_SIZE)
SleepState_VFP_S0   EQU        (SleepState_VFP_FPINST2    +    WORD_SIZE)
SleepState_VFP_S1   EQU        (SleepState_VFP_S0    +    WORD_SIZE)
SleepState_VFP_S2   EQU        (SleepState_VFP_S1    +    WORD_SIZE)
SleepState_VFP_S3   EQU        (SleepState_VFP_S2    +    WORD_SIZE)
SleepState_VFP_S4   EQU    (SleepState_VFP_S3    +    WORD_SIZE)
SleepState_VFP_S5   EQU    (SleepState_VFP_S4    +    WORD_SIZE)
SleepState_VFP_S6   EQU    (SleepState_VFP_S5    +    WORD_SIZE)
SleepState_VFP_S7   EQU    (SleepState_VFP_S6    +    WORD_SIZE)
SleepState_VFP_S8   EQU    (SleepState_VFP_S7    +    WORD_SIZE)
SleepState_VFP_S9   EQU    (SleepState_VFP_S8    +    WORD_SIZE)
SleepState_VFP_S10  EQU    (SleepState_VFP_S9    +    WORD_SIZE)
SleepState_VFP_S11  EQU    (SleepState_VFP_S10    +    WORD_SIZE)
SleepState_VFP_S12  EQU    (SleepState_VFP_S11    +    WORD_SIZE)
SleepState_VFP_S13  EQU    (SleepState_VFP_S12    +    WORD_SIZE)
SleepState_VFP_S14    EQU    (SleepState_VFP_S13    +    WORD_SIZE)
SleepState_VFP_S15    EQU    (SleepState_VFP_S14    +    WORD_SIZE)
SleepState_VFP_S16    EQU    (SleepState_VFP_S15    +    WORD_SIZE)
SleepState_VFP_S17    EQU    (SleepState_VFP_S16    +    WORD_SIZE)
SleepState_VFP_S18    EQU    (SleepState_VFP_S17    +    WORD_SIZE)
SleepState_VFP_S19    EQU    (SleepState_VFP_S18    +    WORD_SIZE)
SleepState_VFP_S20    EQU    (SleepState_VFP_S19    +    WORD_SIZE)
SleepState_VFP_S21    EQU    (SleepState_VFP_S20    +    WORD_SIZE)
SleepState_VFP_S22    EQU    (SleepState_VFP_S21    +    WORD_SIZE)
SleepState_VFP_S23    EQU    (SleepState_VFP_S22    +    WORD_SIZE)
SleepState_VFP_S24    EQU    (SleepState_VFP_S23    +    WORD_SIZE)
SleepState_VFP_S25    EQU    (SleepState_VFP_S24    +    WORD_SIZE)
SleepState_VFP_S26    EQU    (SleepState_VFP_S25    +    WORD_SIZE)
SleepState_VFP_S27    EQU    (SleepState_VFP_S26    +    WORD_SIZE)
SleepState_VFP_S28    EQU    (SleepState_VFP_S27    +    WORD_SIZE)
SleepState_VFP_S29    EQU    (SleepState_VFP_S28    +    WORD_SIZE)
SleepState_VFP_S30    EQU    (SleepState_VFP_S29    +    WORD_SIZE)
SleepState_VFP_S31    EQU    (SleepState_VFP_S30    +    WORD_SIZE)
SleepState_Data_End        EQU    (SleepState_VFP_S31    +     WORD_SIZE )
SLEEPDATA_SIZE          EQU ((SleepState_Data_End - SleepState_Data_Start) / 4)

;//------------------------------------------------------------------------------

    END

