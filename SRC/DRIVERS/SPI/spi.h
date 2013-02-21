// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Module Name :    SPI.H
//
// Abstract    :    SPI Interface Routines for Samsung S3C6410 CPU
//
// Environment :    Samsung S3C6410
//
// 2007/04/
//
//-------------------------------------------------------------------------------------------------------------------------

#ifndef _SPI_H_
#define _SPI_H_

#include <windows.h>
#include <s3c6410.h>

//#define TEST_MODE

//#define SPI_TX_DATA_PHY_ADDR    0x52000018
//#define SPI_RX_DATA_PHY_ADDR    0x5200001C
#define SPI_TX_DATA_PHY_ADDR    0x7F00B018
#define SPI_RX_DATA_PHY_ADDR    0x7F00B01C

#define FIFO_SIZE             0x40
#define FIFO_HALF_SIZE        0x20

#define FIFO_FULL            0x40
#define FIFO_EMPTY            0x0

//#define    RX_TRIG_LEVEL        0x8
//#define    TX_TRIG_LEVEL        0x14
#define    RX_TRIG_LEVEL        01
#define    TX_TRIG_LEVEL        63

#define HIGH_SPEED_MASK        (1<<6)
#define HIGH_SPEED_DIS        (0<<6)
#define HIGH_SPEED_EN        (1<<6)

#define    SW_RST                (1<<5)
#define    SPI_MASTER            (0<<4)
#define    SPI_SLAVE            (1<<4)
#define    CPOL_RISING        (0<<3)
#define    CPOL_FALLING        (1<<3)
#define    CPHA_FORMAT_A        (0<<2)
#define    CPHA_FORMAT_B        (1<<2)
#define    RX_CH_OFF            (0<<1)
#define    RX_CH_ON            (1<<1)
#define    TX_CH_OFF            (0<<0)
#define    TX_CH_ON            (1<<0)


#define    CLKSEL_PCLK        (0<<9)
#define    CLKSEL_USBCLK        (1<<9)
#define    CLKSEL_EPLL        (2<<9)
#define    ENCLK_DISABLE        (0<<8)
#define    ENCLK_ENABLE        (1<<8)


#define    CH_SIZE_BYTE        (0<<29)
#define    CH_SIZE_HALF        (1<<29)
#define    CH_SIZE_WORD        (2<<29)
#define    BUS_SIZE_BYTE        (0<<17)
#define    BUS_SIZE_HALF        (1<<17)
#define    BUS_SIZE_WORD        (2<<17)
#define    DMA_SINGLE            (0<<0)
#define    DMA_4BURST            (1<<0)
#define    RX_DMA_ON            (1<<2)
#define    TX_DMA_ON            (1<<1)
#define    MODE_DEFAULT        (0)


#define    INT_TRAILING        (1<<6)
#define    INT_RX_OVERRUN    (1<<5)
#define    INT_RX_UNDERRUN    (1<<4)
#define    INT_TX_OVERRUN    (1<<3)
#define    INT_TX_UNDERRUN    (1<<2)
#define    INT_RX_FIFORDY        (1<<1)
#define    INT_TX_FIFORDY        (1<<0)

#define    TX_DONE            (1<<21)
#define    TRAILCNT_ZERO        (1<<20)
#define    RX_OVERRUN            (1<<5)
#define    RX_UNDERRUN        (1<<4)
#define    TX_OVERRUN            (1<<3)
#define    TX_UNDERRUN        (1<<2)
#define    RX_FIFORDY            (1<<1)
#define    TX_FIFORDY            (1<<0)

#define    PACKET_CNT_EN        (1<<16)

#define    TX_UNDERRUN_CLR    (1<<4)
#define    TX_OVERRUN_CLR    (1<<3)
#define    RX_UNDERRUN_CLR    (1<<2)
#define    RX_OVERRUN_CLR    (1<<1)
#define    TRAILING_CLR        (1<<0)

#define    RX_HALF_SWAP        (1<<7)
#define    RX_BYTE_SWAP        (1<<6)
#define    RX_BIT_SWAP        (1<<5)
#define    RX_SWAP_EN        (1<<4)
#define    TX_HALF_SWAP        (1<<3)
#define    TX_BYTE_SWAP        (1<<2)
#define    TX_BIT_SWAP        (1<<1)
#define    TX_SWAP_EN        (1<<0)

#define    SPI_0NS_DELAY        (0x0)
#define    SPI_2NS_DELAY        (0x1)
#define    SPI_4NS_DELAY        (0x2)
#define    SPI_6NS_DELAY        (0x3)

//#define    PADDRFIX            (1<<24)

typedef enum {            
    STATE_TIMEOUT,
    STATE_READING,
    STATE_RXDMA,
    STATE_RXINTR,
    STATE_WRITING,
    STATE_TXDMA,
    STATE_TXINTR,
    STATE_CONTROLLING,
    STATE_RXBUFFERRING,
    STATE_TXBUFFERRING,
    STATE_IDLE,
    STATE_CANCELLING,
    STATE_INIT,
    STATE_ERROR
} SPI_STATUS;

typedef struct {
    PBYTE pStrMem;
    PBYTE pEndMem;
    PBYTE pCurMem;
    DWORD dwMemSize;
    DWORD dwDataSize;
    DWORD dwUsedSize;
    DWORD dwUnusedSize;
    BOOL  bNeedBuffering;
} SPI_BUFFER;


typedef struct {
    PVOID                        pSpiPrivate;
    
    volatile S3C6410_GPIO_REG     *pGPIOregs;
    volatile S3C6410_SPI_REG           *pSPIregs;    //    For HS-SPI
    volatile S3C6410_SYSCON_REG     *pSYSCONregs;
    volatile S3C6410_DMAC_REG       *pDMAC0regs;
    volatile S3C6410_DMAC_REG       *pDMAC1regs;
    
    
    DWORD                        dwRxThreadId;
    DWORD                        dwRxThreadPrio;
    HANDLE                        hRxEvent;
    HANDLE                        hRxThread;
    HANDLE                        hRxDoneEvent;
    HANDLE                        hRxIntrDoneEvent;
    
    DWORD                        dwTxThreadId;
    DWORD                        dwTxThreadPrio;
    HANDLE                        hTxEvent;
    HANDLE                        hTxThread;
    HANDLE                        hTxDoneEvent;
    HANDLE                        hTxIntrDoneEvent;
    
    DWORD                        dwSpiThreadId;
    DWORD                        dwSpiThreadPrio;
    DWORD                        dwSpiSysIntr;
    HANDLE                        hSpiEvent;
    HANDLE                         hSpiThread;
    
    DWORD                        dwRxDmaDoneThreadId;
    DWORD                        dwRxDmaDoneThreadPrio;
    DWORD                         dwRxDmaDoneSysIntr;
    HANDLE                            hRxDmaDoneEvent;
    HANDLE                            hRxDmaDoneDoneEvent;
    HANDLE                              hRxDmaDoneThread;
    
    DWORD                        dwTxDmaDoneThreadId;
    DWORD                                 dwTxDmaDoneThreadPrio;
    DWORD                         dwTxDmaDoneSysIntr;
    HANDLE                            hTxDmaDoneEvent;
    HANDLE                            hTxDmaDoneDoneEvent;
    HANDLE                              hTxDmaDoneThread;
    
    CRITICAL_SECTION              CsTxAccess;
    CRITICAL_SECTION              CsRxAccess;
    
//    DWORD                                   dwSizeOfUsedBuffer;
} SPI_PUBLIC_CONTEXT, *PSPI_PUBLIC_CONTEXT;

typedef struct {
    PSPI_PUBLIC_CONTEXT            pSpiPublic;
    
    DWORD                        dwMode;

    BOOL                                  bUseRxDMA;
    BOOL                        bUseRxIntr;
    SPI_BUFFER                         RxBuffer;
    LPVOID                        pRxBuffer;
    LPVOID                        pRxDMABuffer;    
    DWORD                        dwRxCount;
    S3C6410_SPI_REG                   RxSPIregs;    // for HS-SPI        
    
    BOOL                                  bUseTxDMA;
    BOOL                        bUseTxIntr;
    SPI_BUFFER                        TxBuffer;
    LPVOID                        pTxBuffer;
    LPVOID                        pTxDMABuffer;
    DWORD                        dwTxCount;
    S3C6410_SPI_REG                   TxSPIregs;    // for HS-SPI
    
    DWORD                        dwTimeOutVal;
    DWORD                        dwPrescaler;
    DWORD                        dwError;
    
    SPI_STATUS                    State;
} SPI_PRIVATE_CONTEXT, *PSPI_PRIVATE_CONTEXT;





#endif

