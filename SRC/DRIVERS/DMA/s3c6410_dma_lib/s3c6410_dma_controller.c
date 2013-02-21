#include <windows.h>
#include <CEDDK.h>
#include <bsp_cfg.h>
#include <oal_intr.h>
#include <s3c6410.h>
#include "s3c6410_dma_controller_macro.h"
#include "s3c6410_dma_controller.h"

#define DMA_MSG(x)
#define DMA_INF(x)
#define DMA_ERR(x)    RETAILMSG(TRUE, x)

#define DMA_MUTEX    TEXT("S3C6410_DMA_Mutex")

static volatile S3C6410_DMAC_REG *g_pDMAC0Reg = NULL;
static volatile S3C6410_DMAC_REG *g_pDMAC1Reg = NULL;
static volatile S3C6410_SYSCON_REG *g_pSysConReg = NULL;
static HANDLE g_hMutex = NULL;

DMA_ERROR DMA_initialize_register_address(void *pDMAC0Reg, void *pDMAC1Reg, void *pSysConReg)
{
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_initialize_register_address(0x%08x, 0x%08x, 0x%08x)\n\r"), pDMAC0Reg, pDMAC1Reg, pSysConReg));

    if (pDMAC0Reg == NULL || pDMAC1Reg == NULL || pSysConReg == NULL)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_register_address() : NULL pointer parameter\n\r")));
        error = DMA_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pDMAC0Reg = (S3C6410_DMAC_REG *)pDMAC0Reg;
        g_pDMAC1Reg = (S3C6410_DMAC_REG *)pDMAC1Reg;
        g_pSysConReg = (S3C6410_SYSCON_REG *)pSysConReg;
        DMA_INF((_T("[DMA:INF] g_pDMAC0Reg = 0x%08x\n\r"), g_pDMAC0Reg));
        DMA_INF((_T("[DMA:INF] g_pDMAC1Reg = 0x%08x\n\r"), g_pDMAC1Reg));
        DMA_INF((_T("[DMA:INF] g_pSysConReg = 0x%08x\n\r"), g_pSysConReg));

        g_pSysConReg->SDMA_SEL = 0xcfffffff;    // All DMA is set to Normal DMA except Security TX, RX
        DMA_INF((_T("[DMA:INF] All DMA Source is set to Normal DMA\n\r")));
    }

    if (g_hMutex == NULL)
    {
        g_hMutex = CreateMutex(NULL, FALSE, DMA_MUTEX);
        if (g_hMutex == NULL)
        {
            DMA_ERR((_T("[DMA:ERR] DMA_initialize_register_address() : CreateMutex() Failed\n\r")));
            error = DMA_ERROR_NOT_INITIALIZED;
        }
    }

    DMA_MSG((_T("[DMA]--DMA_initialize_register_address() : %d\n\r"), error));

    return error;
}

BOOL DMA_request_channel(DMA_CH_CONTEXT *pCtxt, DMA_SOURCE DMASrc)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    BOOL bRet = TRUE;
    int i;

    DMA_MSG((_T("[DMA]++DMA_request_channel(%d)\n\r"), DMASrc));

    pCtxt->DMASrc = DMASrc;

    if (g_pDMAC0Reg == NULL || g_pDMAC1Reg == NULL)
    {
        DMA_ERR((_T("[DMA:ERR] DMA Register Address is Not initialized\n\r")));
        bRet = FALSE;
        goto CleanUp2;
    }

    switch(DMASrc)
    {
    case DMA_UART0_TX:
    case DMA_UART0_RX:
    case DMA_UART1_TX:
    case DMA_UART1_RX:
    case DMA_UART2_TX:
    case DMA_UART2_RX:
    case DMA_UART3_TX:
    case DMA_UART3_RX:
    case DMA_PCM0_TX:
    case DMA_PCM0_RX:
    case DMA_I2S0_TX:
    case DMA_I2S0_RX:
    case DMA_SPI0_TX:
    case DMA_SPI0_RX:

    case DMA_I2S_V40_TX:
    case DMA_I2S_V40_RX:

    case DMA_MEM:
    case DMA_FIMG:
        pCtxt->Controller = DMAC0;
        break;
    case DMA_PCM1_TX:
    case DMA_PCM1_RX:
    case DMA_I2S1_TX:
    case DMA_I2S1_RX:
    case DMA_SPI1_TX:
    case DMA_SPI1_RX:
    case DMA_AC97_PCMOUT:
    case DMA_AC97_PCMIN:
    case DMA_AC97_MICIN:
    case DMA_PWM:
    case DMA_IRDA:
    case DMA_EXTERNAL:
    case DMA_SSS:
        pCtxt->Controller = DMAC1;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_request_channel() : Unknown DMA Source [%d]\n\r"), DMASrc));
        bRet = FALSE;
        goto CleanUp;
        break;
    }

    // Lock DMA Access
    DMA_lock();

    // Enable DMAC HCLK
    g_pSysConReg->HCLK_GATE |= (1<<12);    // DMAC0 HCLK Pass
    g_pSysConReg->HCLK_GATE |= (1<<13);    // DMAC1 HCLK Pass

    if (pCtxt->Controller == DMAC0)    // Find available channel in DMAC0
    {
        for (i=0; i<8; i++)
        {
            if (i == 3) continue;    // for OneNAND DMA channel

            pDMACHReg = (S3C6410_DMA_CH_REG *)((ULONG)g_pDMAC0Reg + DMA_CH_REG_OFFSET + i*DMA_CH_REG_SIZE);

            if (!(pDMACHReg->Control0 & TCINT_ENABLE))    // Check channel in use with TC Int Enable Bit !!!
            {
                pDMACHReg->Control0 |= TCINT_ENABLE;    // Set TC Int Enable Bit to reserve this channel!!!
                break;
            }
        }

        if (i < 8)        // there is available channel
        {
            DMA_INF((_T("[DMA:INF] DMA_request_channel() : Ch[%d] in DMAC0 is Available for DMASrc[%d]\n\r"), i, DMASrc));
            pCtxt->Channel = (DMAC_CH)i;
            pCtxt->dwIRQ = IRQ_DMA0_CH0+i;
            pCtxt->pCHReg = (void *)pDMACHReg;
            pCtxt->bValid = TRUE;
            DMA_dmac0_enable();
        }
        else if (DMASrc == DMA_MEM)    // DMA MEM2MEM can use channel in DMAC1
        {
            pCtxt->Controller = DMAC1;
        }
        else            // there is no available channel
        {
            DMA_ERR((_T("[DMA:ERR] DMA_request_channel() : No Available Channel in DMAC0 for DMASrc[%d]\n\r"), DMASrc));
            bRet = FALSE;
            goto CleanUp;
        }
    }

    if (pCtxt->Controller == DMAC1)    // Find available channel in DMAC1
    {
        for (i=0; i<8; i++)
        {
            if (i == 3) continue;    // for OneNAND DMA channel

            pDMACHReg = (S3C6410_DMA_CH_REG *)((ULONG)g_pDMAC1Reg + DMA_CH_REG_OFFSET + i*DMA_CH_REG_SIZE);

            if (!(pDMACHReg->Control0 & TCINT_ENABLE))    // Check channel in use with TC Int Enable Bit !!!
            {
                pDMACHReg->Control0 |= TCINT_ENABLE;    // Set TC Int Enable Bit to reserve this channel!!!
                break;
            }
        }

        if (i < 8)        // there is available channel
        {
            DMA_INF((_T("[DMA:INF] DMA_request_channel() : Ch[%d] in DMAC1 is Available for DMASrc[%d]\n\r"), i, DMASrc));
            pCtxt->Channel = (DMAC_CH)i;
            pCtxt->dwIRQ = IRQ_DMA1_CH0+i;
            pCtxt->pCHReg = (void *)pDMACHReg;
            pCtxt->bValid = TRUE;
            DMA_dmac1_enable();
        }
        else            // there is no available channel
        {
            DMA_ERR((_T("[DMA:ERR] DMA_request_channel() : No Available Channel in DMAC1 for DMASrc[%d]\n\r"), DMASrc));
            bRet = FALSE;
            goto CleanUp;
        }
    }

CleanUp:

    // Disable DMAC HCLK when No channel is allocated
    if (!(g_pDMAC0Reg->DMACConfiguration & DMAC_ENABLE))
    {
        g_pSysConReg->HCLK_GATE &= ~(1<<12);    // DMAC0 HCLK Mask
    }

    if (!(g_pDMAC1Reg->DMACConfiguration & DMAC_ENABLE))
    {
        g_pSysConReg->HCLK_GATE &= ~(1<<13);    // DMAC1 HCLK Mask
    }
    // Unlock DMA Access
    DMA_unlock();

    if (bRet == FALSE)        // Request is denied
    {
        // Clear DMA Channel Context
        memset((void *)pCtxt, 0x0, sizeof(DMA_CH_CONTEXT));
    }
CleanUp2:

    DMA_MSG((_T("[DMA]--DMA_request_channel() : %d\n\r"), bRet));

    return bRet;
}

BOOL DMA_release_channel(DMA_CH_CONTEXT *pCtxt)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    BOOL bRet = TRUE;

    DMA_MSG((_T("[DMA]++DMA_release_channel() : Ch%d in DMAC%d, \n\r"), pCtxt->Channel, pCtxt->Controller));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_release_channel() : Invalid DMA_CH_CONTEXT\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Stop DMA Channel
    DMA_channel_stop(pCtxt);

    // Lock DMA Access
    DMA_lock();

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;
    pDMACHReg->Control0 &= ~TCINT_ENABLE;        // Clear TC Int Enable Bit to release this channel!!!

    if (pCtxt->Controller == DMAC0)        // Release channel in DMAC0
    {
        DMA_dmac0_disable();
    }
    else if (pCtxt->Controller == DMAC1)    // Release channel in DMAC1
    {
        DMA_dmac1_disable();
    }

    // Unlock DMA Access
    DMA_unlock();

CleanUp:

    // Release LLI buffer
    DMA_free_LLI_context(pCtxt);

    // Clear DMA Channel Context
    memset((void *)pCtxt, 0x0, sizeof(DMA_CH_CONTEXT));

    DMA_MSG((_T("[DMA]--DMA_release_channel() : %d\n\r"), bRet));

    return bRet;
}

DMA_ERROR DMA_initialize_channel(DMA_CH_CONTEXT *pCtxt, BOOL bSync)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_initialize_channel() : Ch%d in DMAC%d (bSync:%d)\n\r"), pCtxt->Channel, pCtxt->Controller, bSync));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_channel() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pCtxt->LLICount = 0;
    pCtxt->LLIAHBM = AHB_M1;

    switch(pCtxt->DMASrc)
    {
    //-------------------
    // DMA Source in DMC0
    //-------------------
    case DMA_UART0_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_UART0_TX;        // UART0_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_UART0_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_UART0_RX;        // UART0_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_UART1_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_UART1_TX;        // UART1_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_UART1_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_UART1_RX;        // UART1_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_UART2_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_UART2_TX;        // UART2_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_UART2_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_UART2_RX;        // UART2_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_UART3_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_UART3_TX;        // UART3_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_UART3_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_UART3_RX;        // UART3_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_PCM0_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_PCM0_TX;        // PCM0_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_PCM0_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_PCM0_RX;        // PCM0_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_I2S0_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_I2S0_TX;        // I2S0_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_I2S0_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_I2S0_RX;        // I2S0_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_SPI0_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_SPI0_TX;        // SPI0_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_SPI0_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_SPI0_RX;        // SPI0_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_I2S_V40_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC0_I2S_V40_TX;    // I2S_V40_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_I2S_V40_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC0_I2S_V40_RX;    // I2S_V40_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    
    //-------------------
    // DMA Source in DMC1
    //-------------------
    case DMA_PCM1_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC1_PCM1_TX;        // PCM1_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_PCM1_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC1_PCM1_RX;        // PCM1_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_I2S1_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC1_I2S1_TX;        // I2S1_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_I2S1_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC1_I2S1_RX;        // I2S1_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_SPI1_TX:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC1_SPI1_TX;        // SPI1_TX
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_SPI1_RX:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC1_SPI1_RX;        // SPI1_RX
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_AC97_PCMOUT:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Peripheral
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = DMAC1_AC97_PCMOUT;    // AC97_PCMOUT
        pCtxt->FlowCtrl = MEM_TO_PERI;        // Memory -> Peripheral
        break;
    case DMA_AC97_PCMIN:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC1_AC97_PCMIN;    // AC97_PCMIN
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_AC97_MICIN:
        pCtxt->SrcAHBM = AHB_M2;            // Peripheral
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = DMAC1_AC97_MICIN;    // AC97_MICIN
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = PERI_TO_MEM;        // Peripheral -> Memory
        break;
    case DMA_PWM:
    case DMA_IRDA:
    case DMA_EXTERNAL:
    case DMA_SSS:
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_channel() : Not Implemented\n\r")));
        error = DMA_ERROR_NOT_IMPLEMENTED;
        goto CleanUp;
        break;
    //------------------------
    // DMA Source in MEM to MEM
    //------------------------
    case DMA_MEM:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M1;            // Memory
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = MEM_TO_MEM;        // Memory -> Memory
        break;
    case DMA_FIMG:
        pCtxt->SrcAHBM = AHB_M1;            // Memory
        pCtxt->DstAHBM = AHB_M2;            // Memory
        pCtxt->SrcPeri = 0;                    // Memory (Don't Care)
        pCtxt->DstPeri = 0;                    // Memory (Don't Care)
        pCtxt->FlowCtrl = MEM_TO_MEM;        // Memory -> Memory
        break;        
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_channel() : Unknown DMA Source [%d]\n\r"), pCtxt->DMASrc));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

#if    0
    if (pCtxt->Controller == DMAC0)        // channel in DMAC0
    {
        // Lock DMA Access
        DMA_lock();

        if (bSync)
        {
            // Synchronization Logic Enable (Default Value)
            g_pDMAC0Reg->DMACSync &= ~(1<<(pCtxt->DMASrc));
        }
        else
        {
            // Synchronization Logic Disable
            g_pDMAC0Reg->DMACSync |= (1<<(pCtxt->DMASrc));
        }

        // Unlock DMA Access
        DMA_unlock();
    }
    else if (pCtxt->Controller == DMAC1)    // channel in DMAC1
    {
        // Lock DMA Access
        DMA_lock();

        if (bSync)
        {
            // Synchronization Logic Enable (Default Value)
            g_pDMAC1Reg->DMACSync &= ~(1<<(pCtxt->DMASrc-16));
        }
        else
        {
            // Synchronization Logic Disable
            g_pDMAC1Reg->DMACSync |= (1<<(pCtxt->DMASrc-16));
        }

        // Unlock DMA Access
        DMA_unlock();
    }
#endif

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;
    pDMACHReg->LLI = 0;        // Disable LLI

    pDMACHReg->Configuration = ALLOW_REQUEST | UNLOCK | FLOWCTRL(pCtxt->FlowCtrl)
                        | DEST_PERI(pCtxt->DstPeri) | SRC_PERI(pCtxt->SrcPeri);

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_initialize_channel() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_set_channel_source(DMA_CH_CONTEXT *pCtxt, unsigned int uiSrcAddr, TRANSFER_UNIT Unit, BURST_SIZE Burst, ADDRESS_UPDATE Update)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_set_channel_source() : Ch%d in DMAC%d (0x%08x, %d, %d, %d)\n\r"), pCtxt->Channel, pCtxt->Controller, uiSrcAddr, Unit, Burst, Update));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_source() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    switch(Unit)
    {
    case BYTE_UNIT: case HWORD_UNIT: case WORD_UNIT:
        pCtxt->SrcUnit = Unit;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_source() : Unknown Transfer Unit [%d]\n\r"), Unit));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    switch(Burst)
    {
    case BURST_1:    case BURST_4:    case BURST_8:    case BURST_16:
    case BURST_32:    case BURST_64:    case BURST_128:    case BURST_256:
        pCtxt->SrcBurst = Burst;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_source() : Unknown Burst Size [%d]\n\r"), Burst));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    switch(Update)
    {
    case INCREASE: case FIXED:
        pCtxt->SrcUpdate = Update;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_source() : Unknown Address Update [%d]\n\r"), Update));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    pDMACHReg->SrcAddr = uiSrcAddr;
    pDMACHReg->Control0 = (pDMACHReg->Control0 & ~((1<<26)|(1<<24)|SRC_UNIT_MASK|SRC_BURST_MASK))
                        |(pCtxt->SrcUpdate<<26) |(pCtxt->SrcAHBM<<24)|(pCtxt->SrcUnit<<18)|(pCtxt->SrcBurst<<12);

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_initialize_channel() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_set_channel_destination(DMA_CH_CONTEXT *pCtxt, unsigned int uiDstAddr, TRANSFER_UNIT Unit, BURST_SIZE Burst, ADDRESS_UPDATE Update)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_set_channel_destnation() : Ch%d in DMAC%d (0x%08x, %d, %d, %d)\n\r"), pCtxt->Channel, pCtxt->Controller, uiDstAddr, Unit, Burst, Update));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_destnation() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    switch(Unit)
    {
    case BYTE_UNIT: case HWORD_UNIT: case WORD_UNIT:
        pCtxt->DstUnit = Unit;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_destnation() : Unknown Transfer Unit [%d]\n\r"), Unit));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    switch(Burst)
    {
    case BURST_1:    case BURST_4:    case BURST_8:    case BURST_16:
    case BURST_32:    case BURST_64:    case BURST_128:    case BURST_256:
        pCtxt->DstBurst = Burst;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_destnation() : Unknown Burst Size [%d]\n\r"), Burst));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    switch(Update)
    {
    case INCREASE: case FIXED:
        pCtxt->DstUpdate = Update;
        break;
    default:
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_destnation() : Unknown Address Update [%d]\n\r"), Update));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
        break;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    pDMACHReg->DestAddr = uiDstAddr;
    pDMACHReg->Control0 = (pDMACHReg->Control0 & ~((1<<27)|(1<<25)|DEST_UNIT_MASK|DEST_BURST_MASK))
                        |(pCtxt->DstUpdate<<27) |(pCtxt->DstAHBM<<25)|(pCtxt->DstUnit<<21)|(pCtxt->DstBurst<<15);

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_set_channel_destnation() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_set_channel_transfer_size(DMA_CH_CONTEXT *pCtxt, unsigned int uiByteCount)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_set_channel_transfer_size() : Ch%d in DMAC%d (ByteCount : %d)\n\r"), pCtxt->Channel, pCtxt->Controller, uiByteCount));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_set_channel_transfer_size() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    switch(pCtxt->SrcUnit)
    {
    case BYTE_UNIT:
        pDMACHReg->Control1 = TRANSFERCOUNT(uiByteCount);
        break;
    case HWORD_UNIT:
        pDMACHReg->Control1 = TRANSFERCOUNT(uiByteCount/2);
        break;
    case WORD_UNIT:
        pDMACHReg->Control1 = TRANSFERCOUNT(uiByteCount/4);
        break;
    }

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_initialize_channel() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_initialize_LLI(DMA_CH_CONTEXT *pCtxt, int iLLICount)
{
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_initialize_LLI() : Ch%d in DMAC%d (%d)\n\r"), pCtxt->Channel, pCtxt->Controller, iLLICount));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    if (iLLICount == 0 || iLLICount > MAX_LLI_ENTRY)
    {
        DMA_ERR((_T("[DMA] DMA_initialize_LLI() : LLICount [%d] Out of Range \n\r"), pCtxt->LLICount));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pCtxt->LLICount = iLLICount;

    if (DMA_allocate_LLI_context(pCtxt) == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : LLI Context Allocation Failed\n\r")));
        error = DMA_ERROR_NOT_INITIALIZED;
        goto CleanUp;
    }

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_initialize_LLI() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_set_initial_LLI(DMA_CH_CONTEXT *pCtxt, int iIntialLLIEntryNumber)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_initialize_LLI() : Ch%d in DMAC%d (%d)\n\r"), pCtxt->Channel, pCtxt->Controller, iIntialLLIEntryNumber));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    if (pCtxt->LLICount == 0)
    {
        DMA_ERR((_T("[DMA] DMA_initialize_LLI() : LLI Not Initialized\n\r")));
        error = DMA_ERROR_NOT_INITIALIZED;
        goto CleanUp;
    }

    if (iIntialLLIEntryNumber > pCtxt->LLICount-1)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : Initial Entry Number exceed  Maximum Entry Number (%d > %d)\n\r"), iIntialLLIEntryNumber, pCtxt->LLICount-1));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    pDMACHReg->LLI = NEXT_LLI_ITEM(pCtxt->LLIPhyAddr+sizeof(DMA_LLI_ENTRY)*iIntialLLIEntryNumber) |pCtxt->LLIAHBM;

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_initialize_LLI() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_set_LLI_entry(DMA_CH_CONTEXT *pCtxt, int iEntryNumber, LLI_NEXT_ITEM NextItem, unsigned int uiSrcAddr, unsigned int uiDstAddr, unsigned int uiByteCount)
{
    DMA_LLI_ENTRY *pLLIEntry;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_initialize_LLI() : Ch%d in DMAC%d (%d, %d, 0x%08x, 0x%08x, %d)\n\r"), pCtxt->Channel, pCtxt->Controller, iEntryNumber, NextItem, uiSrcAddr, uiDstAddr, uiByteCount));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    if (iEntryNumber > pCtxt->LLICount-1)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_initialize_LLI() : LLI Entry exceed Maximum Entry Number (%d > %d)\n\r"), iEntryNumber, pCtxt->LLICount-1));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pLLIEntry = (DMA_LLI_ENTRY *)(pCtxt->LLIVirAddr+sizeof(DMA_LLI_ENTRY)*iEntryNumber);

    pLLIEntry->SrcAddr = uiSrcAddr;
    pLLIEntry->DestAddr = uiDstAddr;
    pLLIEntry->Control0 = TCINT_ENABLE |(pCtxt->DstUpdate<<27) | (pCtxt->SrcUpdate<<26)
                    | (pCtxt->DstAHBM<<25) | (pCtxt->SrcAHBM<<24)|(pCtxt->DstUnit<<21)
                    | (pCtxt->SrcUnit<<18) | (pCtxt->DstBurst<<15) | (pCtxt->SrcBurst<<12);

    switch(pCtxt->SrcUnit)
    {
    case BYTE_UNIT:
        pLLIEntry->Control1 = TRANSFERCOUNT(uiByteCount);
        break;
    case HWORD_UNIT:
        pLLIEntry->Control1 = TRANSFERCOUNT(uiByteCount/2);
        break;
    case WORD_UNIT:
        pLLIEntry->Control1 = TRANSFERCOUNT(uiByteCount/4);
        break;
    }

    switch(NextItem)
    {
    case LLI_NEXT_ENTRY:
        if (iEntryNumber+1 > pCtxt->LLICount-1)
        {
            DMA_ERR((_T("[DMA:ERR] DMA_set_LLI_entry() : This Entry is Last. Can't set LLI to Next Entry (%d)\n\r"), iEntryNumber));
            error = DMA_ERROR_ILLEGAL_PARAMETER;
            goto CleanUp;
        }
        else
        {
            pLLIEntry->LLI = NEXT_LLI_ITEM(pCtxt->LLIPhyAddr+sizeof(DMA_LLI_ENTRY)*(iEntryNumber+1)) |pCtxt->LLIAHBM;
        }
        break;
    case LLI_FIRST_ENTRY:
        pLLIEntry->LLI = NEXT_LLI_ITEM(pCtxt->LLIPhyAddr) |pCtxt->LLIAHBM;
        break;
    case LLI_THIS_IS_END:
        pLLIEntry->LLI = 0;
        break;
    }

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_set_LLI_entry() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_channel_start(DMA_CH_CONTEXT *pCtxt)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_channel_start() : Ch%d in DMAC%d\n\r"), pCtxt->Channel, pCtxt->Controller));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_channel_start() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    if (pDMACHReg->Configuration & CHANNEL_ENABLE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_channel_start() : Channel Alread in Use\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    // Clear Interrupt Pending
    DMA_clear_interrupt_pending(pCtxt);

    // Clear Interrupt Mask
    DMA_clear_interrupt_mask(pCtxt);

    // Clear Halt Bit
    pDMACHReg->Configuration &= ~HALT;

    // Enable Channel
    pDMACHReg->Configuration |= CHANNEL_ENABLE;

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_channel_start() : %d\n\r"), error));

    return error;
}

DMA_ERROR DMA_channel_stop(DMA_CH_CONTEXT *pCtxt)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    DMA_ERROR error = DMA_SUCCESS;

    DMA_MSG((_T("[DMA]++DMA_channel_stop() : Ch%d in DMAC%d\n\r"), pCtxt->Channel, pCtxt->Controller));

    if (pCtxt->bValid == FALSE)
    {
        DMA_ERR((_T("[DMA:ERR] DMA_channel_stop() : Invalid DMA_CH_CONTEXT\n\r")));
        error = DMA_ERROR_ILLEGAL_PARAMETER;
        goto CleanUp;
    }

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    if (!(pDMACHReg->Configuration & CHANNEL_ENABLE))
    {
        DMA_INF((_T("[DMA:INF] DMA_channel_stop() : Channel Already Disabled\n\r")));
    }
    else
    {
        // Set Interrupt Mask
        DMA_set_interrupt_mask(pCtxt);

        // Set Halt Bit
        pDMACHReg->Configuration |= HALT;

#if    0    // TODO: Sometimes Channel is not inactivated when TC reached to "0"
        // Wait For Channel Inactive
        while(pDMACHReg->Configuration & ACTIVE);
#endif

        // Disable Channel
        pDMACHReg->Configuration &= ~CHANNEL_ENABLE;
    }

    // Clear Interrupt Pending
    DMA_clear_interrupt_pending(pCtxt);

CleanUp:

    DMA_MSG((_T("[DMA]--DMA_channel_stop() : %d\n\r"), error));

    return error;
}

DMA_INT_STATUS DMA_get_interrupt_status(DMA_CH_CONTEXT *pCtxt)
{
    DMA_INT_STATUS status = NO_INT_PEND;

    if (pCtxt->Controller == DMAC0)        // channel in DMAC0
    {
        if (g_pDMAC0Reg->DMACIntTCStatus & (1<<(pCtxt->Channel)))
        {
            status |= TC_INT_PEND;
        }

        if (g_pDMAC0Reg->DMACIntErrStatus & (1<<(pCtxt->Channel)))
        {
            status |= ERR_INT_PEND;
        }
    }
    else if (pCtxt->Controller == DMAC1)    // channel in DMAC1
    {
        if (g_pDMAC1Reg->DMACIntTCStatus & (1<<(pCtxt->Channel)))
        {
            status |= TC_INT_PEND;
        }

        if (g_pDMAC1Reg->DMACIntErrStatus & (1<<(pCtxt->Channel)))
        {
            status |= ERR_INT_PEND;
        }
    }

    return status;
}

void DMA_set_interrupt_mask(DMA_CH_CONTEXT *pCtxt)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    pDMACHReg->Configuration &= ~(TCINT_UNMASK|ERRINT_UNMASK);        // Mask is Bit Clear, Unmask is Bit Set
}

void DMA_clear_interrupt_mask(DMA_CH_CONTEXT *pCtxt)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;

    pDMACHReg = (S3C6410_DMA_CH_REG *)pCtxt->pCHReg;

    pDMACHReg->Configuration |= (TCINT_UNMASK|ERRINT_UNMASK);        // Mask is Bit Clear, Unmask is Bit Set
}

void DMA_clear_interrupt_pending(DMA_CH_CONTEXT *pCtxt)
{
    if (pCtxt->Controller == DMAC0)        // channel in DMAC0
    {
        g_pDMAC0Reg->DMACIntTCClear = (1<<(pCtxt->Channel));
        g_pDMAC0Reg->DMACIntErrClear = (1<<(pCtxt->Channel));
    }
    else if (pCtxt->Controller == DMAC1)    // channel in DMAC1
    {
        g_pDMAC1Reg->DMACIntTCClear = (1<<(pCtxt->Channel));
        g_pDMAC1Reg->DMACIntErrClear = (1<<(pCtxt->Channel));
    }
}

static BOOL DMA_dmac0_enable(void)
{
    if (!(g_pDMAC0Reg->DMACConfiguration & DMAC_ENABLE))        // DMAC0 is Disabled
    {
        // Enable DMAC0
        g_pDMAC0Reg->DMACConfiguration = M1_LITTLE_ENDIAN | M2_LITTLE_ENDIAN | DMAC_ENABLE;
        DMA_MSG((_T("[DMA] DMA_dmac0_enable() : DMAC0 Enabled\n\r")));
    }

    return TRUE;
}

static BOOL DMA_dmac0_disable(void)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    BOOL bInUse = FALSE;
    int i;

    if (g_pDMAC0Reg->DMACConfiguration & DMAC_ENABLE)    // DMAC0 is Enabled
    {
        for (i=0; i<8; i++)        // Check channel in Use
        {
            pDMACHReg = (S3C6410_DMA_CH_REG *)((ULONG)g_pDMAC0Reg + DMA_CH_REG_OFFSET + i*DMA_CH_REG_SIZE);

            if (pDMACHReg->Control0 & TCINT_ENABLE)    // Check channel in use with TC Int Enable Bit !!!
            {
                bInUse = TRUE;
                break;
            }
        }

        if (bInUse == FALSE)
        {
            if (g_pDMAC0Reg->DMACEnbldChns & 0xff)
            {
                DMA_ERR((_T("[DMA:ERR] DMA_dmac0_disable() : All TCINT_ENABLE Bit is Clear, But Channel Enabled [0x%02x]\n\r"), (g_pDMAC0Reg->DMACEnbldChns&0xff)));
            }
            else        // There is no channel enabled
            {
                // Disable DMAC0
                g_pDMAC0Reg->DMACConfiguration &= ~DMAC_ENABLE;
                g_pSysConReg->HCLK_GATE &= ~(1<<12);    // DMAC0 HCLK Mask
                DMA_MSG((_T("[DMA] DMA_dmac0_disable() : DMAC0 Disabled\n\r")));
            }
        }
    }

    return TRUE;
}

static BOOL DMA_dmac1_enable(void)
{
    if (!(g_pDMAC1Reg->DMACConfiguration & DMAC_ENABLE))        // DMAC1 is Disabled
    {
        // Enable DMAC1
        g_pDMAC1Reg->DMACConfiguration = M1_LITTLE_ENDIAN | M2_LITTLE_ENDIAN | DMAC_ENABLE;
        DMA_MSG((_T("[DMA] DMA_dmac1_enable() : DMAC1 Enabled\n\r")));
    }

    return TRUE;
}

static BOOL DMA_dmac1_disable(void)
{
    volatile S3C6410_DMA_CH_REG *pDMACHReg;
    BOOL bInUse = FALSE;
    int i;

    if (g_pDMAC1Reg->DMACConfiguration & DMAC_ENABLE)    // DMAC0 is Enabled
    {
        for (i=0; i<8; i++)        // Check channel in Use
        {
            pDMACHReg = (S3C6410_DMA_CH_REG *)((ULONG)g_pDMAC1Reg + DMA_CH_REG_OFFSET + i*DMA_CH_REG_SIZE);

            if (pDMACHReg->Control0 & TCINT_ENABLE)    // Check channel in use with TC Int Enable Bit !!!
            {
                bInUse = TRUE;
                break;
            }
        }

        if (bInUse == FALSE)
        {
            if (g_pDMAC1Reg->DMACEnbldChns & 0xff)
            {
                DMA_ERR((_T("[DMA:ERR] DMA_dmac1_disable() : All TCINT_ENABLE Bit is Clear, But Channel Enabled [0x%02x]\n\r"), (g_pDMAC1Reg->DMACEnbldChns&0xff)));
            }
            else        // There is no channel enabled
            {
                // Disable DMAC1
                g_pDMAC1Reg->DMACConfiguration &= ~DMAC_ENABLE;
                g_pSysConReg->HCLK_GATE &= ~(1<<13);    // DMAC1 HCLK Mask
                DMA_MSG((_T("[DMA] DMA_dmac1_disable() : DMAC1 Disabled\n\r")));
            }
        }
    }

    return TRUE;
}

static BOOL DMA_allocate_LLI_context(DMA_CH_CONTEXT *pCtxt)
{
    DMA_ADAPTER_OBJECT Adapter;
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID pVirtualAddress;
    BOOL bRet = TRUE;

    DMA_MSG((_T("[DMA] ++DMA_allocate_LLI_context()\n\r")));

    if ((pCtxt->LLICount > MAX_LLI_ENTRY) || pCtxt->LLICount == 0)
    {
        DMA_ERR((_T("[DMA] DMA_allocate_LLI_context() : LLICount [%d] Out of Range \n\r"), pCtxt->LLICount));
        bRet = FALSE;
        goto CleanUp;
    }

    if (pCtxt->LLIVirAddr)    // for Reinitialize LLI case
    {
        DMA_free_LLI_context(pCtxt);
    }

    memset(&Adapter, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;

    pVirtualAddress = HalAllocateCommonBuffer(&Adapter, pCtxt->LLICount*sizeof(DMA_LLI_ENTRY), &PhysicalAddress, FALSE);
    if (pVirtualAddress == NULL)
    {
        DMA_ERR((_T("[DMA] DMA_allocate_LLI_context() : HalAllocateCommonBuffer() Failed \n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    pCtxt->LLIPhyAddr = (unsigned int)PhysicalAddress.LowPart;
    pCtxt->LLIVirAddr = (unsigned int)pVirtualAddress;

CleanUp:

    if (bRet == FALSE)
    {
        pCtxt->LLIPhyAddr = 0;
        pCtxt->LLIVirAddr = 0;
    }

    DMA_MSG((_T("[DMA] --DMA_allocate_LLI_context() : %d\n\r"), bRet));

    return bRet;
}

static BOOL DMA_free_LLI_context(DMA_CH_CONTEXT *pCtxt)
{
    PHYSICAL_ADDRESS PhysicalAddress;
    PVOID pVirtualAddress;
    BOOL bRet = TRUE;

    DMA_MSG((_T("[DMA] ++DMA_free_LLI_context()\n\r")));

    if (pCtxt->LLIVirAddr)
    {
        PhysicalAddress.LowPart = pCtxt->LLIPhyAddr;        // No Meaning just for compile
        pVirtualAddress = (PVOID)pCtxt->LLIVirAddr;

        HalFreeCommonBuffer(0, 0, PhysicalAddress, pVirtualAddress, FALSE);

        pCtxt->LLIPhyAddr = 0;
        pCtxt->LLIVirAddr = 0;
    }

    DMA_MSG((_T("[DMA] --DMA_free_LLI_context() : %d\n\r"), bRet));

    return bRet;
}

static BOOL DMA_lock(void)
{
    DWORD dwRet;

    dwRet = WaitForSingleObject(g_hMutex, INFINITE);
    if (dwRet != WAIT_OBJECT_0)
    {
        DMA_ERR((_T("[DMA] DMA_lock() : WaitForSingleObject() Failed\n\r")));
        return FALSE;
    }

    return TRUE;
}

static BOOL DMA_unlock(void)
{
    BOOL bRet;

    bRet = ReleaseMutex(g_hMutex);
    if (bRet == FALSE)
    {
        DMA_ERR((_T("[DMA] DMA_unlock() : ReleaseMutex() Failed\n\r")));
        return FALSE;
    }

    return TRUE;
}

