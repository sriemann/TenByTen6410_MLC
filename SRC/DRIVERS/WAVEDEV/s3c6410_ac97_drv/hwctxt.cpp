//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.


Module Name:    HWCTXT.CPP

Abstract:        Platform dependent code for the mixing audio driver.

Notes:            The following file contains all the hardware specific code
                for the mixing audio driver.  This code's primary responsibilities
                are:

                    * Initialize audio hardware (including codec chip)
                    * Schedule DMA operations (move data from/to buffers)
                    * Handle audio interrupts

                All other tasks (mixing, volume control, etc.) are handled by the "upper"
                layers of this driver.

                ****** IMPORTANT ******
                For the S3C6410 CPU, DMA channel 2 can be used for both input and output.  In this,
                configuration, however, only one type operation (input or output) can execute.  In
                order to implement simultaneous playback and recording, two things must be done:

                    1) Input DMA should be moved to DMA Channel 1; Output DMA still uses DMA Channel 2.
                    2) Step #3 in InterruptThread() needs to be implemented so that the DMA interrupt
                       source (input DMA or output DMA?) can be determined.  The interrupt source needs
                       to be determined so that the appropriate buffers can be copied (Steps #4,#5...etc.).

                Lastly, the m_OutputDMAStatus and m_InputDMAStatus variables shouldn't need to be modified.
                The logic surrounding these drivers is simply used to determine which buffer (A or B) needs
                processing.

-*/

#include "wavemain.h"
#include <ceddk.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "WM9713.h"
#include "s3c6410_ac97_interface.h"
#include "s3c6410_dma_controller.h"
#include "hwctxt.h"

typedef enum
{
    DMA_CH_OUT    = 0x1,
    DMA_CH_IN        = 0x2
} DMA_CH_SELECT;


#define WAV_MSG(x)
#define WAV_INF(x)    DEBUGMSG(ZONE_FUNCTION, x)
#define WAV_ERR(x)    DEBUGMSG(ZONE_ERROR, x)


#define INTERRUPT_THREAD_PRIORITY_DEFAULT    (150)

HardwareContext *g_pHWContext        = NULL;

static volatile S3C6410_AC97_REG    *g_pAC97Reg = NULL;
static volatile S3C6410_GPIO_REG    *g_pGPIOReg = NULL;
static volatile S3C6410_DMAC_REG    *g_pDMAC0Reg = NULL;
static volatile S3C6410_DMAC_REG    *g_pDMAC1Reg = NULL;
static volatile S3C6410_SYSCON_REG    *g_pSysConReg = NULL;

static DMA_CH_CONTEXT    g_OutputDMA;
static DMA_CH_CONTEXT    g_InputDMA;
static PHYSICAL_ADDRESS    g_PhyDMABufferAddr;


BOOL
HardwareContext::CreateHWContext(DWORD Index)
{
    if (g_pHWContext)
    {
        return(TRUE);
    }

    g_pHWContext = new HardwareContext;
    if (g_pHWContext == NULL)
    {
        return(FALSE);
    }

    return(g_pHWContext->Initialize(Index));
}


HardwareContext::HardwareContext()
: m_InputDeviceContext(), m_OutputDeviceContext()
{
    InitializeCriticalSection(&m_csLock);
    m_bInitialized = FALSE;
}


HardwareContext::~HardwareContext()
{
    DeleteCriticalSection(&m_csLock);
}


BOOL
HardwareContext::Initialize(DWORD Index)
{
    BOOL bRet;

    if (m_bInitialized)
    {
        return(FALSE);
    }

    m_DriverIndex = Index;
    m_InPowerHandler = FALSE;

    m_bOutputDMARunning = FALSE;
    m_bInputDMARunning = FALSE;
    m_bSavedInputDMARunning = FALSE;
    m_bSavedOutputDMARunning = FALSE;
    m_InputDMAStatus    = DMA_CLEAR;
    m_OutputDMAStatus = DMA_CLEAR;
    m_nOutByte[OUTPUT_DMA_BUFFER0] = 0;
    m_nOutByte[OUTPUT_DMA_BUFFER1] = 0;
    m_nInByte[INPUT_DMA_BUFFER0] = 0;
    m_nInByte[INPUT_DMA_BUFFER1] = 0;

    m_dwSysintrOutput = NULL;
    m_dwSysintrInput = NULL;
    m_hOutputDMAInterrupt = NULL;
    m_hInputDMAInterrupt = NULL;
    m_hOutputDMAInterruptThread = NULL;
    m_hInputDMAInterruptThread = NULL;

    m_dwOutputGain = 0xFFFF;
    m_dwInputGain = 0xFFFF;
    m_bOutputMute = FALSE;
    m_bInputMute = FALSE;

    m_NumForcedSpeaker = 0;

    // Map Virtual Address for SFR
    bRet = MapRegisters();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : MapRegisters() Failed\n\r")));
        goto CleanUp;
    }

    // Allocation and Map DMA Buffer
    bRet = MapDMABuffers();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : MapDMABuffers() Failed\n\r")));
        goto CleanUp;
    }

    // Enable Clock for AC97
    g_pSysConReg->PCLK_GATE |= (1<<14);

    // WM9714 Codec Power On
    CodecPowerOn();

    // Initialize SFR address for PDD Library
    AC97_initialize_register_address((void *)g_pAC97Reg, (void *)g_pGPIOReg);
    DMA_initialize_register_address((void *)g_pDMAC0Reg, (void *)g_pDMAC1Reg, (void *)g_pSysConReg);

    // Initialize AC97 Interface
    bRet = InitAC97();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitAC97() Failed\n\r")));
        goto CleanUp;
    }

    // Initialize Audio Codec
    bRet = InitCodec();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitCodec() Failed\n\r")));
        goto CleanUp;
    }

    // Request DMA Channel and Initialize
    // DMA context have Virtual IRQ Number of Allocated DMA Channel
    // You Should initialize Interrupt after DMA initialization
    bRet = InitOutputDMA();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitOutputDMA() Failed\n\r")));
        goto CleanUp;
    }

    bRet = InitInputDMA();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitInputDMA() Failed\n\r")));
        goto CleanUp;
    }

    // Initialize Interrupt
    bRet = InitInterruptThread();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] Initialize() : InitInterruptThread() Failed\n\r")));
        goto CleanUp;
    }

    // Set HwCtxt Initialize Flag
    m_bInitialized = TRUE;

    //-----------------------------------------------
    // Power Manager expects us to init in D0.
    // We are normally in D4 unless we are opened for play.
    // Inform the PM.
    //-----------------------------------------------
    m_Dx = D0;
    DevicePowerNotify(_T("WAV1:"),(_CEDEVICE_POWER_STATE)D4, POWER_NAME);

CleanUp:

    return bRet;
}


BOOL
HardwareContext::Deinitialize()
{
    if (m_bInitialized)
    {
        DeinitInterruptThread();

        StopOutputDMA();
        StopInputDMA();
        DMA_release_channel(&g_OutputDMA);
        DMA_release_channel(&g_InputDMA);

        CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);

        UnMapDMABuffers();
        UnMapRegisters();
    }

    return TRUE;
}


void
HardwareContext::PowerUp()
{
    WAV_MSG((_T("[WAV] ++PowerUp()\n\r")));

    // WM9714 Codec Power On
    CodecPowerOn();

    // Enable Clock for AC97
    g_pSysConReg->PCLK_GATE |= (1<<14);

    InitAC97();

    InitCodec();

    CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);

    WAV_MSG((_T("[WAV] --PowerUp()\n\r")));
}


void HardwareContext::PowerDown()
{
    WAV_MSG((_T("[WAV] ++PowerDown()\n\r")));

    CodecMuteControl(DMA_CH_OUT|DMA_CH_IN, TRUE);
    CodecPowerControl();

    // Disable Clock for AC97
    g_pSysConReg->PCLK_GATE &= ~(1<<14);

    // WM9714 Codec Power On
    CodecPowerOff();

    WAV_MSG((_T("[WAV] --PowerDown()\n\r")));
}


DWORD
HardwareContext::Open(void)
{
    DWORD mmErr = MMSYSERR_NOERROR;
    DWORD dwErr;

    // Don't allow play when not on, if there is a power constraint upon us.
    if ( D0 != m_Dx )
    {
        // Tell the Power Manager we need to power up.
        // If there is a power constraint then fail.
        dwErr = DevicePowerNotify(_T("WAV1:"), D0, POWER_NAME);
        if ( ERROR_SUCCESS !=  dwErr )
        {
            WAV_ERR((_T("[WAV:ERR] Open() : DevicePowerNotify Error : %u\r\n"), dwErr));
            mmErr = MMSYSERR_ERROR;
        }
    }

    return mmErr;
}


DWORD
HardwareContext::Close(void)
{
    DWORD mmErr = MMSYSERR_NOERROR;
    DWORD dwErr;

    // we are done so inform Power Manager to power us down, 030711
    dwErr = DevicePowerNotify(_T("WAV1:"), (_CEDEVICE_POWER_STATE)D4, POWER_NAME);
    if ( ERROR_SUCCESS !=  dwErr )
    {
        WAV_ERR((_T("[WAV:ERR] Close() : DevicePowerNotify Error : %u\r\n"), dwErr));
        mmErr = MMSYSERR_ERROR;
    }

    return mmErr;
}


BOOL
HardwareContext::IOControl(
            DWORD  dwOpenData,
            DWORD  dwCode,
            PBYTE  pBufIn,
            DWORD  dwLenIn,
            PBYTE  pBufOut,
            DWORD  dwLenOut,
            PDWORD pdwActualOut)
{
    DWORD dwErr = ERROR_SUCCESS;
    BOOL  bRc = TRUE;

    UNREFERENCED_PARAMETER(dwOpenData);

    switch (dwCode)
    {
    //-----------------
    // Power Management
    //-----------------
    case IOCTL_POWER_CAPABILITIES:
        {
            PPOWER_CAPABILITIES ppc;

            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(POWER_CAPABILITIES)) )
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] IOCTL_POWER_CAPABILITIES : Invalid Parameter\n\r")));
                break;
            }

            ppc = (PPOWER_CAPABILITIES)pBufOut;

            memset(ppc, 0, sizeof(POWER_CAPABILITIES));

            ppc->DeviceDx = 0x11;    // support D0, D4
            ppc->WakeFromDx = 0x0;    // no wake
            ppc->InrushDx = 0x0;        // no inrush

            // REVIEW: Do we enable all these for normal playback?
            // D0: SPI + I2S + CODEC (Playback) + Headphone=
            //     0.5 mA + 0.5 mA + (23 mW, into BUGBUG ohms ) + (30 mW, into 32 ohms)
            //     500 uA + 500 uA + 23000 uA + 32000 uA
            ppc->Power[D0] = 56000;

            // Report our nominal power consumption in uAmps rather than mWatts.
            ppc->Flags = POWER_CAP_PREFIX_MICRO | POWER_CAP_UNIT_AMPS;

            *pdwActualOut = sizeof(POWER_CAPABILITIES);
        }
        break;

    case IOCTL_POWER_SET:
        {
            CEDEVICE_POWER_STATE NewDx;

            if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) )
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] CEDEVICE_POWER_STATE : Invalid Parameter\n\r")));
                break;
            }

            NewDx = *(PCEDEVICE_POWER_STATE)pBufOut;

            if ( VALID_DX(NewDx) )
            {
                // grab the CS since the normal Xxx_PowerXxx can not.
                switch (NewDx)
                {
                case D0:
                    if (m_Dx != D0)
                    {
                        m_Dx = D0;

                        PowerUp();

                        Lock();

                        if (m_bSavedOutputDMARunning)
                        {
                            SetInterruptEvent(m_dwSysintrOutput);
                        }

                        Unlock();
                    }
                    break;
                default:
                    if (m_Dx != (_CEDEVICE_POWER_STATE)D4)
                    {
                        // Save last DMA state before Power Down
                        m_bSavedInputDMARunning = m_bInputDMARunning;
                        m_bSavedOutputDMARunning = m_bOutputDMARunning;

                        m_Dx = (_CEDEVICE_POWER_STATE)D4;

                        Lock();

                        StopOutputDMA();
                        StopInputDMA();

                        Unlock();

                        PowerDown();
                    }
                    break;
                }

                // return our state
                *(PCEDEVICE_POWER_STATE)pBufOut = m_Dx;

                *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);

                WAV_INF((_T("[WAV:INF] IOCTL_POWER_SET -> [D%d]\n\r"), m_Dx));
            }
            else
            {
                bRc = FALSE;
                dwErr = ERROR_INVALID_PARAMETER;
                WAV_ERR((_T("[WAV:ERR] CEDEVICE_POWER_STATE : Invalid Parameter Dx\n\r")));
            }
        }
        break;

    case IOCTL_POWER_GET:
        if ( !pdwActualOut || !pBufOut || (dwLenOut < sizeof(CEDEVICE_POWER_STATE)) )
        {
            bRc = FALSE;
            dwErr = ERROR_INVALID_PARAMETER;
            break;
        }

        *(PCEDEVICE_POWER_STATE)pBufOut = m_Dx;

        WAV_INF((_T("WAVEDEV: IOCTL_POWER_GET: D%u \r\n"), m_Dx));

        *pdwActualOut = sizeof(CEDEVICE_POWER_STATE);
        break;

    default:
        bRc = FALSE;
        dwErr = ERROR_INVALID_FUNCTION;
        WAV_INF((_T(" Unsupported ioctl 0x%X\r\n"), dwCode));
        break;
    }

    if (!bRc)
    {
        SetLastError(dwErr);
    }

    return(bRc);
}


BOOL
HardwareContext::StartOutputDMA()
{
    ULONG OutputTransferred;

    WAV_MSG((_T("[WAV] StartOutputDMA()\r\n")));

    if((m_bOutputDMARunning == FALSE) && (m_Dx == D0))
    {
        m_bOutputDMARunning = TRUE;
        m_nOutByte[OUTPUT_DMA_BUFFER0] = 0;
        m_nOutByte[OUTPUT_DMA_BUFFER1] = 0;

        m_nOutputBufferInUse = OUTPUT_DMA_BUFFER0;    // Start DMA with Buffer 0
        m_OutputDMAStatus = (DMA_DONEA | DMA_DONEB) & ~DMA_BIU;
        OutputTransferred = TransferOutputBuffer(m_OutputDMAStatus);

        if(OutputTransferred)
        {
            CodecPowerControl();                    // Turn Output Channel
            CodecMuteControl(DMA_CH_OUT, FALSE);    // Unmute Output Channel

            // AC97 PCM output enable
            AC97_set_pcmout_transfer_mode(AC97_CH_DMA);

            // Output DMA Start
            DMA_set_channel_source(&g_OutputDMA, m_OutputDMABufferPhyPage[OUTPUT_DMA_BUFFER0], WORD_UNIT, BURST_1, INCREASE);
            DMA_set_channel_destination(&g_OutputDMA, AC97_get_pcmout_physical_buffer_address(), WORD_UNIT, BURST_1, FIXED);
            DMA_set_channel_transfer_size(&g_OutputDMA, AUDIO_DMA_PAGE_SIZE);
            DMA_set_initial_LLI(&g_OutputDMA, 1);
            DMA_channel_start(&g_OutputDMA);
        }
        else
        {
            WAV_ERR((_T("[WAV:ERR] StartOutputDMA() : There is no data to transfer\r\n")));
            m_bOutputDMARunning = FALSE;
        }
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


void
HardwareContext::StopOutputDMA()
{
    WAV_MSG((_T("[WAV] StopOutputDMA()\r\n")));

    if (m_bOutputDMARunning)
    {
        m_OutputDMAStatus = DMA_CLEAR;

        // Stop output DMA
        DMA_channel_stop(&g_OutputDMA);

        // AC97 PCM output disable
        AC97_set_pcmout_transfer_mode(AC97_CH_OFF);
    }

    m_bOutputDMARunning = FALSE;

    CodecMuteControl(DMA_CH_OUT, TRUE);
    CodecPowerControl();
}


BOOL
HardwareContext::StartInputDMA()
{
    WAV_MSG((_T("[WAV] StartInputDMA()\r\n")));

    if(m_bInputDMARunning == FALSE)
    {
        m_bInputDMARunning = TRUE;

        m_nInByte[INPUT_DMA_BUFFER0] = 0;
        m_nInByte[INPUT_DMA_BUFFER1] = 0;

        m_nInputBufferInUse = INPUT_DMA_BUFFER0;    // Start DMA with Buffer 0
        m_InputDMAStatus = (DMA_DONEA | DMA_DONEB) & ~DMA_BIU;

        CodecPowerControl();                    // Turn On Channel
        CodecMuteControl(DMA_CH_IN, FALSE);    // Unmute Input Channel

        // AC97 PCM input enable
        AC97_set_pcmin_transfer_mode(AC97_CH_DMA);

        DMA_set_channel_source(&g_InputDMA, AC97_get_pcmin_physical_buffer_address(), WORD_UNIT, BURST_1, FIXED);
        DMA_set_channel_destination(&g_InputDMA, m_InputDMABufferPhyPage[INPUT_DMA_BUFFER0], WORD_UNIT, BURST_1, INCREASE);
        DMA_set_channel_transfer_size(&g_InputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_set_initial_LLI(&g_InputDMA, 1);
        DMA_channel_start(&g_InputDMA);
    }
    else
    {
        return FALSE;
    }

    return TRUE;
}


void
HardwareContext::StopInputDMA()
{
    WAV_MSG((_T("[WAV] StopInputDMA()\r\n")));

    if (m_bInputDMARunning)
    {
        DMA_channel_stop(&g_InputDMA);
        AC97_set_pcmin_transfer_mode(AC97_CH_OFF);

        m_InputDMAStatus = DMA_CLEAR;
    }

    m_bInputDMARunning = FALSE;

    CodecMuteControl(DMA_CH_IN, TRUE);
    CodecPowerControl();
}


DWORD
HardwareContext::GetOutputGain (void)
{
    return m_dwOutputGain;
}


MMRESULT
HardwareContext::SetOutputGain (DWORD dwGain)
{
    WAV_MSG((_T("[WAV] SetOutputGain(0x%08x)\r\n"), dwGain));

    m_dwOutputGain = dwGain & 0xffff;    // save off so we can return this from GetGain - but only MONO

    // convert 16-bit gain to 5-bit attenuation
    UCHAR ucGain;
    if (m_dwOutputGain == 0)
    {
        ucGain = 0x3F; // mute: set maximum attenuation
    }
    else
    {
        ucGain = (UCHAR) ((0xffff - m_dwOutputGain) >> 11);    // codec supports 64dB attenuation, we'll only use 32
    }

    //ASSERT((ucGain & 0xC0) == 0); // bits 6,7 clear indicate DATA0 in Volume mode.

    return MMSYSERR_NOERROR;
}


DWORD
HardwareContext::GetInputGain (void)
{
    return m_dwInputGain;
}


MMRESULT
HardwareContext::SetInputGain (DWORD dwGain)
{
    WAV_MSG((_T("[WAV] SetInputGain(0x%08x)\r\n"), dwGain));

    m_dwInputGain = dwGain;

    if (!m_bInputMute)
    {
        m_InputDeviceContext.SetGain(dwGain);
    }

    return MMSYSERR_NOERROR;
}


BOOL
HardwareContext::GetOutputMute (void)
{
    return m_bOutputMute;
}


MMRESULT
HardwareContext::SetOutputMute (BOOL bMute)
{
    USHORT CodecReg;

    m_bOutputMute = bMute;

    CodecReg = ReadCodecRegister(WM9713_HEADPHONE_VOL);

    if (bMute)
    {
        CodecReg |= 0x8080;
    }
    else
    {
        CodecReg &= ~0x8080;
    }

    WriteCodecRegister(WM9713_HEADPHONE_VOL, CodecReg);

    return MMSYSERR_NOERROR;
}


BOOL
HardwareContext::GetInputMute (void)
{
    return m_bInputMute;
}


MMRESULT
HardwareContext::SetInputMute (BOOL bMute)
{
    m_bInputMute = bMute;
    return m_InputDeviceContext.SetGain(bMute ? 0: m_dwInputGain);
}


DWORD
HardwareContext::ForceSpeaker(BOOL bForceSpeaker)
{
    // If m_NumForcedSpeaker is non-zero, audio should be routed to an
    // external speaker (if hw permits).
    if (bForceSpeaker)
    {
        m_NumForcedSpeaker++;
        if (m_NumForcedSpeaker == 1)
        {
            SetSpeakerEnable(TRUE);
        }
    }
    else
    {
        m_NumForcedSpeaker--;
        if (m_NumForcedSpeaker ==0)
        {
            SetSpeakerEnable(FALSE);
        }
    }

    return MMSYSERR_NOERROR;
}


void
HardwareContext::InterruptThreadOutputDMA()
{
    ULONG OutputTransferred;

#if (_WIN32_WCE < 600)
    // Fast way to access embedded pointers in wave headers in other processes.
    SetProcPermissions((DWORD)-1);
#endif

    WAV_INF((_T("[WAV:INF] ++InterruptThreadOutputDMA()\n\r")));

    while(TRUE)
    {
        WaitForSingleObject(m_hOutputDMAInterrupt, INFINITE);

        Lock();

        __try
        {
            DMA_set_interrupt_mask(&g_OutputDMA);
            DMA_clear_interrupt_pending(&g_OutputDMA);

            InterruptDone(m_dwSysintrOutput);

            DMA_clear_interrupt_mask(&g_OutputDMA);

            if ( m_Dx == D0 )
            {
                // DMA Output Buffer is Changed by LLI
                if (m_nOutputBufferInUse == OUTPUT_DMA_BUFFER0)
                {
                    // Buffer0 DMA finished
                    // DMA start with Buffer 1
                    m_nOutputBufferInUse = OUTPUT_DMA_BUFFER1;
                }
                else
                {
                    // Buffer 1 DMA finished
                    // DMA start with Buffer 0
                    m_nOutputBufferInUse = OUTPUT_DMA_BUFFER0;
                }

                if(m_OutputDMAStatus & DMA_BIU)
                {
                    m_OutputDMAStatus &= ~DMA_STRTB;    // Buffer B just completed...
                    m_OutputDMAStatus |= DMA_DONEB;
                    m_OutputDMAStatus &= ~DMA_BIU;        // Buffer A is in use
                }
                else
                {
                    m_OutputDMAStatus &= ~DMA_STRTA;    // Buffer A just completed...
                    m_OutputDMAStatus |= DMA_DONEA;
                    m_OutputDMAStatus |= DMA_BIU;        // Buffer B is in use
                }

                OutputTransferred = TransferOutputBuffer(m_OutputDMAStatus);

                if (m_bSavedOutputDMARunning)
                {
                    m_bSavedOutputDMARunning = FALSE;
                    StartOutputDMA();        // for DMA resume when wake up
                }

            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WAV_ERR((_T("WAVDEV2.DLL:InterruptThreadOutputDMA() - EXCEPTION: %d"), GetExceptionCode()));
        }

        Unlock();
    }

    WAV_INF((_T("[WAV:INF] --InterruptThreadOutputDMA()\n\r")));
}


void
HardwareContext::InterruptThreadInputDMA()
{
    ULONG InputTransferred;        // How can I use it ???

#if (_WIN32_WCE < 600)
    // Fast way to access embedded pointers in wave headers in other processes.
    SetProcPermissions((DWORD)-1);
#endif

    WAV_INF((_T("[WAV:INF] ++InterruptThreadInputDMA()\n\r")));

    while(TRUE)
    {
        WaitForSingleObject(m_hInputDMAInterrupt, INFINITE);

        Lock();

        __try
        {
            DMA_set_interrupt_mask(&g_InputDMA);
            DMA_clear_interrupt_pending(&g_InputDMA);

            InterruptDone(m_dwSysintrInput);

            DMA_clear_interrupt_mask(&g_InputDMA);

            if ( m_Dx == D0 )
            {
                if (m_nInputBufferInUse == INPUT_DMA_BUFFER0)
                {
                    // Buffer0 DMA finished
                    // DMA start with Buffer 1
                    m_nInputBufferInUse = INPUT_DMA_BUFFER1;
                }
                else
                {
                    // Buffer 1 DMA finished
                    // DMA start with Buffer 0
                    m_nInputBufferInUse = INPUT_DMA_BUFFER0;
                }

                if(m_InputDMAStatus & DMA_BIU)
                {
                    m_InputDMAStatus &= ~DMA_STRTB;        // Buffer B just completed...
                    m_InputDMAStatus |= DMA_DONEB;
                    m_InputDMAStatus &= ~DMA_BIU;        // Buffer A is in use
                }
                else
                {
                    m_InputDMAStatus &= ~DMA_STRTA;        // Buffer A just completed...
                    m_InputDMAStatus |= DMA_DONEA;
                    m_InputDMAStatus |= DMA_BIU;            // Buffer B is in use
                }

                InputTransferred = TransferInputBuffers(m_InputDMAStatus);
                WAV_INF((_T("[WAV:INF] InputTransferred = %d\n\r"), InputTransferred));
            }
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            WAV_ERR((_T("WAVDEV2.DLL:InterruptThreadInputDMA() - EXCEPTION: %d"), GetExceptionCode()));
        }

        Unlock();
    }

    WAV_INF((_T("[WAV:INF] --InterruptThreadInputDMA()\n\r")));
}


BOOL
HardwareContext::MapRegisters()
{
    BOOL bRet = TRUE;
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

    WAV_MSG((_T("[WAV] ++MapRegisters()\n\r")));

    // Alloc and Map GPIO SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
    g_pGPIOReg = (S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
    if (g_pGPIOReg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pGPIOReg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map DMAC0 SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA0;    
    g_pDMAC0Reg = (S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (g_pDMAC0Reg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pDMAC0Reg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map DMAC1 SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_DMA1;
    g_pDMAC1Reg = (S3C6410_DMAC_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_DMAC_REG), FALSE);
    if (g_pDMAC1Reg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pDMAC1Reg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map AC97 Interface SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_AC97;    
    g_pAC97Reg = (S3C6410_AC97_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_AC97_REG), FALSE);
    if (g_pAC97Reg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pAC97Reg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Alloc and Map System Controller SFR
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;      
    g_pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
    if (g_pSysConReg == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] MapRegisters() : g_pSysConReg MmMapIoSpace() Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

CleanUp:

    if (bRet == FALSE)
    {
        UnMapRegisters();
    }

    WAV_MSG((_T("[WAV] --MapRegisters()\n\r")));

    return(TRUE);
}


BOOL
HardwareContext::UnMapRegisters()
{
    WAV_MSG((_T("[WAV] UnMapRegisters()\n\r")));

    if (g_pGPIOReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pGPIOReg, sizeof(S3C6410_GPIO_REG));
        g_pGPIOReg = NULL;
    }

    if (g_pAC97Reg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pAC97Reg, sizeof(S3C6410_AC97_REG));
        g_pAC97Reg = NULL;
    }

    if (g_pDMAC0Reg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pDMAC0Reg, sizeof(S3C6410_DMAC_REG));
        g_pDMAC0Reg = NULL;
    }

    if (g_pDMAC1Reg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pDMAC1Reg, sizeof(S3C6410_DMAC_REG));
        g_pDMAC1Reg = NULL;
    }

    if (g_pSysConReg != NULL)
    {
        MmUnmapIoSpace((PVOID)g_pSysConReg, sizeof(S3C6410_SYSCON_REG));
        g_pSysConReg = NULL;
    }

    return TRUE;
}


BOOL
HardwareContext::MapDMABuffers()
{
    PVOID pVirtDMABufferAddr = NULL;
    DMA_ADAPTER_OBJECT Adapter;
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++MapDMABuffers()\n\r")));

    memset(&Adapter, 0, sizeof(DMA_ADAPTER_OBJECT));
    Adapter.ObjectSize = sizeof(DMA_ADAPTER_OBJECT);
    Adapter.InterfaceType = Internal;

    // Allocate DMA Buffer
    pVirtDMABufferAddr = HalAllocateCommonBuffer(&Adapter, AUDIO_DMA_BUFFER_SIZE, &g_PhyDMABufferAddr, FALSE);
    if (pVirtDMABufferAddr == NULL)
    {
        WAV_MSG((_T("[WAV:ERR] MapDMABuffers() : DMA Buffer Allocation Failed\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    // Setup the Physical Address of DMA Buffer Page Address
    m_OutputDMABufferPhyPage[0] = (UINT32)g_PhyDMABufferAddr.LowPart;
    m_OutputDMABufferPhyPage[1] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE);
    m_InputDMABufferPhyPage[0] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE*2);
    m_InputDMABufferPhyPage[1] = (UINT32)(g_PhyDMABufferAddr.LowPart+AUDIO_DMA_PAGE_SIZE*3);

    // Setup the Virtual Address of DMA Buffer Page Address
    m_OutputDMABufferVirPage[0] = (PBYTE)pVirtDMABufferAddr;
    m_OutputDMABufferVirPage[1] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE);
    m_InputDMABufferVirPage[0] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE*2);
    m_InputDMABufferVirPage[1] = (PBYTE)((UINT32)pVirtDMABufferAddr+AUDIO_DMA_PAGE_SIZE*3);

CleanUp:

    WAV_MSG((_T("[WAV] --MapDMABuffers() : %d\n\r"), bRet));

    return bRet;
}


BOOL
HardwareContext::UnMapDMABuffers()
{
    WAV_MSG((_T("[WAV] UnMapDMABuffers()\n\r")));

    if(m_OutputDMABufferVirPage[0])
    {
        PHYSICAL_ADDRESS PhysicalAddress;
        PhysicalAddress.LowPart = m_OutputDMABufferPhyPage[0];    // No Meaning just for compile

        HalFreeCommonBuffer(0, 0, PhysicalAddress, (PVOID)m_OutputDMABufferVirPage[0], FALSE);

        m_OutputDMABufferVirPage[0] = NULL;
        m_OutputDMABufferVirPage[1] = NULL;
        m_InputDMABufferVirPage[0] = NULL;
        m_InputDMABufferVirPage[1] = NULL;
    }

    return TRUE;
}


void
HardwareContext::CodecPowerOn()
{
    WAV_MSG((_T("[WAV] ++CodecPowerOn() \n\r")));

    // Codec Power is no controllerable in SMDK6410

    WAV_MSG((_T("[WAV] --CodecPowerOn() \n\r")));
}


void
HardwareContext::CodecPowerOff()
{
    WAV_MSG((_T("[WAV] ++CodecPowerOff() \n\r")));

    // Codec Power is no controllerable in SMDK6410

    WAV_MSG((_T("[WAV] --CodecPowerOff() \n\r")));
}


BOOL
HardwareContext::InitAC97()
{
    BOOL bRet = FALSE;

    WAV_MSG((_T("[WAV] ++InitAC97() \n\r")));

    AC97_initialize_ACLink();

    // WM9713 Need This !!!!
    WriteCodecRegister(WM9713_POWER_CONTROL, 0x1000);

    AC97_enable_codec_ready_interrupt();

    AC97_enable_ACLink_data_transfer();

    bRet = AC97_wait_for_codec_ready();
    if (bRet == FALSE)
    {
        WAV_ERR((_T("[WAV:ERR] --InitAC97() : AC97 Initialize Failed\n\r")));
        return FALSE;
    }

    AC97_disable_all_interrupt();
    AC97_clear_all_interrupt();

    AC97_set_pcmout_transfer_mode(AC97_CH_OFF);
    AC97_set_pcmin_transfer_mode(AC97_CH_OFF);

    WAV_MSG((_T("[WAV] --InitAC97() \n\r")));

    return TRUE;
}


BOOL
HardwareContext::InitCodec()
{
    USHORT CodecVendorID0, CodecVendorID1;
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++InitCodec()\n\r")));

    WriteCodecRegister(WM9713_POWER_CONTROL, 0x0000);
    WriteCodecRegister(WM9713_POWERDOWN1, 0x0000);
    WriteCodecRegister(WM9713_POWERDOWN2, 0x0000);
    WriteCodecRegister(WM9713_MCLK_PLL_CTRL0, 0x0b80);

    WriteCodecRegister(WM9713_HEADPHONE_VOL, 0x0808);            // Unmute HPL, HPR
    WriteCodecRegister(WM9713_SPEAKER_VOL, 0x0);//test 
    WriteCodecRegister(WM9713_DAC_VOL_ROUTING, 0x0420);           // Unmute DAC to HPMix, SPKMix, MONOMix, 0 dB
    WriteCodecRegister (WM9713_LINEIN_VOL_ROUTING,0x108);

    WriteCodecRegister(WM9713_MIC_INPUT_SEL, 0x5C40);       
    WriteCodecRegister(WM9713_RECORD_ROUTING_MUX, 0x6B00);
	   	
    WriteCodecRegister(WM9713_RECORD_VOL, 0x0000);            // Unmute ADC input
    WriteCodecRegister(WM9713_OUTPUT_MUX, 0x1ca0);			   // Output HPL, HPR is HPMix, All others is Vmid

    WriteCodecRegister(WM9713_ADDITIONAL_FUNC2, 0x0080);        // AC97_ADDITIONAL_FUNC2    // Set DAC Auto-Mute, bit[1:0]=00, ADC Slot(L/R=3/4)
    WriteCodecRegister(WM9713_EXTED_AUDIOCTRL, 0x0031);         // bit[5:4]=11, SPDIF Output Slot(L/R=10/11), VRA Enabled

    //------------------------------------------------------------------------------
    // If using sampling rate other than 48KHz you must enable VRA before set sampling rate !!!!!
    //------------------------------------------------------------------------------
    WriteCodecRegister(WM9713_AUDIO_DAC_RATE, SAMPLERATE);        // 2Ch DAC Sample rate
    WriteCodecRegister(WM9713_AUDIO_ADC_RATE, SAMPLERATE);        // 32h ADC Sample rate
    WriteCodecRegister(WM9713_AUX_DAC_RATE, SAMPLERATE);         // AC97_AUXDAC_RATE    // 2Eh AUXDAC Sample rate

    CodecMuteControl(DMA_CH_OUT | DMA_CH_IN, FALSE);    // output, Input Mute
    CodecPowerControl();                                // ADC, DAC Power Off

    CodecVendorID0 = ReadCodecRegister(WM9713_VENDOR_ID1);
    CodecVendorID1 = ReadCodecRegister(WM9713_VENDOR_ID2);

    if (CodecVendorID0 != 0x574d || CodecVendorID1 != 0x4c13)
    {
        // 0x574D4C13 is VenderID of WM9713 Codec
        WAV_ERR((_T("[WAV:ERR] InitCodec() : VenderID Mismatch\n\r")));
        bRet = FALSE;
    }

    WAV_INF((_T("[WAV:INF] InitCodec() : VenderID = 0x%08x\n\r"), ((CodecVendorID0<<16) | CodecVendorID1)));

    WAV_MSG((_T("[WAV] --InitCodec()\n\r")));

    return bRet;
}


BOOL
HardwareContext::InitOutputDMA()
{
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++InitOutputDMA()\n\r")));

    if (!g_PhyDMABufferAddr.LowPart)
    {
        WAV_ERR((_T("[WAV:ERR] InitOutputDMA() : DMA Buffer is Not Allocated Yet\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    bRet = DMA_request_channel(&g_OutputDMA, DMA_AC97_PCMOUT);
    if (bRet)
    {
        DMA_initialize_channel(&g_OutputDMA, TRUE);
        DMA_set_channel_source(&g_OutputDMA, m_OutputDMABufferPhyPage[0], WORD_UNIT, BURST_1, INCREASE);
        DMA_set_channel_destination(&g_OutputDMA, AC97_get_pcmout_physical_buffer_address(), WORD_UNIT, BURST_1, FIXED);
        DMA_set_channel_transfer_size(&g_OutputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_initialize_LLI(&g_OutputDMA, 2);
        DMA_set_LLI_entry(&g_OutputDMA, 0, LLI_NEXT_ENTRY, m_OutputDMABufferPhyPage[0],
                            AC97_get_pcmout_physical_buffer_address(), AUDIO_DMA_PAGE_SIZE);
        DMA_set_LLI_entry(&g_OutputDMA, 1, LLI_FIRST_ENTRY, m_OutputDMABufferPhyPage[1],
                            AC97_get_pcmout_physical_buffer_address(), AUDIO_DMA_PAGE_SIZE);
        DMA_set_initial_LLI(&g_OutputDMA, 1);
    }

CleanUp:

    WAV_MSG((_T("[WAV] --InitOutputDMA()\n\r")));

    return bRet;
}


BOOL HardwareContext::InitInputDMA()
{
    BOOL bRet = TRUE;

    WAV_MSG((_T("[WAV] ++InitInputDMA()\n\r")));

    if (!g_PhyDMABufferAddr.LowPart)
    {
        WAV_ERR((_T("[WAV:ERR] InitInputDMA() : DMA Buffer is Not Allocated Yet\n\r")));
        bRet = FALSE;
        goto CleanUp;
    }

    bRet = DMA_request_channel(&g_InputDMA, DMA_AC97_PCMIN);
    if (bRet)
    {
        DMA_initialize_channel(&g_InputDMA, TRUE);
        DMA_set_channel_source(&g_InputDMA, AC97_get_pcmin_physical_buffer_address(), WORD_UNIT, BURST_1, FIXED);
        DMA_set_channel_destination(&g_InputDMA, m_InputDMABufferPhyPage[0], WORD_UNIT, BURST_1, INCREASE);
        DMA_set_channel_transfer_size(&g_InputDMA, AUDIO_DMA_PAGE_SIZE);
        DMA_initialize_LLI(&g_InputDMA, 2);
        DMA_set_initial_LLI(&g_InputDMA, 1);
        DMA_set_LLI_entry(&g_InputDMA, 0, LLI_NEXT_ENTRY, AC97_get_pcmin_physical_buffer_address(),
                            m_InputDMABufferPhyPage[0], AUDIO_DMA_PAGE_SIZE);
        DMA_set_LLI_entry(&g_InputDMA, 1, LLI_FIRST_ENTRY, AC97_get_pcmin_physical_buffer_address(),
                            m_InputDMABufferPhyPage[1], AUDIO_DMA_PAGE_SIZE);
    }

CleanUp:

    WAV_MSG((_T("[WAV] --InitInputDMA()\n\r")));

    return bRet;
}


BOOL
HardwareContext::InitInterruptThread()
{
    DWORD Irq;
    DWORD dwPriority;

    Irq = g_OutputDMA.dwIRQ;
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(DWORD), &m_dwSysintrOutput, sizeof(DWORD), NULL))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA IOCTL_HAL_REQUEST_SYSINTR Failed \n\r")));
        return FALSE;
    }

    Irq = g_InputDMA.dwIRQ;
    if (!KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR, &Irq, sizeof(DWORD), &m_dwSysintrInput, sizeof(DWORD), NULL))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA IOCTL_HAL_REQUEST_SYSINTR Failed \n\r")));
        return FALSE;
    }

    m_hOutputDMAInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hOutputDMAInterrupt == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA CreateEvent() Failed \n\r")));
        return(FALSE);
    }

    m_hInputDMAInterrupt = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (m_hInputDMAInterrupt == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA CreateEvent() Failed \n\r")));
        return(FALSE);
    }

    if (!InterruptInitialize(m_dwSysintrOutput, m_hOutputDMAInterrupt, NULL, 0))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA InterruptInitialize() Failed \n\r")));
        return FALSE;
    }

    if (! InterruptInitialize(m_dwSysintrInput, m_hInputDMAInterrupt, NULL, 0))
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA InterruptInitialize() Failed \n\r")));
        return FALSE;
    }


    m_hOutputDMAInterruptThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
                            (LPTHREAD_START_ROUTINE)CallInterruptThreadOutputDMA, this, 0, NULL);

    if (m_hOutputDMAInterruptThread == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Output DMA CreateThread() Failed \n\r")));
        return FALSE;
    }

    m_hInputDMAInterruptThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL, 0,
                            (LPTHREAD_START_ROUTINE)CallInterruptThreadInputDMA, this, 0, NULL);

    if (m_hInputDMAInterruptThread == NULL)
    {
        WAV_ERR((_T("[WAV:ERR] InitInterruptThread() : Input DMA CreateThread() Failed \n\r")));
        return FALSE;
    }

    dwPriority = GetInterruptThreadPriority();

    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(m_hOutputDMAInterruptThread, dwPriority);
    CeSetThreadPriority(m_hInputDMAInterruptThread, dwPriority);
    WAV_INF((_T("[WAV:INF] InitInterruptThread() : IST Priority = %d\n\r"), dwPriority));

    return(TRUE);
}


BOOL
HardwareContext::DeinitInterruptThread()
{
    return TRUE;
}


DWORD
HardwareContext::GetInterruptThreadPriority()
{
    HKEY hDevKey;
    DWORD dwValType;
    DWORD dwValLen;
    DWORD dwPrio = INTERRUPT_THREAD_PRIORITY_DEFAULT;
    LONG lResult;

    hDevKey = OpenDeviceKey((LPWSTR)m_DriverIndex);
    if (INVALID_HANDLE_VALUE != hDevKey)
    {
        dwValLen = sizeof(DWORD);
        lResult = RegQueryValueEx(hDevKey, TEXT("Priority256"), NULL, &dwValType, (PUCHAR)&dwPrio, &dwValLen);
        RegCloseKey(hDevKey);
    }
    else
    {
        WAV_ERR((_T("[WAV:ERR] GetInterruptThreadPriority() : OpenDeviceKey() Failed\n\r")));
    }

    return dwPrio;
}


ULONG
HardwareContext::TransferOutputBuffer(DWORD dwStatus)
{
    ULONG BytesTransferred = 0;
    ULONG BytesTotal = 0;

    dwStatus &= (DMA_DONEA|DMA_DONEB|DMA_BIU);

    WAV_MSG((_T("[WAV] TransferOutputBuffer(0x%08x)\n\r"), dwStatus));

    switch (dwStatus)
    {
    case 0:
    case DMA_BIU:
        // No done bits set- must not be my interrupt
        return 0;
    case DMA_DONEA|DMA_DONEB|DMA_BIU:
        // Load B, then A
        BytesTransferred = FillOutputBuffer(OUTPUT_DMA_BUFFER1);
        // fall through
    case DMA_DONEA: // This should never happen!
    case DMA_DONEA|DMA_BIU:
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER0);        // charlie, A => B
        break;
    case DMA_DONEA|DMA_DONEB:
        // Load A, then B
        BytesTransferred = FillOutputBuffer(OUTPUT_DMA_BUFFER0);
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER1);
        break;
    case DMA_DONEB|DMA_BIU: // This should never happen!
    case DMA_DONEB:
        // Load B
        BytesTransferred += FillOutputBuffer(OUTPUT_DMA_BUFFER1);        // charlie, B => A
        break;
    }

    // If it was our interrupt, but we weren't able to transfer any bytes
    // (e.g. no full buffers ready to be emptied)
    // and all the output DMA buffers are now empty, then stop the output DMA
    BytesTotal = m_nOutByte[OUTPUT_DMA_BUFFER0]+m_nOutByte[OUTPUT_DMA_BUFFER1];

    if (BytesTotal == 0)
    {
        StopOutputDMA();
    }

    return BytesTransferred;
}


ULONG
HardwareContext::FillOutputBuffer(int nBufferNumber)
{
    ULONG BytesTransferred = 0;
    PBYTE pBufferStart = m_OutputDMABufferVirPage[nBufferNumber];
    PBYTE pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE;
    PBYTE pBufferLast;

    WAV_MSG((_T("[WAV] FillOutputBuffer(%d)\n\r"), nBufferNumber));

    __try
    {
        pBufferLast = m_OutputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, NULL);

        BytesTransferred = pBufferLast-pBufferStart;
        m_nOutByte[nBufferNumber] = BytesTransferred;

        // Enable if you need to clear the rest of the DMA buffer
        StreamContext::ClearBuffer(pBufferLast, pBufferEnd);

        if(nBufferNumber == OUTPUT_DMA_BUFFER0)            // Output Buffer A
        {
            m_OutputDMAStatus &= ~DMA_DONEA;
            m_OutputDMAStatus |= DMA_STRTA;
        }
        else                                // Output Buffer B
        {
            m_OutputDMAStatus &= ~DMA_DONEB;
            m_OutputDMAStatus |= DMA_STRTB;
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WAV_ERR((_T("[WAV:ERR] FillOutputBuffer() : Exception ccurs [%d]\n\r"), GetExceptionCode()));
    }

    return BytesTransferred;
}


ULONG
HardwareContext::TransferInputBuffers(DWORD dwStatus)
{
    ULONG BytesTransferred=0;

    dwStatus &= (DMA_DONEA|DMA_DONEB|DMA_BIU);

    WAV_MSG((_T("[WAV] TransferInputBuffers(0x%08x)\n\r"), dwStatus));

    switch (dwStatus)
    {
    case 0:
    case DMA_BIU:
        // No done bits set- must not be my interrupt
        return 0;
    case DMA_DONEA|DMA_DONEB|DMA_BIU:
        // Load B, then A
        BytesTransferred = FillInputBuffer(INPUT_DMA_BUFFER1);
        // fall through
    case DMA_DONEA: // This should never happen!
    case DMA_DONEA|DMA_BIU:
        // Load A
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER0);
        break;
    case DMA_DONEA|DMA_DONEB:
        // Load A, then B
        BytesTransferred = FillInputBuffer(INPUT_DMA_BUFFER0);
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER1);
        break;
    case DMA_DONEB|DMA_BIU: // This should never happen!
    case DMA_DONEB:
        // Load B
        BytesTransferred += FillInputBuffer(INPUT_DMA_BUFFER1);
        break;
    }

    // If it was our interrupt, but we weren't able to transfer any bytes
    // (e.g. no empty buffers ready to be filled)
    // Then stop the input DMA
    if (BytesTransferred==0)
    {
        StopInputDMA();
    }

    return BytesTransferred;
}


ULONG
HardwareContext::FillInputBuffer(int nBufferNumber)
{
    ULONG BytesTransferred = 0;

    PBYTE pBufferStart = m_InputDMABufferVirPage[nBufferNumber];
    PBYTE pBufferEnd = pBufferStart + AUDIO_DMA_PAGE_SIZE;
    PBYTE pBufferLast;

    WAV_MSG((_T("[WAV] FillInputBuffer(%d)\n\r"), nBufferNumber));

    __try
    {
        pBufferLast = m_InputDeviceContext.TransferBuffer(pBufferStart, pBufferEnd, NULL);
        BytesTransferred = m_nInByte[nBufferNumber] = pBufferLast-pBufferStart;

        if(nBufferNumber == INPUT_DMA_BUFFER0)            // Input Buffer A
        {
            m_InputDMAStatus &= ~DMA_DONEA;
            m_InputDMAStatus |= DMA_STRTA;
        }
        else                                                // Input Buffer B
        {
            m_InputDMAStatus &= ~DMA_DONEB;
            m_InputDMAStatus |= DMA_STRTB;
        }

    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        WAV_ERR((_T("[WAV:ERR] FillInputBuffer() : Exception ccurs [%d]\n\r"), GetExceptionCode()));
    }

    return BytesTransferred;
}


void
HardwareContext::WriteCodecRegister(UCHAR Reg, USHORT Val)
{
    AC97_write_codec(Reg, Val);
}


USHORT
HardwareContext::ReadCodecRegister(UCHAR Reg)
{
    return AC97_read_codec(Reg);
}


BOOL
HardwareContext::CodecPowerControl()
{

    if( m_bInputDMARunning & m_bOutputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC On\n\r")));
        WriteCodecRegister(WM9713_POWER_CONTROL, AC97_PWR_D0);    // ADC, DAC power up
    }
    else if( m_bInputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC On\n\r")));
        WriteCodecRegister(WM9713_POWER_CONTROL, AC97_PWR_PR1);    // DAC power down
    }
    else if( m_bOutputDMARunning )
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() DAC On\n\r")));
        WriteCodecRegister(WM9713_POWER_CONTROL, AC97_PWR_PR0);    // ADC power down
    }
    else
    {
        WAV_MSG((_T("[WAV] CodecPowerControl() : CodecPowerControl() ADC & DAC Off\n\r")));
        WriteCodecRegister(WM9713_POWER_CONTROL, AC97_PWR_PR1|AC97_PWR_PR0);    // ADC, DAC power down
    }

    return(TRUE);
}


BOOL
HardwareContext::CodecMuteControl(DWORD channel, BOOL bMute)
{
    USHORT volume;

    if((channel & DMA_CH_OUT ))// && !m_bOutputDMARunning )
    {
        if(bMute)
        {
            volume = ReadCodecRegister(WM9713_HEADPHONE_VOL);
            WriteCodecRegister(WM9713_HEADPHONE_VOL, volume|0x8080);
        }
        else
        {
            volume = ReadCodecRegister(WM9713_HEADPHONE_VOL);
            WriteCodecRegister(WM9713_HEADPHONE_VOL, volume&~0x8080);
        }
    }

    if( (channel & DMA_CH_IN))// && !m_bInputDMARunning )
    {
        if(bMute)
        {
            volume = ReadCodecRegister(WM9713_RECORD_VOL);
            WriteCodecRegister(WM9713_RECORD_VOL, volume|0x8000);
        }
        else
        {
            volume = ReadCodecRegister(WM9713_RECORD_VOL);
            WriteCodecRegister(WM9713_RECORD_VOL, volume&~0x8000);
        }
    }

    return(TRUE);
}


void HardwareContext::SetSpeakerEnable(BOOL bEnable)
{
    // Code to turn speaker on/off here
    return;
}


void CallInterruptThreadOutputDMA(HardwareContext *pHWContext)
{
    pHWContext->InterruptThreadOutputDMA();
}


void CallInterruptThreadInputDMA(HardwareContext *pHWContext)
{
    pHWContext->InterruptThreadInputDMA();
}


