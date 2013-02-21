;------------------------------------------------------------------------------
;
;  File:  memory_cfg.inc
;
;  This file is used to define g_oalAddressTable. This table is passed to
;  KernelStart to estabilish physical to virtual memory mapping. This table
;  is used also in IOMEM OAL module to map between physical and virtual
;  memory addresses via OALPAtoVA/OALVAtoPA functions.
;
;------------------------------------------------------------------------------

; Export Definition

        EXPORT  g_oalAddressTable[DATA]

;------------------------------------------------------------------------------
;
; TABLE FORMAT
;       cached address, physical address, size
;------------------------------------------------------------------------------

g_oalAddressTable
    [ BSP_TYPE = BSP_DRAM128

    ; mDDR 128 MB     
        DCD     0x80000000, 0x50000000,  128     ; 128 MB DRAM
        DCD     0x90000000, 0x70000000,  4      ; SROM SFR
        ;DCD     0x90100000, 0x70100000,  1      ; OneNAND SFR
        ;DCD     0x90200000, 0x70200000,  1      ; NFCON SFR
        ;DCD     0x90300000, 0x70300000,  1      ; CFCON SFR
        DCD     0x90400000, 0x71000000,  4      ; TZIC0
        ;DCD     0x90500000, 0x71100000,  1      ; TZIC1
        ;DCD     0x90600000, 0x71200000,  1      ; INTC0
        ;DCD     0x90700000, 0x71300000,  1      ; INTC1
        DCD        0x90800000, 0x72000000,  1    ; FIMG-3DSE SFR
        ;DCD     0x90800000, 0x73000000,  2      ; ETB Memory
        ;DCD     0x90900000, 0x73100000,  1      ; ETB Registers
        DCD     0x90A00000, 0x74000000,  2      ; Indirect Host I/F
        ;DCD     0x90B00000, 0x74100000,  1      ; Direct Host I/F(MODEM)
        DCD     0x90C00000, 0x74300000,  2      ; USB Host
        ;DCD     0x90D00000, 0x74400000,  1      ; MDP I/F
        DCD     0x90E00000, 0x75000000,  2      ; DMA0
        ;DCD     0x90F00000, 0x75100000,  1      ; DMA1
        DCD     0x91000000, 0x76100000,  3      ; 2D Graphics
        ;DCD     0x91100000, 0x76200000,  1      ; TV Encoder
        ;DCD     0x91200000, 0x76300000,  1      ; TV Scaler
        DCD     0x91300000, 0x77000000,  3      ; Post Processor
        ;DCD     0x91400000, 0x77100000,  1      ; LCD Controller
        ;DCD     0x91500000, 0x77200000,  1      ; Rotator
        DCD     0x91600000, 0x78000000,  1      ; Camera I/F
        DCD     0x91700000, 0x78800000,  1      ; JPEG
        DCD     0x91800000, 0x7C000000,  5      ; USB OTG LINK
        ;DCD     0x91900000, 0x7C100000,  1      ; USB OTG PHY SFR
        DCD     0x91A00000, 0x7C200000,  1      ; SD-MMC Controller 0
        DCD     0x91B00000, 0x7C300000,  1      ; SD-MMC Controller 1
        ;DCD     0x91C00000, 0x7C400000,  1      ; SD-MMC Controller 2
        DCD     0x91D00000, 0x7D000000,  13      ; D&I(Security Subsystem Config) SFR
        ;DCD     0x91E00000, 0x7D100000,  1      ; AES_RX
        ;DCD     0x91F00000, 0x7D200000,  1      ; DES_RX
        ;DCD     0x92000000, 0x7D300000,  1      ; HASH(SHA/PRNG)_RX
        ;DCD     0x92100000, 0x7D400000,  1      ; RX_FIFO SFR
        ;DCD     0x92200000, 0x7D500000,  1      ; AES_TX
        ;DCD     0x92300000, 0x7D600000,  1      ; DES_TX
        ;DCD     0x92400000, 0x7D700000,  1      ; HASH(SHA/PRNG)_TX
        ;DCD     0x92500000, 0x7D800000,  1      ; TX FIFO SFR
        ;DCD     0x92600000, 0x7D900000,  1      ; RX_FIFO
        ;DCD     0x92700000, 0x7DA00000,  1      ; TX_FIFO
        ;DCD     0x92800000, 0x7DB00000,  1      ; SDMA0
        ;DCD     0x92900000, 0x7DC00000,  1      ; SDMA1
        DCD     0x92A00000, 0x7E000000,  1      ; DMC, MFC, WDT, RTC, HSI TX/RX, Keypad, ADC, SYSCON
        DCD     0x92B00000, 0x7F000000,  1      ; TZPC, AC97, I2S, I2C, UART, PWM, IrDA, GPIO, PCM, SPI

        DCD     0x93000000, 0x00000000,  16      ; 32 MB SROM(SRAM/ROM) BANK 0

        ; nCS1~nCS5, nCS0
        DCD     0x94000000, 0x18000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 1 = CS8900
        ;DCD     0x96000000, 0x20000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 2
        ;DCD     0x98000000, 0x28000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 3
        ;DCD     0x9A000000, 0x30000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 4
        ;DCD     0x9C000000, 0x38000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 5
        ;DCD     0x9E000000, 0x00000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 0

        DCD     0x00000000, 0x00000000,  0      ; end of table
      ]

    [ BSP_TYPE = BSP_DRAM256
        DCD     0x80000000, 0x50000000,  256     ; 256 MB DRAM
        DCD     0x90000000, 0x70000000,  4      ; SROM SFR
        ;DCD     0x90100000, 0x70100000,  1      ; OneNAND SFR
        ;DCD     0x90200000, 0x70200000,  1      ; NFCON SFR
        ;DCD     0x90300000, 0x70300000,  1      ; CFCON SFR
        DCD     0x90400000, 0x71000000,  4      ; TZIC0
        ;DCD     0x90500000, 0x71100000,  1      ; TZIC1
        ;DCD     0x90600000, 0x71200000,  1      ; INTC0
        ;DCD     0x90700000, 0x71300000,  1      ; INTC1
        DCD        0x90800000, 0x72000000,  1    ; FIMG-3DSE SFR
        ;DCD     0x90800000, 0x73000000,  2      ; ETB Memory
        ;DCD     0x90900000, 0x73100000,  1      ; ETB Registers
        DCD     0x90A00000, 0x74000000,  2      ; Indirect Host I/F
        ;DCD     0x90B00000, 0x74100000,  1      ; Direct Host I/F(MODEM)
        DCD     0x90C00000, 0x74300000,  2      ; USB Host
        ;DCD     0x90D00000, 0x74400000,  1      ; MDP I/F
        DCD     0x90E00000, 0x75000000,  2      ; DMA0
        ;DCD     0x90F00000, 0x75100000,  1      ; DMA1
        DCD     0x91000000, 0x76100000,  3      ; 2D Graphics
        ;DCD     0x91100000, 0x76200000,  1      ; TV Encoder
        ;DCD     0x91200000, 0x76300000,  1      ; TV Scaler
        DCD     0x91300000, 0x77000000,  3      ; Post Processor
        ;DCD     0x91400000, 0x77100000,  1      ; LCD Controller
        ;DCD     0x91500000, 0x77200000,  1      ; Rotator
        DCD     0x91600000, 0x78000000,  1      ; Camera I/F
        DCD     0x91700000, 0x78800000,  1      ; JPEG
        DCD     0x91800000, 0x7C000000,  5      ; USB OTG LINK
        ;DCD     0x91900000, 0x7C100000,  1      ; USB OTG PHY SFR
        DCD     0x91A00000, 0x7C200000,  1      ; SD-MMC Controller 0
        DCD     0x91B00000, 0x7C300000,  1      ; SD-MMC Controller 1
        ;DCD     0x91C00000, 0x7C400000,  1      ; SD-MMC Controller 2
        DCD     0x91D00000, 0x7D000000,  13      ; D&I(Security Subsystem Config) SFR
        ;DCD     0x91E00000, 0x7D100000,  1      ; AES_RX
        ;DCD     0x91F00000, 0x7D200000,  1      ; DES_RX
        ;DCD     0x92000000, 0x7D300000,  1      ; HASH(SHA/PRNG)_RX
        ;DCD     0x92100000, 0x7D400000,  1      ; RX_FIFO SFR
        ;DCD     0x92200000, 0x7D500000,  1      ; AES_TX
        ;DCD     0x92300000, 0x7D600000,  1      ; DES_TX
        ;DCD     0x92400000, 0x7D700000,  1      ; HASH(SHA/PRNG)_TX
        ;DCD     0x92500000, 0x7D800000,  1      ; TX FIFO SFR
        ;DCD     0x92600000, 0x7D900000,  1      ; RX_FIFO
        ;DCD     0x92700000, 0x7DA00000,  1      ; TX_FIFO
        ;DCD     0x92800000, 0x7DB00000,  1      ; SDMA0
        ;DCD     0x92900000, 0x7DC00000,  1      ; SDMA1
        DCD     0x92A00000, 0x7E000000,  1      ; DMC, MFC, WDT, RTC, HSI TX/RX, Keypad, ADC, SYSCON
        DCD     0x92B00000, 0x7F000000,  1      ; TZPC, AC97, I2S, I2C, UART, PWM, IrDA, GPIO, PCM, SPI

        DCD     0x93000000, 0x00000000,  16      ; 32 MB SROM(SRAM/ROM) BANK 0

        ; nCS1~nCS5, nCS0
        DCD     0x94000000, 0x18000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 1 = CS8900
        ;DCD     0x96000000, 0x20000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 2
        ;DCD     0x98000000, 0x28000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 3
        ;DCD     0x9A000000, 0x30000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 4
        ;DCD     0x9C000000, 0x38000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 5
        ;DCD     0x9E000000, 0x00000000, 32      ; 32 MB SROM(SRAM/ROM) BANK 0

        DCD     0x00000000, 0x00000000,  0      ; end of table
      ]

;------------------------------------------------------------------------------

        END
