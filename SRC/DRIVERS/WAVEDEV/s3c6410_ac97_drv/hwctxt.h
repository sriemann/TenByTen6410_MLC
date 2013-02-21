//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
#pragma once
//
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.


Module Name:    HWCTXT.H

Abstract:        Platform dependent code for the mixing audio driver.

Environment:    Samsung S3C6410 CPU and Windows 5.0 (or later)

-*/

#include <pm.h>
#include <pmplatform.h>
#include <s3c6410.h>

#define OUTCHANNELS    (2)
#define INCHANNELS        (2)
#define BITSPERSAMPLE    (16)
#define SAMPLERATE        (44100)

// Inverse sample rate, in .32 fixed format, with 1 added at bottom to ensure round up.
#define INVSAMPLERATE ((UINT32)(((1i64<<32)/SAMPLERATE)+1))

typedef INT16 HWSAMPLE;
typedef HWSAMPLE *PHWSAMPLE;

// Set USE_MIX_SATURATE to 1 if you want the mixing code to guard against saturation
// This costs a couple of instructions in the inner loop
#define USE_MIX_SATURATE (1)

// The code will use the follwing values as saturation points
#define AUDIO_SAMPLE_MAX    (32767)
#define AUDIO_SAMPLE_MIN    (-32768)

typedef enum
{
    OUTPUT_DMA_BUFFER0 = 0,
    OUTPUT_DMA_BUFFER1,
    OUTPUT_DMA_BUFFER_COUNT
} OUTPUT_BUFFER_NUMBER;

typedef enum
{
    INPUT_DMA_BUFFER0 = 0,
    INPUT_DMA_BUFFER1,
    INPUT_DMA_BUFFER_COUNT
} INPUT_BUFFER_NUMBER;

#define AUDIO_DMA_PAGE_SIZE        (4096)                    // Size in bytes
#define AUDIO_DMA_BUFFER_SIZE        (AUDIO_DMA_PAGE_SIZE*(OUTPUT_DMA_BUFFER_COUNT+INPUT_DMA_BUFFER_COUNT))

//----- Used to track DMA controllers status -----
#define DMA_CLEAR            0x00000000
#define DMA_DONEA            0x00000008
#define DMA_STRTA            0x00000010
#define DMA_DONEB            0x00000020
#define DMA_STRTB            0x00000040
#define DMA_BIU                0x00000080    // Determines which buffer is in use: (A=0, B=1)


class HardwareContext
{
public:

    static BOOL CreateHWContext(DWORD Index);

    HardwareContext();

    ~HardwareContext();


    void Lock()
    {
        EnterCriticalSection(&m_csLock);
    }


    void Unlock()
    {
        LeaveCriticalSection(&m_csLock);
    }


    DWORD GetNumInputDevices()
    {
        return 1;
    }


    DWORD GetNumOutputDevices()
    {
        return 1;
    }


    DWORD GetNumMixerDevices()
    {
        return 1;
    }


    DeviceContext *GetInputDeviceContext(UINT DeviceId)
    {
        return &m_InputDeviceContext;
    }


    DeviceContext *GetOutputDeviceContext(UINT DeviceId)
    {
        return &m_OutputDeviceContext;
    }


    BOOL Initialize(DWORD Index);
    BOOL Deinitialize();

    void PowerUp();
    void PowerDown();

    DWORD Open();
    DWORD Close();


    BOOL IOControl( DWORD  dwOpenData,
                    DWORD  dwCode,
                    PBYTE  pBufIn,
                    DWORD  dwLenIn,
                    PBYTE  pBufOut,
                    DWORD  dwLenOut,
                    PDWORD pdwActualOut);


    BOOL StartOutputDMA();
    void StopOutputDMA();

    BOOL StartInputDMA();
    void StopInputDMA();

    DWORD       GetOutputGain (void);
    MMRESULT    SetOutputGain (DWORD dwVolume);
    DWORD       GetInputGain (void);
    MMRESULT    SetInputGain (DWORD dwVolume);

    BOOL        GetOutputMute (void);
    MMRESULT    SetOutputMute (BOOL fMute);
    BOOL        GetInputMute (void);
    MMRESULT    SetInputMute (BOOL fMute);

    DWORD ForceSpeaker (BOOL bSpeaker);

    void InterruptThreadOutputDMA();
    void InterruptThreadInputDMA();

    BOOL m_InPowerHandler;

protected:

    BOOL MapRegisters();
    BOOL UnMapRegisters();

    BOOL MapDMABuffers();
    BOOL UnMapDMABuffers();

    void CodecPowerOn();
    void CodecPowerOff();

    BOOL InitAC97();
    BOOL InitCodec();

    BOOL InitInputDMA();
    BOOL InitOutputDMA();

    BOOL InitInterruptThread();
    BOOL DeinitInterruptThread();
    DWORD GetInterruptThreadPriority();

    ULONG TransferOutputBuffer(DWORD dwStatus);
     ULONG FillOutputBuffer(int nBufferNumber);

    ULONG TransferInputBuffers(DWORD dwStatus);
    ULONG FillInputBuffer(int nBufferNumber);

    void WriteCodecRegister(UCHAR Reg, USHORT Val);
    USHORT ReadCodecRegister(UCHAR Reg);

    BOOL CodecPowerControl();
    BOOL CodecMuteControl(DWORD channel, BOOL bMute);

    void SetSpeakerEnable(BOOL bEnable);

    CEDEVICE_POWER_STATE GetDx() { return m_Dx;}

    UINT32 m_OutputDMABufferPhyPage[OUTPUT_DMA_BUFFER_COUNT];
    UINT32 m_InputDMABufferPhyPage[INPUT_DMA_BUFFER_COUNT];
    PBYTE m_OutputDMABufferVirPage[OUTPUT_DMA_BUFFER_COUNT];
    PBYTE m_InputDMABufferVirPage[INPUT_DMA_BUFFER_COUNT];

    BOOL m_bInputDMARunning;
    BOOL m_bOutputDMARunning;
    BOOL m_bSavedInputDMARunning;
    BOOL m_bSavedOutputDMARunning;
    int m_nOutputBufferInUse;
    int m_nInputBufferInUse;
    DWORD  m_OutputDMAStatus;
    DWORD  m_InputDMAStatus;
    ULONG m_nOutByte[OUTPUT_DMA_BUFFER_COUNT];
    ULONG m_nInByte[INPUT_DMA_BUFFER_COUNT];

    DWORD m_dwSysintrOutput;
    DWORD m_dwSysintrInput;

    HANDLE m_hOutputDMAInterrupt;
    HANDLE m_hInputDMAInterrupt;
    HANDLE m_hOutputDMAInterruptThread;
    HANDLE m_hInputDMAInterruptThread;

    InputDeviceContext m_InputDeviceContext;
    OutputDeviceContext m_OutputDeviceContext;

    DWORD m_DriverIndex;
    BOOL m_bInitialized;

    CRITICAL_SECTION m_csLock;

    DWORD m_dwOutputGain;
    DWORD m_dwInputGain;
    BOOL  m_bInputMute;
    BOOL  m_bOutputMute;

    LONG m_NumForcedSpeaker;

    CEDEVICE_POWER_STATE    m_Dx;
};

void CallInterruptThreadOutputDMA(HardwareContext *pHWContext);
void CallInterruptThreadInputDMA(HardwareContext *pHWContext);

extern HardwareContext *g_pHWContext;

