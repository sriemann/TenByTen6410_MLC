//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
////////////////////////////////////////////////////////////////////////////////
//
//  Tux_SPI TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include "s3c6410_spi.h"

#define TOTAL_CONFIG          3
#define TESTSIZE_500         500
#define TESTSIZE_1000       1000
#define TESTSIZE_1504       1504  // SPI Driver Buffer_Mem_Size
#define TOTALSIZE_COUNT        3
#define TIME_SLAVEAP_RESET  2000
#define TIME_WAIT_WRITE        5  // In Milliseconds
#define TIMEOUT_CONSTANT   10000
#define TEST_BUFFER_SIZE   sizeof(pTestBuffer)

INT TotalTestSize[TOTALSIZE_COUNT] = { TESTSIZE_500, TESTSIZE_1000, TESTSIZE_1504 };

DWORD WINAPI ThreadProc_InvalidOpen                  ( LPVOID lpParameter );
DWORD WINAPI ThreadProc_InvalidRead                  ( LPVOID lpParameter );
DWORD WINAPI ThreadProc_InvalidWrite                 ( LPVOID lpParameter );
DWORD WINAPI ThreadProc_Invalid_SPI_IOCTL_SET_CONFIG ( LPVOID lpParameter );

SET_CONFIG g_spiConfigData[TOTAL_CONFIG]={
    //dwMode         bUseFullDuflex  dwRxBurstDataLen  bUseRxDMA   bUseRxIntr  dwTxBurstDataLen    bUseTxDMA   bUseTxIntr  dwPrescaler  dwTimeOutVal
    // DMA
    {SPI_MASTER_MODE,    TRUE,            0,                TRUE,     FALSE,        0,                TRUE,      FALSE,          0,        0    },
    //Intr
    {SPI_MASTER_MODE,    TRUE,            0,                FALSE,     TRUE,        0,                FALSE,      TRUE,          0,        0    },
    //Intr
    {SPI_MASTER_MODE,    TRUE,            0,                FALSE,     FALSE,       0,                FALSE,     FALSE,          0,        0    }
};

static BYTE pTestBuffer[] = {
    0x01, 0x02, 0x03 ,0x04 ,0x05 ,0x06 ,0x07 ,0x08 ,0x09 ,0x0a ,0x0b ,0x0c ,0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13 ,0x14 ,0x15 ,0x16 ,0x17 ,0x18 ,0x19 ,0x1A ,0x1B ,0x1C ,0x1D, 0x1E, 0x1F, 0x20,
    0x21, 0x22, 0x23 ,0x24 ,0x25 ,0x26 ,0x27 ,0x28 ,0x29 ,0x2a ,0x2b ,0x2c ,0x2d, 0x2e, 0x2f, 0x30,
    0x31, 0x32, 0x33 ,0x34 ,0x35 ,0x36 ,0x37 ,0x38 ,0x39 ,0x3A ,0x3B ,0x3C ,0x3D, 0x3E, 0x3F, 0x40
    //64
};

typedef struct
{
    HANDLE hWaitInvalidTest;
    BOOL   bTestResult;
}CMD_DATA, *PCMD_DATA;

typedef struct 
{
    BOOL bNullBuffer;
    INT  iSize;
}INVALID_TESTDATA;



////////////////////////////////////////////////////////////////////////////////
// SPITUX_OpenCloseVerify
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_OpenCloseVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    HANDLE hSPI    = NULL;
    INT    retFlag = TPR_PASS; 

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        retFlag=TPR_FAIL;
    }
    else
    {
        if ( !SPIDevice_Close(hSPI) )
        {
            retFlag = TPR_FAIL;
        }
    }

    return retFlag;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_IoctlVerify_CmdConfig
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_IoctlVerify_CmdConfig(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    DWORD  dwReturn  = TPR_PASS;    
    HANDLE hSPI      = NULL ;
    BOOL   bRetClose = FALSE;
    INT    iConfig   = 0;
    SET_CONFIG spiConfig;
    
    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI)
    {
        dwReturn = TPR_FAIL;
        goto Exit;
    }

    //Change different settings for SPI_IOCTL_SET_CONFIG
    for( iConfig = 0; iConfig < TOTAL_CONFIG; iConfig++)
    {
        spiConfig = g_spiConfigData[iConfig];

        if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, NULL, NULL, NULL) ) 
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
            dwReturn = TPR_FAIL;
            break;            
        }
    }

Exit:

    if( hSPI )
    {
        bRetClose =  SPIDevice_Close(hSPI);

        if( !bRetClose )
        {
            dwReturn = TPR_FAIL;
        }
    }

    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_IoctlVerify_CmdStart
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_IoctlVerify_CmdStart(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    DWORD  dwReturn  = TPR_PASS;    
    HANDLE hSPI      = NULL;
    BOOL   bRetClose = FALSE;

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        dwReturn=TPR_FAIL;
        goto Exit;
    }

    if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, NULL, NULL, NULL, NULL, NULL) ) 
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START"));
    }
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START"));
        dwReturn = TPR_FAIL;
    }

Exit:
    if( hSPI )
    {
        bRetClose = SPIDevice_Close(hSPI);
        if( !bRetClose )
        {
            dwReturn = TPR_FAIL;
        }
    }
    return dwReturn;
}



////////////////////////////////////////////////////////////////////////////////
// SPITUX_InvalidRead
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_InvalidRead(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    CMD_DATA CmdData;
    DWORD    dwWaitRet = 0;
    HANDLE   hThread   = NULL;
    DWORD    dwReturn  = TPR_PASS;    

    CmdData.hWaitInvalidTest = CreateEvent(NULL, FALSE, FALSE, _T("SPI_InavlidRead_Event"));
    CmdData.bTestResult=TRUE;

    hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_InvalidRead, (LPVOID)&CmdData, 0,0);
    dwWaitRet=WaitForSingleObject(CmdData.hWaitInvalidTest, TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("[TUXSPI]:SPI read with invalid parameters timeout."));
        CmdData.bTestResult = FALSE;
    } 

    if( hThread )
    {
        CloseHandle(hThread);
        hThread = NULL;
    }

    if( CmdData.hWaitInvalidTest )
    {
        CmdData.hWaitInvalidTest = NULL;
        CloseHandle(CmdData.hWaitInvalidTest);
    }

    if ( CmdData.bTestResult == TRUE )
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:SPI read with invalid parameters test succeeded"));
    } 
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI read with invalid parameters test failed"));
        dwReturn = TPR_FAIL;
    }

    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_InvalidWrite
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_InvalidWrite(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    CMD_DATA CmdData;
    DWORD    dwWaitRet = 0;
    HANDLE   hThread   = NULL;
    DWORD    dwReturn  = TPR_PASS;

    CmdData.hWaitInvalidTest = CreateEvent(NULL, FALSE, FALSE, _T("SPI_InavlidWrite_Event"));
    CmdData.bTestResult = TRUE;

    hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_InvalidWrite, (LPVOID)&CmdData, 0,0);
    dwWaitRet=WaitForSingleObject(CmdData.hWaitInvalidTest, TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("[TUXSPI]:SPI write with invalid parameters timeout."));
        CmdData.bTestResult = FALSE;
    } 

    if( hThread )
    {
        CloseHandle(hThread);
        hThread = NULL;
    }

    if( CmdData.hWaitInvalidTest )
    {
        CmdData.hWaitInvalidTest = NULL;
        CloseHandle(CmdData.hWaitInvalidTest);
    }

    if ( CmdData.bTestResult == TRUE )
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:SPI write with invalid parameters test succeeded"));
    } 
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI write with invalid parameters test failed"));
        dwReturn=TPR_FAIL;
    }

    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_InvalidIOCTL_Config
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_InvalidIOCTL_Config(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    CMD_DATA CmdData;
    DWORD    dwWaitRet = 0;
    HANDLE   hThread   = NULL;
    DWORD    dwReturn  = TPR_PASS;

    CmdData.hWaitInvalidTest = CreateEvent(NULL, FALSE, FALSE, _T("SPI_IOCTL_SET_CONFIG_Event"));
    CmdData.bTestResult = TRUE;

    hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Invalid_SPI_IOCTL_SET_CONFIG, (LPVOID)&CmdData, 0,0);
    dwWaitRet=WaitForSingleObject(CmdData.hWaitInvalidTest, TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("[TUXSPI]:SPI_IOCTL_SET_CONFIG with invalid parameters timeout."));
        CmdData.bTestResult = FALSE;
    } 

    if( hThread )
    {
        CloseHandle(hThread);
        hThread = NULL;
    }

    if( CmdData.hWaitInvalidTest )
    {
        CmdData.hWaitInvalidTest = NULL;
        CloseHandle(CmdData.hWaitInvalidTest);
    }


    if ( CmdData.bTestResult == TRUE )
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:SPI_IOCTL_SET_CONFIG with invalid parameters test succeeded"));
    } 
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI_IOCTL_SET_CONFIG with invalid parameters test failed"));
        dwReturn = TPR_FAIL;
    }

    return dwReturn;
}



////////////////////////////////////////////////////////////////////////////////
// SPITUX_TransferData
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_TransferData(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    DWORD  dwReturn   = TPR_PASS;
    DWORD  count      = 0;
    INT    iConfig    = 0;
    INT    iStartTick = 0;
    INT    iEndTick   = 0;
    BOOL   bRetClose  = FALSE;
    HANDLE hSPI       = NULL;
    SET_CONFIG spiConfig;
    unsigned char* plocalTestBuffer = NULL;

    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI)
    {
        dwReturn = TPR_FAIL;
        goto Exit;
    }

    //Change different settings for SPI_IOCTL_SET_CONFIG
    for( iConfig = 0; iConfig < TOTAL_CONFIG; iConfig++)
    {
        Sleep(TIME_SLAVEAP_RESET);

        spiConfig = g_spiConfigData[iConfig];

        if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, NULL, NULL, NULL) ) 
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);

            if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, NULL, NULL, NULL, NULL, NULL) ) 
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START") );
            }
            else
            {
                g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START") );
                dwReturn = TPR_FAIL;
                goto Exit;
            }
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
            dwReturn = TPR_FAIL;
            goto Exit;
        }
        
        if ( 0 == WriteFile(hSPI, pTestBuffer, TEST_BUFFER_SIZE, &count, NULL) )
        {
            g_pKato->Log(LOG_FAIL, TEXT("Writing to SPI device failed\n"));
            dwReturn = TPR_FAIL;
            goto Exit;
        }

        if( count == TEST_BUFFER_SIZE )
        {
            BOOL bSuccess   = TRUE;
            INT  iCheckByte = 0;

            plocalTestBuffer = (unsigned char *) malloc(TEST_BUFFER_SIZE);

            if ( NULL == plocalTestBuffer )
            {
                bSuccess = FALSE;
                g_pKato->Log(LOG_FAIL, TEXT("Allocation of plocalTestBuffer failed\n"));
                break;
            }

            Sleep(TIME_WAIT_WRITE);

            if ( 0 == ReadFile(hSPI, plocalTestBuffer, TEST_BUFFER_SIZE, &count, NULL) )
            {
                bSuccess = FALSE;
                g_pKato->Log(LOG_FAIL, TEXT("Reading from SPI device failed\n"));
                break;
            }

            for( iCheckByte = 0; iCheckByte < TEST_BUFFER_SIZE; iCheckByte++ )
            {
                if( pTestBuffer[iCheckByte] != plocalTestBuffer[iCheckByte] )
                {
                    bSuccess = FALSE;
                    g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail Compare index=%d\n"),iCheckByte );
                    break;
                }
            }

            if( bSuccess )
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to Read and Verify.") );
            }
            else
            {
                g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to Read and Verify from slave device, Check Byte Fail=%d"),iCheckByte );
                dwReturn = TPR_FAIL;
            }

            if( plocalTestBuffer )
            {
                free(plocalTestBuffer);
                plocalTestBuffer = NULL;
            }
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to Write Data to slave device") );
            dwReturn = TPR_FAIL;
        }
    }

Exit:

    if( plocalTestBuffer )
    {
        free(plocalTestBuffer);
        plocalTestBuffer = NULL;
    }

    if( hSPI )
    {
        bRetClose =  SPIDevice_Close(hSPI);
        if( !bRetClose )
        {
            dwReturn = TPR_FAIL;
        }
    }

    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_ReadPerformance
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_ReadPerformance(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    DWORD  dwReturn   = TPR_PASS;
    INT    iConfig    = 0;
    INT    iStartTick = 0;
    INT    iEndTick   = 0;
    INT    iTestSize  = 0;
    BOOL   bRetClose  = FALSE;
    HANDLE hSPI       = NULL;
    SET_CONFIG spiConfig;
    unsigned char* plocalTestBuffer = NULL;

    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        dwReturn = TPR_FAIL;
        goto Exit;
    }

    //Change different settings for SPI_IOCTL_SET_CONFIG
    for( iConfig = 0; iConfig < TOTAL_CONFIG; iConfig++ )
    {
        spiConfig = g_spiConfigData[iConfig];
        
        if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, NULL, NULL, NULL) ) 
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);

            if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, NULL, NULL, NULL, NULL, NULL) ) 
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START") );
            }
            else
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START"));
                dwReturn = TPR_FAIL;
                goto Exit;
            }
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);

            dwReturn = TPR_FAIL;
            goto Exit;
        }

        for( iTestSize = 0; iTestSize < TOTALSIZE_COUNT; iTestSize++ )
        {

            UINT  nWriteSize = TotalTestSize[iTestSize];
            DWORD count      = 0;
            INT   iCostTime  = 0;

            plocalTestBuffer = (unsigned char *) malloc(nWriteSize);
            memset(plocalTestBuffer,0x0,nWriteSize);

            iStartTick = GetTickCount();

            if ( 0 == ReadFile(hSPI, plocalTestBuffer, nWriteSize, &count, NULL) )
            {
                g_pKato->Log(LOG_FAIL, TEXT("Reading from SPI device failed\n"));
                dwReturn = TPR_FAIL;
                break;
            }
            
            iEndTick   = GetTickCount();
            iCostTime  = iEndTick-iStartTick;

            if( count == nWriteSize ) 
            {
                FLOAT fBpms = 0.0;
                fBpms = ( (FLOAT)nWriteSize / (FLOAT)iCostTime );
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to Write Size=%6d Bytes. CostTime: %4d mSec. Rate= %3.2f Bytes/mSec \n"), nWriteSize,iCostTime, fBpms); 
            }
            else
            {
                g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to Write Size=%d / %d Bytes. CostTime: %4d mSec \n"), count,nWriteSize,iCostTime);
                dwReturn = TPR_FAIL;
                break;
            }

            if( plocalTestBuffer )
            {
                free(plocalTestBuffer);
                plocalTestBuffer = NULL;
            }
        }
    }

Exit:

    if( plocalTestBuffer )
    {
        free(plocalTestBuffer);
        plocalTestBuffer = NULL;
    }

    if( hSPI )
    {
        bRetClose =  SPIDevice_Close(hSPI);
        if( !bRetClose)
        {
            dwReturn = TPR_FAIL;
        }
    }
    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPITUX_WritePerformance
//  Executes one test.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.

TESTPROCAPI SPITUX_WritePerformance(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    DWORD  dwReturn   = TPR_PASS;
    INT    iConfig    = 0;
    INT    iStartTick = 0;
    INT    iEndTick   = 0;
    INT    iTestSize  = 0;
    BOOL   bRetClose  = FALSE;
    HANDLE hSPI       = NULL;
    SET_CONFIG spiConfig;
    unsigned char* plocalTestBuffer = NULL;

    // The shell doesn't necessarily want us to execute the test. Make sure
    // first.
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        dwReturn = TPR_FAIL;
        goto Exit;
    }

    //Change different settings for SPI_IOCTL_SET_CONFIG
    for( iConfig = 0; iConfig < TOTAL_CONFIG; iConfig++)
    {
        spiConfig = g_spiConfigData[iConfig];

        if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, NULL, NULL, NULL) ) 
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
            
            if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, NULL, NULL, NULL, NULL, NULL) ) 
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START") );
            }
            else
            {
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START"));
                dwReturn = TPR_FAIL;
                goto Exit;
            }
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                         spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
            dwReturn = TPR_FAIL;
            goto Exit;
        }
        
        for( iTestSize = 0; iTestSize < TOTALSIZE_COUNT; iTestSize++)
        {
            UINT  nWriteSize = TotalTestSize[iTestSize];
            DWORD count      = 0;
            INT   iCostTime  = 0;

            plocalTestBuffer = (unsigned char *) malloc(nWriteSize);
            memset(plocalTestBuffer, 0x0, nWriteSize);
        
            iStartTick = GetTickCount();
            WriteFile(hSPI, plocalTestBuffer, nWriteSize, &count, NULL);
            iEndTick   = GetTickCount();
            iCostTime  = iEndTick-iStartTick;

            if( count == nWriteSize ) 
            {
                FLOAT fBpms = 0.0;
                fBpms = ( (FLOAT)nWriteSize / (FLOAT)iCostTime );
                g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to Write Size=%6d Bytes. CostTime: %4d mSec. Rate= %3.2f Bytes/mSec \n"), nWriteSize,iCostTime, fBpms); 
            }
            else
            {
                g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to Write Size=%6d / %6d Bytes. CostTime: %4d \t\n"), count,nWriteSize,iCostTime);
                dwReturn = TPR_FAIL;
                break;
            }
            
            if( plocalTestBuffer )
            {
                free(plocalTestBuffer);
                plocalTestBuffer = NULL;
            }
        }
    }

Exit:

    if( plocalTestBuffer )
    {
        free(plocalTestBuffer);
        plocalTestBuffer = NULL;
    }

    if( hSPI )
    {
        bRetClose =  SPIDevice_Close(hSPI);
        if( !bRetClose )
        {
            dwReturn = TPR_FAIL;
        }
    }
    return dwReturn;
}


////////////////////////////////////////////////////////////////////////////////
// SPIDevice_Open
//  Open SPI device
//
// Parameters:
// 
// Return value:
//  Device handle if open device correctly, NULL if open device fails.
//  special conditions.

HANDLE SPIDevice_Open()
{
    HANDLE hSPI = NULL;

    hSPI = CreateFile(_T("SPI1:"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0);

    if(INVALID_HANDLE_VALUE != hSPI)
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to open SPI1"));
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to open SPI1"));
    }

    return hSPI;
}


////////////////////////////////////////////////////////////////////////////////
// SPIDevice_Close
//  Close SPI device
//
// Parameters:
// 
// Return value:
//  TRUE if close device correctly, FALSE if close device fails.
//  special conditions.

BOOL SPIDevice_Close(HANDLE hSPIClose)
{
    BOOL bRet = FALSE;

    if( hSPIClose )
    {
        bRet = CloseHandle(hSPIClose);

        if( bRet )
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to close SPI1"));
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to close SPI1"));
        }
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to close SPI1 - SPI Handle NULL"));
    }

    return bRet;
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_InvalidRead
//  Thread for SPITUX_InvalidRead
//
// Parameters:
//  lpParameter            command data.
//  
DWORD WINAPI ThreadProc_InvalidRead(LPVOID lpParameter)
{
    HANDLE     hSPI     = INVALID_HANDLE_VALUE;
    BOOL       bResult  = FALSE;
    PCMD_DATA  pCmdData = (PCMD_DATA) lpParameter;
    DWORD      count    = 0;
    SET_CONFIG spiConfig;
    unsigned char* plocalTestBuffer = NULL;

    plocalTestBuffer = (unsigned char *) malloc(TEST_BUFFER_SIZE);

    spiConfig.dwMode      = SPI_MASTER_MODE;
    spiConfig.bUseRxDMA   = FALSE;
    spiConfig.bUseRxIntr  = TRUE;
    spiConfig.bUseTxDMA   = FALSE;
    spiConfig.bUseTxIntr  = TRUE;
    spiConfig.dwPrescaler = 2;

    pCmdData->bTestResult = TRUE;

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        goto Exit;
    }

    if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, 0, NULL, NULL) ) 
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                     spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler   );

        if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, 0, NULL, 0, NULL, NULL) ) 
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START"));
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START"));
            goto Exit;
        }
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                     spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
        goto Exit;
    }

    //READ with NULL pointer test  
    if ( 0 != ReadFile(hSPI, NULL, TEST_BUFFER_SIZE, &count, NULL) )
    {
        if( 0 != count )
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI read with NULL pointer test failed"));
            pCmdData->bTestResult = FALSE;
        }
    }

Exit:
    if( plocalTestBuffer )
    {
        free(plocalTestBuffer);
        plocalTestBuffer=NULL;
    }

    if( hSPI )
    {
        if( SPIDevice_Close(hSPI) == FALSE )
        {
            pCmdData->bTestResult = FALSE;;
        }
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return ERROR_SUCCESS;
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_InvalidWrite
//  Thread for SPITUX_InvalidWrite
//
// Parameters:
//  lpParameter            command data.
//  
DWORD WINAPI ThreadProc_InvalidWrite(LPVOID lpParameter)
{
    HANDLE     hSPI     = INVALID_HANDLE_VALUE;
    BOOL       bResult  = FALSE;
    PCMD_DATA  pCmdData = (PCMD_DATA) lpParameter;
    DWORD      count    = 0;
    SET_CONFIG spiConfig;
    unsigned char* plocalTestBuffer = NULL;

    plocalTestBuffer = (unsigned char *) malloc(TEST_BUFFER_SIZE);

    spiConfig.dwMode      = SPI_MASTER_MODE;
    spiConfig.bUseRxDMA   = FALSE;
    spiConfig.bUseRxIntr  = TRUE;
    spiConfig.bUseTxDMA   = FALSE;
    spiConfig.bUseTxIntr  = TRUE;
    spiConfig.dwPrescaler = 2;

    pCmdData->bTestResult = TRUE;

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        goto Exit;
    }

    if( DeviceIoControl(hSPI, SPI_IOCTL_SET_CONFIG, &spiConfig, sizeof(spiConfig), NULL, 0, NULL, NULL) )
    {
        g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                     spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);

        if( DeviceIoControl(hSPI, SPI_IOCTL_START, NULL, 0, NULL, 0, NULL, NULL) )
        {
            g_pKato->Log(LOG_PASS, TEXT("[TUXSPI]:Success to set SPI_IOCTL_START"));
        }
        else
        {
            g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_START"));
            goto Exit;
        }
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:Fail to set SPI_IOCTL_SET_CONFIG Master=%d, DMA=%d, Intrrupt=%d,preScaler=%d"),
                     spiConfig.dwMode,spiConfig.bUseRxDMA,spiConfig.bUseRxIntr,spiConfig.dwPrescaler);
        goto Exit;
    }

    //WRITE with NULL pointer test  
    WriteFile(hSPI, NULL, TEST_BUFFER_SIZE, &count, NULL);
    
    if( 0 != count )
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI write with NULL pointer test failed"));
        pCmdData->bTestResult = FALSE;
    }

Exit:
    if( plocalTestBuffer )
    {
        free(plocalTestBuffer);
        plocalTestBuffer = NULL;
    }

    if( hSPI )
    {
        if( SPIDevice_Close(hSPI) == FALSE )
        {
            pCmdData->bTestResult = FALSE;;
        }
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_SPI_IOCTL_SET_CONFIG
//  Thread for SPITUX_InvalidIOCTL_Config
//
// Parameters:
//  lpParameter            command data.
//  
DWORD WINAPI ThreadProc_Invalid_SPI_IOCTL_SET_CONFIG(LPVOID lpParameter)
{
    HANDLE     hSPI     = INVALID_HANDLE_VALUE;
    BOOL       bResult  = FALSE;
    PCMD_DATA  pCmdData = (PCMD_DATA) lpParameter;
    SET_CONFIG spiConfig;

    spiConfig.dwMode      = SPI_MASTER_MODE;
    spiConfig.bUseRxDMA   = FALSE;
    spiConfig.bUseRxIntr  = TRUE;
    spiConfig.bUseTxDMA   = FALSE;
    spiConfig.bUseTxIntr  = TRUE;
    spiConfig.dwPrescaler = 2;
    
    pCmdData->bTestResult = TRUE;

    hSPI = SPIDevice_Open();

    if( INVALID_HANDLE_VALUE == hSPI )
    {
        goto Exit;
    }

    // SPI_IOCTL_SET_CONFIG with NULL pointer test     
    if ( FALSE != DeviceIoControl(hSPI,
                                  SPI_IOCTL_SET_CONFIG,
                                  NULL, 
                                  sizeof(SET_CONFIG), 
                                  NULL, 
                                  0, 
                                  NULL, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXSPI]:SPI_IOCTL_SET_CONFIG with NULL pointer test failed"));
        pCmdData->bTestResult = FALSE;
    }    

Exit:
    if( hSPI )
    {
        if( FALSE == SPIDevice_Close(hSPI) )
        {
            pCmdData->bTestResult = FALSE;
        }
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return ERROR_SUCCESS;
}
