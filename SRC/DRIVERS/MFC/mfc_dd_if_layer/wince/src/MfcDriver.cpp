//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include <windows.h>
#include <nkintr.h>
#if (_WIN32_WCE >= 600)
#include <s3c6410_vintr.h>
#else
#include <oal_intr.h>
#endif
#include <pm.h>
#include <pmplatform.h>


#include "MfcDriver.h"
#include "MfcPwrMgmt.h"
#include "MfcSetClk.h"
#include "MfcDrvParams.h"

#include "MfcConfig.h"

#include "MFC_HW_Init.h"
#include "MFC_Instance.h"
#include "MFC_Inst_Pool.h"
#include "MfcSfr.h"
#include "DataBuf.h"

#include "LogMsg.h"

#include "MfcMutex.h"
#include "MfcIntrNotification.h"
#include "CacheOpr.h"


static HANDLE gMfcIntrEvent;
static HANDLE gMfcIntrThread;

static BOOL is_mfc_on = FALSE;
    
UINT32 g_MfcIrq     = IRQ_MFC;
UINT32 g_MfcSysIntr = SYSINTR_UNDEFINED;

static BOOL InitializeIST();
static LONG _openhandle_count = 0;


extern int MFC_GetConfigParams(MFCInstCtx  *pMfcInst, MFC_ARGS   *args);
extern int MFC_SetConfigParams(MFCInstCtx  *pMfcInst, MFC_ARGS   *args);


typedef struct _MFC_HANDLE
{
    MFCInstCtx  *mfc_inst;

#if (_WIN32_WCE >= 600)
    HANDLE          hUsrProc;    // Used for virtual address conversion (WCE600 or higher)
    unsigned char  *pStrmBuf;    // STRM_BUF pointer (virtual address of Application)
    unsigned char  *pFramBuf;    // FRAM_BUF pointer (virtual address of Application)
#endif

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
    unsigned char  *pMV;
    unsigned char  *pMBType;
#endif
} MFC_HANDLE;

static MFC_HANDLE   *gMfcHandlePower = NULL;



BOOL WINAPI
DllEntry(HANDLE hInstDll, DWORD dwReason, LPVOID lpvReserved)
{
    switch ( dwReason )
    {
    case DLL_PROCESS_ATTACH:

        DisableThreadLibraryCalls((HMODULE) hInstDll);
        break;

    }

    return (TRUE);
}

/*
** Function Name : MFC_Init
**
** Function Description : This function initialize MFC.
**                        MFC initailzation contains memory setup, clock configuration, HW initialization, IST initialization.
**                        First, You have to create mutex for mutual exclusive access of MFC.
*/
DWORD
MFC_Init(
    DWORD dwContext
    )
{
    /////////////////////////
    //  1. Mutex Creation  //
    /////////////////////////
    if (MFC_Mutex_Create() == FALSE)
    {
        // No CleanUp
        return FALSE;
    }

    ///////////////////////////
    //  2. MFC Memory Setup  //
    ///////////////////////////
    if (MFC_MemorySetup() == FALSE)
    {
        // CleanUp
        MFC_Mutex_Delete();
        return FALSE;
    }

    /////////////////////////////////////////
    //  3. MFC Clock divider configuration //
    /////////////////////////////////////////
    if (Mfc_Set_ClkDiv(MFC_CLOCK_DIVIDER_RATIO_HALF) == FALSE)
    {
        // CleanUp
        MFC_Mutex_Delete();
        return FALSE;
    }

    ///////////////////////////////////////////////////////
    //  4. IST(Interrupt Service Thread) Initialization  //
    ///////////////////////////////////////////////////////
    if (InitializeIST() == FALSE)
    {
        // CleanUp
        MFC_Mutex_Delete();
        return FALSE;
    }


    // Number of open handle
    _openhandle_count = 0;

    return MFC_INIT_SUCCESS;
}


/*
** Function Name : MFC_Deinit
**
** Function Description : This function de-initialize MFC
**                        In this case, You have to delete mutex only.
*/
BOOL
MFC_Deinit(
    DWORD InitHandle
    )
{
    // CleanUp IST
    if (g_MfcSysIntr != SYSINTR_UNDEFINED)
        InterruptDisable(g_MfcSysIntr);

    DeleteInterruptNotification();

    if (gMfcIntrEvent)
        CloseHandle(gMfcIntrEvent);

    // CleanUp Mutex
    MFC_Mutex_Delete();

    return TRUE;
}


/*
** Function Name : MFC_Open
**
** Function Description : This function open MFC instace and return instance handle.
*/
DWORD
MFC_Open(
    DWORD InitHandle,
    DWORD dwAccess,
    DWORD dwShareMode
    )
{
    MFC_HANDLE   *handle;

    // Mutex Lock
    MFC_Mutex_Lock();

    // Allocate & Clear MFC Handle
    handle = (MFC_HANDLE *) malloc(sizeof(MFC_HANDLE));
    if (!handle)
    {
        RETAILMSG(1, (L"\n[MFC_Open Error] Momory Allocation Fail.\n"));
        MFC_Mutex_Release();
        return 0;
    }
    memset(handle, 0, sizeof(MFC_HANDLE));

    // Increment OpenHandle Count
    InterlockedIncrement(&_openhandle_count);

    //
    if (_openhandle_count == 1) // Handle for Power Control
    {
        // Save Specific Handle for Power Control
        gMfcHandlePower = handle;
        RETAILMSG(1, (L"\n[MFC_Open] Power Manager Handle Opened...\n"));
    }
    else if (_openhandle_count >= 2) // Handle for User Application
    {
        // Create MFC Instance
        handle->mfc_inst = MFCInst_Create();
        if (!handle->mfc_inst)
        {
            RETAILMSG(1, (L"\n[MFC_Open Error] MFC Instance Creattion Fail.\n"));
            InterlockedDecrement(&_openhandle_count);
            free(handle);
            MFC_Mutex_Release();
            return 0;
        }

        if (_openhandle_count == 2) // First Handle for User Application
        {
            // MFC HW Init
            Mfc_Pwr_On();
            Mfc_Clk_On();

            if (MFC_HW_Init() == FALSE)
            {
                Mfc_Clk_Off();
                Mfc_Pwr_Off();
                MFCInst_Delete(handle->mfc_inst);
                InterlockedDecrement(&_openhandle_count);
                MFC_Mutex_Release();
                return 0;
            }
            Mfc_Clk_Off();
        }
    }

    // Mutex Release
    MFC_Mutex_Release();

    return (DWORD) handle;
}


/*
** Function Name : MFC_Close
**
** Function Description : This function close MFC instance.
**                        The instance handle and memory block free here.
*/
BOOL
MFC_Close(
    DWORD OpenHandle
    )
{
    MFC_HANDLE *handle;
    BOOL        ret;

    handle = (MFC_HANDLE *) OpenHandle;
    if (handle == NULL)
        return FALSE;

    if(handle->pStrmBuf)
    {
        ret = VirtualFreeEx(handle->hUsrProc,    // HANDLE hProcess
                          handle->pStrmBuf,
                          0,
                          MEM_RELEASE);
        if (ret == FALSE)
            RETAILMSG(1, (L"\n[MFC_Close] VirtualFreeEx(STRM_BUF) returns FALSE.\n"));
    }

    if(handle->pFramBuf)
    {
        ret = VirtualFreeEx(handle->hUsrProc,    // HANDLE hProcess
                              handle->pFramBuf,
                              0,
                              MEM_RELEASE);
        if (ret == FALSE)
            RETAILMSG(1, (L"\n[MFC_Close] VirtualFreeEx(FRAM_BUF) returns FALSE.\n"));
    }

    if (handle->mfc_inst)
    {
        MFCInst_Delete(handle->mfc_inst);
    }

    free(handle);

    // Decrement OpenHandle Count
    InterlockedDecrement(&_openhandle_count);

    if (_openhandle_count == 1)
    {   // Remain Power Control handle only.
        // MFC is now sw-reset.
        Mfc_Clk_On();
        MfcReset();
        Mfc_Clk_Off();

        // MFC Power Off
        Mfc_Pwr_Off();
    }
    else if (_openhandle_count == 0)
    {
        RETAILMSG(1, (L"\n[MFC_Close] Power Manager Handle closed...\n"));
        gMfcHandlePower = NULL;
    }

    return TRUE;
}

/*
** Function Name : process_MFC_PowerUp
**
** Function Description : This function Power up MFC.
*/
CEDEVICE_POWER_STATE
process_MFC_PowerUp(DWORD OpenHandle, PCEDEVICE_POWER_STATE mfc_pwr_state)
{
    unsigned int            nNumOfInstance = 0;            

    if ( *mfc_pwr_state == D4)
    {
        for (int inst_no = 0; inst_no < MFC_NUM_INSTANCES_MAX; inst_no++) 
        {
                MFCInstCtx *mfcinst_ctx = MFCInst_GetCtx(inst_no);
                if (mfcinst_ctx) {
                    nNumOfInstance++;
                    // On Power Up, the state of the MFC instance is recovered.
                    MFCInst_PowerOnState(mfcinst_ctx);
                }
            }
               LOG_MSG(LOG_TRACE, "MFC_IOControl", "[MFC IOCTL_POWER_SET] nNumOfInstance = 0x%X \r\n",nNumOfInstance);

        if (is_mfc_on)
        {
            Mfc_Pwr_On();
            Mfc_Clk_On();
            MFC_Wakeup();
            Mfc_Clk_Off();

            LOG_MSG(LOG_TRACE, "MFC_IOControl", "[MFC IOCTL_POWER_SET] POWER UP, handle = 0x%X \r\n",(DWORD) OpenHandle);
            }
        }

    *mfc_pwr_state = D0;    // MFC power state is now changed to D0(Power On) state.

    RETAILMSG(1, (L"[MFC] WakeUP! (NumOfInstance=%d) \r\n", nNumOfInstance));

    return *mfc_pwr_state;
}

/*
** Function Name : process_MFC_PowerDown
**
** Function Description : This function Power down MFC.
*/
CEDEVICE_POWER_STATE
process_MFC_PowerDown(DWORD OpenHandle, PCEDEVICE_POWER_STATE mfc_pwr_state)
{
    unsigned int            nNumOfInstance = 0;            

    if ( *mfc_pwr_state == D0)
    {
        is_mfc_on = FALSE;
        // Check if MFC is being used
        // and change the MFC instance state to PowerOff
        for (int inst_no = 0; inst_no < MFC_NUM_INSTANCES_MAX; inst_no++) 
        {
            MFCInstCtx *mfcinst_ctx = MFCInst_GetCtx(inst_no);

            if (mfcinst_ctx)
            {
                nNumOfInstance++;
                   LOG_MSG(LOG_TRACE, "MFC_IOControl", "[MFC IOCTL_POWER_SET] MFC State = 0x%X \r\n",(DWORD) (MFCINST_STATE(mfcinst_ctx)));        
                // On Power Down, the MFC instance is invalidated.
                // Then the MFC operations (DEC_EXE, ENC_EXE, etc.) will not be performed
                // until it is validated by entering Power Up state transition.
                MFCInst_PowerOffState(mfcinst_ctx);
                if (MFCINST_STATE_CHECK(mfcinst_ctx, MFCINST_STATE_CREATED) || MFCINST_STATE_CHECK(mfcinst_ctx, MFCINST_STATE_DELETED))
                {    
                       LOG_MSG(LOG_TRACE, "MFC_IOControl", "[MFC IOCTL_POWER_SET] MFC doesn't working!!! MFC State = 0x%X \r\n",(DWORD) (MFCINST_STATE(mfcinst_ctx)));        
                }
                else
                {
                is_mfc_on = TRUE;
            }
        }
    }

        if (is_mfc_on)
        {
            Mfc_Clk_On();
            MFC_Sleep();
            // Clock & Power off the MFC block if they are on.
            Mfc_Clk_Off();

            LOG_MSG(LOG_TRACE, "MFC_IOControl", "[MFC IOCTL_POWER_SET] POWER DOWN, handle = 0x%X \r\n",(DWORD) OpenHandle);
        }

        }

    *mfc_pwr_state = D4;    // MFC power state is now changed to D0(Power On) state.

    RETAILMSG(1, (L"[MFC] Sleep! (NumOfInstance=%d) \r\n", nNumOfInstance));

    return *mfc_pwr_state;
}

/*
** Function Name : MFC_IOControl
**
** Function Description : This function support any process of MFC instance.
*/
BOOL
MFC_IOControl(
    DWORD OpenHandle,
    DWORD dwIoControlCode,
    PBYTE pInBuf,
    DWORD nInBufSize,
    PBYTE pOutBuf,
    DWORD nOutBufSize,
    PDWORD pBytesReturned
    )
{
    MFC_HANDLE  *handle;
    MFCInstCtx     *pMfcInst;
    MFC_ARGS      *args;

    int ret                       = MFCINST_RET_OK;
    unsigned char  *p_buf         = NULL;
    int n_bufsize                 = 0;
    PVOID pMarshalledInBuf        = NULL;
    static CEDEVICE_POWER_STATE   mfc_pwr_state;
  
    BOOL    result = TRUE;

    handle = (MFC_HANDLE *) OpenHandle;

    /////////////////////
    // Parameter Check //
    /////////////////////
    if (handle == NULL)
    {
        LOG_MSG(LOG_TRACE, "MFC_IOControl", "OpenHandle == NULL\n");
        return FALSE;
    }

    if (handle != gMfcHandlePower)
    {
        if (pInBuf == NULL)
        {
            LOG_MSG(LOG_TRACE, "MFC_IOControl", "pInBuf == NULL\n");
            return FALSE;
        }
        if (nInBufSize == 0)
        {
            LOG_MSG(LOG_TRACE, "MFC_IOControl", "nInBufSize == 0\n");
            return FALSE;
        }

        if ((pOutBuf != NULL) || (nOutBufSize != 0) || (pBytesReturned != NULL))
        {
            LOG_MSG(LOG_TRACE, "MFC_IOControl", "others.....\n");
            return FALSE;
        }
    }

    pMfcInst = handle->mfc_inst;

    MFC_Mutex_Lock();

    switch ( dwIoControlCode )
    {
    case IOCTL_POWER_CAPABILITIES:
        {
            RETAILMSG(1, (L"[MFC IOCTL_POWER_CAPABILITIES]\n"));

            PPOWER_CAPABILITIES ppc;

            if ( !pBytesReturned || !pOutBuf || (nOutBufSize < sizeof(POWER_CAPABILITIES)) ) {
                SetLastError (ERROR_INVALID_PARAMETER);
                MFC_Mutex_Release();
                return FALSE;
            }
            
            __try
            {
                ppc = (PPOWER_CAPABILITIES)pOutBuf;
                memset(ppc, 0, sizeof(POWER_CAPABILITIES));

                // support D0, D4
                ppc->DeviceDx = DX_MASK(D0) | DX_MASK(D4);

                // no wake
                // no inrush

                // Report our nominal power consumption in uAmps rather than mWatts.
                ppc->Flags = POWER_CAP_PREFIX_MICRO | POWER_CAP_UNIT_AMPS;

                *pBytesReturned = sizeof(POWER_CAPABILITIES);

                RETAILMSG(1, (L"[MFC IOCTL_POWER_CAPABILITIES] leaving...\n"));
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
                RETAILMSG(1, (L"[MFC IOCTL_POWER_CAPABILITIES] exception...\n"));
                MFC_Mutex_Release();
                return FALSE;
            }
            break;
        }

    case IOCTL_POWER_SET:
        CEDEVICE_POWER_STATE NewDx;

        //if caller is not kernel mode, do not allow setting power state
        if (GetDirectCallerProcessId() != GetCurrentProcessId())
        {
            RETAILMSG(1, (L"[MFC IOCTL_POWER_SET] User mode access denied\r\n"));
            MFC_Mutex_Release();
            return ERROR_ACCESS_DENIED;
        }

        __try
        {
            if (pOutBuf == NULL)
            {
                return FALSE;
            }
            
            NewDx = *(PCEDEVICE_POWER_STATE) pOutBuf;
            switch ( NewDx )
            {
            case D0:    // Power Up
                    *(PCEDEVICE_POWER_STATE)pOutBuf = process_MFC_PowerUp(OpenHandle, &mfc_pwr_state);
                break;

            case D4:    // Power Down
                    *(PCEDEVICE_POWER_STATE)pOutBuf = process_MFC_PowerDown(OpenHandle, &mfc_pwr_state);
                break;

            default:
                MFC_Mutex_Release();
                return FALSE;
            }

            *pBytesReturned = sizeof(CEDEVICE_POWER_STATE);
        }
        __except(EXCEPTION_EXECUTE_HANDLER)
        {
            RETAILMSG(1, (L"[MFC IOCTL_POWER_SET] exception...\n"));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;

    case IOCTL_MFC_MPEG4_ENC_INIT:
    case IOCTL_MFC_H264_ENC_INIT:
    case IOCTL_MFC_H263_ENC_INIT:
        {
            MFC_CODECMODE   codec_mode;
            enc_info_t      enc_info;

            if (dwIoControlCode == IOCTL_MFC_MPEG4_ENC_INIT)
                codec_mode = MP4_ENC;
            else if (dwIoControlCode == IOCTL_MFC_H264_ENC_INIT)
                codec_mode = AVC_ENC;
            else
                codec_mode = H263_ENC;

            if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_H263_ENC_INIT.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            args = (MFC_ARGS *)pMarshalledInBuf;

            // Input arguments for IOCTL_MFC_xxx_ENC_INIT
            enc_info.width        = args->enc_init.in_width;
            enc_info.height        = args->enc_init.in_height;
            enc_info.frameRateRes    = args->enc_init.in_frameRateRes;
            enc_info.frameRateDiv    = args->enc_init.in_frameRateDiv;
            enc_info.gopNum        = args->enc_init.in_gopNum;
            enc_info.bitrate        = args->enc_init.in_bitrate;

            enc_info.intraqp        = args->enc_init.in_intraqp;
            enc_info.qpmax        = args->enc_init.in_qpmax;
            enc_info.gamma        = args->enc_init.in_gamma;

            ///////////////////////////////////
            ///   Initialize MFC Instance   ///
            ///////////////////////////////////
            Mfc_Clk_On();
            ret = MFCInst_Enc_Init(pMfcInst, codec_mode, &enc_info);
            Mfc_Clk_Off();

            // Output arguments for IOCTL_MFC_xxx_ENC_INIT
            args->enc_init.ret_code = ret;

            if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_H263_ENC_INIT.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            break;
        }

    case IOCTL_MFC_MPEG4_ENC_EXE:
    case IOCTL_MFC_H264_ENC_EXE:
    case IOCTL_MFC_H263_ENC_EXE:
        {
            int nStrmLen, nHdrLen;
            if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_H263_ENC_EXE.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            args = (MFC_ARGS *)pMarshalledInBuf;
            MFCInst_GetFramBuf(pMfcInst, &p_buf, &n_bufsize);
            CleanInvalidateCacheRange((PBYTE )p_buf, (PBYTE )(p_buf + n_bufsize) );
            
            // nStrmLen is size of output stream data
            Mfc_Clk_On();
            ret = MFCInst_Encode(pMfcInst, &nStrmLen, &nHdrLen);
            Mfc_Clk_Off();

            MFCInst_GetLineBuf(pMfcInst, &p_buf, &n_bufsize);
            InvalidateCacheRange((PBYTE )p_buf, (PBYTE )(p_buf + n_bufsize) );
        
            // Output arguments for IOCTL_MFC_xxx_ENC_EXE
            args->enc_exe.ret_code = ret;
            if (ret == MFCINST_RET_OK) {
                args->enc_exe.out_encoded_size = nStrmLen;
                args->enc_exe.out_header_size  = nHdrLen;
            }

            if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_H263_ENC_EXE.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            break;
        }

    case IOCTL_MFC_MPEG4_DEC_INIT:
    case IOCTL_MFC_H263_DEC_INIT:
    case IOCTL_MFC_H264_DEC_INIT:
    case IOCTL_MFC_VC1_DEC_INIT:
        {
            MFC_CODECMODE   codec_mode;

            if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_VC1_DEC_INIT.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            args = (MFC_ARGS *)pMarshalledInBuf;

            if (dwIoControlCode == IOCTL_MFC_MPEG4_DEC_INIT) {
                codec_mode = MP4_DEC;
            }
            else if (dwIoControlCode == IOCTL_MFC_H263_DEC_INIT) {
                codec_mode = MP4_DEC;
            }
            else if (dwIoControlCode == IOCTL_MFC_H264_DEC_INIT) {
                codec_mode = AVC_DEC;
            }
            else {
                codec_mode = VC1_DEC;
            }

            /////////////////////////////////
            //   Initialize MFC Instance   //
            /////////////////////////////////
            Mfc_Clk_On();
            ret = MFCInst_Dec_Init(pMfcInst, codec_mode, args->dec_init.in_strmSize);
            Mfc_Clk_Off();

            // Output arguments for IOCTL_MFC_xxx_DEC_INIT
            args->dec_init.ret_code = ret;
            if (ret == MFCINST_RET_OK) {
                args->dec_init.out_width  = pMfcInst->width;
                args->dec_init.out_height = pMfcInst->height;
            }

            if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
            {
                RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_VC1_DEC_INIT.\r\n")));
                MFC_Mutex_Release();
                return FALSE;
            }

            break;
        }

    case IOCTL_MFC_MPEG4_DEC_EXE:
    case IOCTL_MFC_H263_DEC_EXE:
    case IOCTL_MFC_H264_DEC_EXE:
    case IOCTL_MFC_VC1_DEC_EXE:

        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_VC1_DEC_EXE.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        args = (MFC_ARGS *)pMarshalledInBuf;

        MFCInst_GetLineBuf(pMfcInst, &p_buf, &n_bufsize);
        CleanInvalidateCacheRange((PBYTE )p_buf, (PBYTE )(p_buf + n_bufsize) );

        Mfc_Clk_On();
        ret = MFCInst_Decode(pMfcInst, args->dec_exe.in_strmSize);
        Mfc_Clk_Off();

        // Output arguments for IOCTL_MFC_xxx_DEC_EXE
        args->dec_exe.ret_code = ret;

        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_VC1_DEC_EXE.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;


    case IOCTL_MFC_GET_LINE_BUF_ADDR:

        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_LINE_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        args = (MFC_ARGS *)pMarshalledInBuf;

        ret = MFCInst_GetLineBuf(pMfcInst, &p_buf, &n_bufsize);
        if (ret != MFCINST_RET_OK) 
        {
            goto GetLineBuffCleanup;
        }
        
        if (handle->pStrmBuf == NULL)
        {
            // Map the Line buffer for this instance to the caller's address space
            //
            handle->hUsrProc = (HANDLE) GetDirectCallerProcessId();
            handle->pStrmBuf = (PBYTE) VirtualAllocEx(handle->hUsrProc, 
                                                        NULL,
                                                        n_bufsize, 
                                                        MEM_RESERVE, 
                                                        PAGE_NOACCESS);
            if (handle->pStrmBuf == NULL)
            {
                RETAILMSG(1, (L"DD::MFC VirtualAllocEx(pStrmBuf) returns FALSE.\n"));
                ret = MFCINST_ERR_MEMORY_ALLOCATION_FAIL;
                goto GetLineBuffCleanup;
            }
            result = VirtualCopyEx(handle->hUsrProc,
                                   handle->pStrmBuf,
                                   (HANDLE) GetCurrentProcessId(),
                                   p_buf,
                                   n_bufsize,
                                   PAGE_READWRITE );
            if (result == FALSE){
                RETAILMSG(1, (L"DD::MFC VirtualCopyEx(pStrmBuf) returns FALSE.\n"));
                VirtualFreeEx(handle->hUsrProc, handle->pStrmBuf, 0, MEM_RELEASE);
                handle->pStrmBuf = NULL;
                ret = MFCINST_ERR_MEMORY_ALLOCATION_FAIL;
                goto GetLineBuffCleanup;
            }
        }

        // Output arguments for IOCTL_MFC_GET_FRAM_BUF_ADDR
        args->get_buf_addr.out_buf_size   = n_bufsize;
        args->get_buf_addr.out_buf_addr   = (int) handle->pStrmBuf;


GetLineBuffCleanup:

        args->get_buf_addr.ret_code = ret;
        
        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_LINE_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;

    case IOCTL_MFC_GET_FRAM_BUF_ADDR:

        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_FRAM_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        args = (MFC_ARGS *)pMarshalledInBuf;

        // Decoder case
        ret = MFCInst_GetFramBuf(pMfcInst, &p_buf, &n_bufsize);
        if (ret != MFCINST_RET_OK) 
        {
            goto GetFrameBuffCleanup;
        }

        // Check Paramter
        if (pMfcInst->run_index * n_bufsize < 0 ||
            (pMfcInst->run_index * n_bufsize) > (int)(pMfcInst->nFramBufSize))
        {
            RETAILMSG(1, (L"[MFC ERROR] IOCTL_MFC_GET_FRAM_BUF_ADDR: Run Index out of range.\r\n"));
            ret = MFCINST_ERR_ETC;
            goto GetFrameBuffCleanup;
        }
        
        if (handle->pFramBuf == NULL)
        {
            // Map the Frame buffer for this instance to the caller's address space
            //
            handle->hUsrProc = (HANDLE) GetDirectCallerProcessId();
            handle->pFramBuf = (PBYTE) VirtualAllocEx(handle->hUsrProc, 
                                                      NULL,
                                                      pMfcInst->nFramBufSize, 
                                                      MEM_RESERVE, 
                                                      PAGE_NOACCESS);
            if (handle->pFramBuf == NULL)
            {
                RETAILMSG(1, (L"DD::MFC VirtualAllocEx(pFramBuf) returns FALSE.\n"));
                ret = MFCINST_ERR_MEMORY_ALLOCATION_FAIL;
                goto GetFrameBuffCleanup;
            }

            result= VirtualCopyEx(handle->hUsrProc,       // HANDLE hDstProc
                                  handle->pFramBuf,
                                  (HANDLE) GetCurrentProcessId(),     // HANDLE hSrcProc
                                  pMfcInst->pFramBuf,
                                  pMfcInst->nFramBufSize,
                                  PAGE_READWRITE);
            if (result == FALSE)
            {
                RETAILMSG(1, (L"DD::MFC VirtualCopyEx(pFramBuf) returns FALSE.\n"));
                VirtualFreeEx(handle->hUsrProc, handle->pFramBuf, 0, MEM_RELEASE);
                handle->pFramBuf = NULL;
                ret = MFCINST_ERR_MEMORY_ALLOCATION_FAIL;
                goto GetFrameBuffCleanup;
            }
        }

        if (pMfcInst->run_index >= 0)
        {
            args->get_buf_addr.out_buf_addr  = (int) (handle->pFramBuf + (pMfcInst->run_index * n_bufsize));

#if (MFC_ROTATE_ENABLE == 1)
            // If PostRotMode is enabled, then the output YUV buffer will be different.
            // In VC-1 mode, the rotated output will be the original one.
            if ( (pMfcInst->codec_mode != VC1_DEC) && (pMfcInst->PostRotMode & 0x0010) )
            {
                args->get_buf_addr.out_buf_addr = (int) (handle->pFramBuf + (pMfcInst->frambufCnt * n_bufsize));
            }
#endif
        }
        else
        {
            args->get_buf_addr.out_buf_addr  = 0;
        }
        
        args->get_buf_addr.out_buf_size   = n_bufsize;
        
GetFrameBuffCleanup:

        args->get_buf_addr.ret_code = ret;
        
        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_FRAM_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;

    case IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR:

        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        args = (MFC_ARGS *)pMarshalledInBuf;

        // Decoder case
        ret = MFCInst_GetFramBufPhysical(pMfcInst, &p_buf, &n_bufsize);

        // Output arguments for IOCTL_MFC_xxx_DEC_EXE
        args->get_buf_addr.ret_code = ret;

        if (ret == MFCINST_RET_OK) 
        { 
            // Output arguments for IOCTL_MFC_GET_FRAM_BUF_ADDR
            args->get_buf_addr.out_buf_addr   = (int) p_buf;
            args->get_buf_addr.out_buf_size   = n_bufsize;
        }

        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_PHY_FRAM_BUF_ADDR.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;

    case IOCTL_MFC_GET_MPEG4_ASP_PARAM:

        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_MPEG4_ASP_PARAM.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        args = (MFC_ARGS *)pMarshalledInBuf;

#if (defined(DIVX_ENABLE) && (DIVX_ENABLE == 1))
        ret = MFCINST_RET_OK;
        args->mpeg4_asp_param.ret_code              = MFCINST_RET_OK;
        args->mpeg4_asp_param.mp4asp_vop_time_res   = pMfcInst->RET_DEC_SEQ_INIT_BAK_MP4ASP_VOP_TIME_RES;
        args->mpeg4_asp_param.byte_consumed         = pMfcInst->RET_DEC_PIC_RUN_BAK_BYTE_CONSUMED;
        args->mpeg4_asp_param.mp4asp_fcode          = pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_FCODE;
        args->mpeg4_asp_param.mp4asp_time_base_last = pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_TIME_BASE_LAST;
        args->mpeg4_asp_param.mp4asp_nonb_time_last = pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_NONB_TIME_LAST;
        args->mpeg4_asp_param.mp4asp_trd            = pMfcInst->RET_DEC_PIC_RUN_BAK_MP4ASP_MP4ASP_TRD;

#if (_WIN32_WCE >= 600)
        if (handle->pFramBuf != NULL){
            args->mpeg4_asp_param.mv_addr      = ((unsigned int) handle->pFramBuf) + (pMfcInst->mv_mbyte_addr - pMfcInst->phyadrFramBuf);
            args->mpeg4_asp_param.mb_type_addr = args->mpeg4_asp_param.mv_addr + 25920;    
            args->mpeg4_asp_param.mv_size      = 25920; // '25920' is the maximum MV size (=45*36*16)
            args->mpeg4_asp_param.mb_type_size = 1620; // '1620' is the maximum MBTYE size (=45*36*1)
        }
#else
        args->mpeg4_asp_param.mv_addr      = ((unsigned int) pMfcInst->pFramBuf) + (pMfcInst->mv_mbyte_addr - pMfcInst->phyadrFramBuf);
        args->mpeg4_asp_param.mb_type_addr = args->mpeg4_asp_param.mv_addr + 25920;    
        args->mpeg4_asp_param.mv_size      = 25920;
        args->mpeg4_asp_param.mb_type_size = 1620;
#endif

        InvalidateCacheRange((PBYTE )args->mpeg4_asp_param.mv_addr, (PBYTE )(args->mpeg4_asp_param.mv_addr + args->mpeg4_asp_param.mv_size) );
        InvalidateCacheRange((PBYTE )args->mpeg4_asp_param.mb_type_addr , (PBYTE )(args->mpeg4_asp_param.mb_type_addr  + args->mpeg4_asp_param.mb_type_size) );
#endif

        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_MPEG4_ASP_PARAM.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }

        break;

    case IOCTL_MFC_GET_CONFIG:
        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_GET_CONFIG.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }
        args = (MFC_ARGS *)pMarshalledInBuf;

        ret = MFC_GetConfigParams(pMfcInst, args);

        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_GET_CONFIG.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }
        break;

    case IOCTL_MFC_SET_CONFIG:
        if(FAILED(CeOpenCallerBuffer(&pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR, TRUE)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeOpenCallerBuffer failed in IOCTL_MFC_SET_CONFIG.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }
        args = (MFC_ARGS *)pMarshalledInBuf;

        Mfc_Clk_On();
        ret = MFC_SetConfigParams(pMfcInst, args);
        Mfc_Clk_Off();

        if(FAILED(CeCloseCallerBuffer(pMarshalledInBuf, pInBuf, nInBufSize, ARG_IO_PTR)))
        {
            RETAILMSG(1, (TEXT("MFC_IOControl: CeCloseCallerBuffer failed in IOCTL_MFC_SET_CONFIG.\r\n")));
            MFC_Mutex_Release();
            return FALSE;
        }
        break;

    default:
        RETAILMSG(1, (L"[MFC IOControl] Requested ioctl command is not defined. (ioctl cmd=0x%X)\n", dwIoControlCode));
        MFC_Mutex_Release();
        return FALSE;
    }

    MFC_Mutex_Release();

    switch (ret)
    {
    case MFCINST_RET_OK:
        return TRUE;
    default:
        return FALSE;
    }
    return FALSE;
}

/*
** Function Name : MFC_PowerUp
**
** Function Description :
*/
BOOL MFC_PowerUp(DWORD InitHandle)
{
    return TRUE;
}

/*
** Function Name : MFC_PowerDown
**
** Function Description :
*/
BOOL MFC_PowerDown(DWORD InitHandle)
{
    return TRUE;
}


extern "C"
static DWORD MFC_IntrThread(void)
{
    unsigned int intr_reason;

    while (1)
    {
        // Wait for MFC Interrupt
        WaitForSingleObject(gMfcIntrEvent, INFINITE);

        // Only SEQ_INIT, SEQ_END, PIC_RUN and BUFFER EMPTY/FULL interrupts
        // will be processed.
        intr_reason = MfcIntrReason();
        RETAILMSG(0, (L"(( MFC Interrupt ) reason = 0x%X\n", intr_reason));

        if (intr_reason & MFC_INTR_ENABLE_RESET) {
            // On the MFC Interrupt,
            // MFC command completion event will be sent.
            // This event wakes up the task in WaitInterruptNotification() function.
            SendInterruptNotification(intr_reason);
        }

        // Clearing MFC interrupt bit
        MfcClearIntr();

        // Notify to Kernel that MFC Interrupt processing is completed.
        InterruptDone(g_MfcSysIntr);
    }

}

static BOOL InitializeIST()
{
    BOOL r;

    gMfcIntrEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (!gMfcIntrEvent) {
        ERRORMSG(1, (L"Unable to create interrupt event"));
        return(FALSE);
    }

    if (!CreateInterruptNotification()) {
        ERRORMSG(1, (L"Unable to create interrupt notification"));
        CloseHandle(gMfcIntrEvent);
        return FALSE;
    }

    r = KernelIoControl(IOCTL_HAL_REQUEST_SYSINTR,
                        &g_MfcIrq,     sizeof(UINT32),
                        &g_MfcSysIntr, sizeof(UINT32),
                        NULL);
    if (r != TRUE) {
        ERRORMSG(1, (L"Failed to request sysintr value for MFC interrupt.\r\n"));
        DeleteInterruptNotification();
        CloseHandle(gMfcIntrEvent);
        return FALSE;
    }


    r = InterruptInitialize(g_MfcSysIntr, gMfcIntrEvent, NULL, 0);
    if (r != TRUE) {
        ERRORMSG(1, (L"Unable to initialize output interrupt"));
        DeleteInterruptNotification();
        CloseHandle(gMfcIntrEvent);
        return FALSE;
    }

    gMfcIntrThread = CreateThread((LPSECURITY_ATTRIBUTES)NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE)MFC_IntrThread,
                                  0,
                                  0,
                                  NULL);
    if (!gMfcIntrThread) {
        ERRORMSG(1, (L"Unable to create interrupt thread"));
        InterruptDisable(g_MfcSysIntr);
        DeleteInterruptNotification();
        CloseHandle(gMfcIntrEvent);
        return FALSE;
    }

    // Bump up the priority since the interrupt must be serviced immediately.
    CeSetThreadPriority(gMfcIntrThread, MFC_THREAD_PRIORITY_DEFAULT);

    RETAILMSG(1, (L"MFC Interrupt has been initialized.\n"));

    return TRUE;
}
