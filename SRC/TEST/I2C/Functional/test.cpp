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
//  I2CTUX TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include "winioctl.h"
#include "pm.h"
#include "iic.h"
#include <commctrl.h>
#include <initguid.h>

typedef struct
{
   HANDLE hWaitInvalidTest;
   bool   bTestResult;
}CMD_DATA, *PCMD_DATA;

#define READTIMES            10
#define WRITETIMES           10
#define DX_D0_D4             0x11
#define BUFFER_SIZE          256

#define TIMEOUT_CONSTANT     10000
#define CAMERA_WRITE         (0x5a + 0)
#define CAMERA_READ          (0x5a + 1)
#define CAMERA_REG_ADDRESS   0xFC   // camera moudule register address
#define IMAGE_FORMAT_ADDRESS 0x02   // register for camera image format
#define IMAGE_FORMAT_QVGA    0x03   // QVGA image format
#define IMAGE_FORMAT_QQVGA   0x04   // QQVGA image format
#define CAMERA_COMMAND_PAGE  0x00   // page number for command register
#define DATA_COUNT_2         2
#define DATA_COUNT_1         1

#define IIC_CLOCK_2000       2000  // IIC clock
#define IIC_CLOCK_3000       3000  // IIC clock

#define IIC_FILTER_DISABLE   0     // disable IIC filter
#define IIC_FILTER_ENABLE    1     // enable IIC filter
#define IIC_MODE_INVALID     5     // invalid mode
#define IIC_DELAY_INVALID    5     // invalid delay value
#define IIC_CLOCK_INVALID    0     // invalid clock value

#define CALCULATE_READ_PERFORMANCE(READTIMES,iCostTime)   ((FLOAT)READTIMES  * 1000 / (FLOAT)iCostTime)
#define CALCULATE_WRITE_PERFORMANCE(WRITETIMES,iCostTime) ((FLOAT)WRITETIMES * 3 * 1000 / (FLOAT)iCostTime)

#define IIC_READ_BUFFER_SIZE 13 

#define FILE_DEVICE_CAMERA     FILE_DEVICE_CONTROLLER

#define IOCTL_CAMERA_ON \
    CTL_CODE(FILE_DEVICE_CAMERA, 0, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_CAMERA_OFF \
    CTL_CODE(FILE_DEVICE_CAMERA, 1, METHOD_BUFFERED, FILE_ANY_ACCESS)


////////////////////////////////////////////////////////////////////////////////
// I2CTUX_OpenCloseVerify
//  Verify I2C Open and Close stream interface work correctly.
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_OpenCloseVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver   = INVALID_HANDLE_VALUE;
    DWORD  tprResult = TPR_PASS;
    BOOL   bResult   = FALSE;

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
    }
    else
    {     
        bResult = CloseHandle(hDriver);

        if ( FALSE == bResult )
        {
            g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
            tprResult = TPR_FAIL;
        }
    }

    if ( TPR_PASS == tprResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C Open and Close test succeeded"));
    }

    return tprResult;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_IoctlPowerCapabilitiesVerify
// Verify IOCTL_POWER_CAPABILITIES function work correctly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_IoctlPowerCapabilitiesVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver                = INVALID_HANDLE_VALUE;
    DWORD  tprResult              = TPR_PASS;
    TCHAR  bufferIn[BUFFER_SIZE]  = {0};
    TCHAR  bufferOut[BUFFER_SIZE] = {0};
    DWORD  dwBytesWritten         = 0;
    DWORD  dwSize                 = 0;
    PPOWER_CAPABILITIES ppc;

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        goto Exit;
    }

    dwSize = sizeof(POWER_CAPABILITIES);

    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_POWER_CAPABILITIES,
                                  bufferIn, 
                                  dwSize, 
                                  bufferOut, 
                                  dwSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_POWER_CAPABILITIES failed, IOCTL return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }    
    
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_POWER_CAPABILITIES failed, CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }

    ppc = (PPOWER_CAPABILITIES)bufferOut;

    if( NULL == ppc->DeviceDx || DX_D0_D4 != ppc->DeviceDx )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_POWER_CAPABILITIES failed, ppc->DeviceDx is unset."));
        tprResult = TPR_FAIL;
        goto Exit;
    }
    else
    {
        g_pKato->Log(LOG_COMMENT, TEXT("ppc->DeviceDx = %x."),ppc->DeviceDx);
    }
    
    if( 0 == ppc->Flags )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_POWER_CAPABILITIES failed, ppc->Flags is unset."));
        tprResult = TPR_FAIL;
        goto Exit;
    } 
    else
    {
        g_pKato->Log(LOG_COMMENT, TEXT("ppc->Flags = %x."),ppc->Flags);
    }

    if ( TPR_PASS == tprResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("IOCTL_POWER_CAPABILITIES function test succeeded"));
    }     

Exit:
    return tprResult;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_IoctlClockVerify
// Verify IOCTL_IIC_SET_CLOCK and IOCTL_IIC_GET_CLOCK functions work correctly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_IoctlClockVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver                = INVALID_HANDLE_VALUE;
    DWORD  tprResult              = TPR_PASS;
    TCHAR  bufferIn[BUFFER_SIZE]  = {0};
    TCHAR  bufferOut[BUFFER_SIZE] = {0};
    DWORD  dwBytesWritten         = 0;
    DWORD  dwSize                 = 0;
    UINT32 iGetClock              = 0;
    UINT32 iSetClock              = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        goto Exit;
    }
    
    dwSize = sizeof(bufferOut);

    // Get the current I2C clock setting first.
    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_CLOCK,
                                  bufferIn, 
                                  dwSize, 
                                  bufferOut, 
                                  dwSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_CLOCK failed, IOCTL return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }    
    
    if( 0 == dwBytesWritten )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_CLOCK failed, IOCTL bytes written = 0. "));
        tprResult = TPR_FAIL;
    }
    else
    {
        g_pKato->Log(LOG_COMMENT, TEXT("Original IIC_Clock = %d."), bufferOut[0]);
        
        iGetClock = (UINT32)bufferOut[0];
        
        if( IIC_CLOCK_3000 == iGetClock )
        {
            iSetClock = IIC_CLOCK_2000;
        }
        else
        {
            iSetClock = IIC_CLOCK_3000;
        }
        
        bufferIn[0] = iSetClock;
        
        g_pKato->Log(LOG_COMMENT, TEXT("Set IIC_Clock to %d."), iSetClock);

        // Set I2C clock
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_SET_CLOCK,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_SET_CLOCK failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }  

        // Get I2C clock        
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_GET_CLOCK,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_CLOCK failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }    
    
        if( 0 == dwBytesWritten )
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_CLOCK failed, IOCTL bytes written = 0. "));
            tprResult = TPR_FAIL;
        }
        else
        {
            g_pKato->Log(LOG_COMMENT, TEXT("Updated IIC_Clock = %d."), bufferOut[0]);
            
            iGetClock = (UINT32)bufferOut[0];

            // Compare clock setting
            if( iGetClock == iSetClock )
            {
                 g_pKato->Log(LOG_COMMENT, TEXT("IoctlClockVerify succeeded. "));
            }
            else
            {
                 g_pKato->Log(LOG_FAIL, TEXT("IoctlClockVerify failed, data does not match. "));
                 tprResult = TPR_FAIL;     
            }
        }
    }

Exit:    
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;
    }

    if ( tprResult == TPR_PASS )
    {
        g_pKato->Log(LOG_PASS, TEXT("IOCTL_IIC_SET_CLOCK and IOCTL_IIC_GET_CLOCK functions test succeeded"));
    }   

    return tprResult;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_IoctlFilterVerify
// Verify IOCTL_IIC_SET_FILTER and IOCTL_IIC_GET_FILTER functions work correctly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_IoctlFilterVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver                = INVALID_HANDLE_VALUE;
    DWORD  tprResult              = TPR_PASS;
    TCHAR  bufferIn[BUFFER_SIZE]  = {0};
    TCHAR  bufferOut[BUFFER_SIZE] = {0};
    DWORD  dwBytesWritten         = 0;
    DWORD  dwSize                 = 0;
    BYTE   byGetFilter            = 0;
    BYTE   bySetFilter            = 0;

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        goto Exit;
    }
    
    dwSize = sizeof(bufferOut);

    // Get the current I2C filter setting first.
    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_FILTER,
                                  bufferIn, 
                                  dwSize, 
                                  bufferOut, 
                                  dwSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_FILTER failed, IOCTL return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }    

    if( 0 == dwBytesWritten )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_FILTER failed, IOCTL bytes written = 0. "));
        tprResult = TPR_FAIL;
    }
    else
    {
        g_pKato->Log(LOG_COMMENT, TEXT("Original IIC_FILTER = %x."), bufferOut[0]);

        byGetFilter = (BYTE)bufferOut[0];
        
        if( IIC_FILTER_DISABLE == byGetFilter )
        {
            bySetFilter = IIC_FILTER_ENABLE;
        }
        else
        {
            bySetFilter = IIC_FILTER_DISABLE;
        }
        
        bufferIn[0] = bySetFilter;
        
        g_pKato->Log(LOG_COMMENT, TEXT("Set IIC_Filter to %x."), bySetFilter);

        // Set I2C Filter
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_SET_FILTER,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_SET_FILTER failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }  

        // Get I2C Filter        
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_GET_FILTER,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_FILTER failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }    

        if( 0 == dwBytesWritten )
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_FILTER failed, IOCTL bytes written = 0. "));
            tprResult = TPR_FAIL;
        }
        else
        {
            g_pKato->Log(LOG_COMMENT, TEXT("Updated IIC_FILTER = %x."), bufferIn[0]);

            byGetFilter = (BYTE)bufferOut[0];

            // Compare filter setting
            if( byGetFilter == bySetFilter )
            {
                 g_pKato->Log(LOG_COMMENT, TEXT("IoctlFilterVerify succeeded. "));
            }
            else
            {
                 g_pKato->Log(LOG_FAIL, TEXT("IoctlFilterVerify failed, data does not match. "));
                 tprResult = TPR_FAIL;
            }
        }
    }

Exit:
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;
    }

    if ( TPR_PASS == tprResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("IOCTL_IIC_SET_FILTER and IOCTL_IIC_GET_FILTER functions test succeeded"));
    }   

    return tprResult;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_IoctlDelayVerify
// Verify IOCTL_IIC_SET_DELAY and IOCTL_IIC_GET_DELAY functions work correctly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_IoctlDelayVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver                = INVALID_HANDLE_VALUE;
    DWORD  tprResult              = TPR_PASS;
    TCHAR  bufferIn[BUFFER_SIZE]  = {0};
    TCHAR  bufferOut[BUFFER_SIZE] = {0};
    DWORD  dwBytesWritten         = 0;
    DWORD  dwSize                 = 0;
    int    iGetDelay              = 0;
    int    iSetDelay              = 0;

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }    

    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        goto Exit;
    }
    
    dwSize = sizeof(bufferOut);

    // Get the current I2C delay setting first.
    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_DELAY,
                                  bufferIn, 
                                  dwSize, 
                                  bufferOut, 
                                  dwSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_DELAY failed, IOCTL return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }    

    if( 0 == dwBytesWritten )
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_DELAY failed, IOCTL bytes written = 0. "));
        tprResult = TPR_FAIL;
    }
    else
    {
        g_pKato->Log(LOG_COMMENT, TEXT("Original IIC_Delay = %x."),bufferIn[0]);

        iGetDelay = bufferOut[0];
        
        if( Clk_15 == iGetDelay )
        {
            iSetDelay = Clk_10;
        }
        else
        {
            iSetDelay = Clk_15;
        }
        
        bufferIn[0] = iSetDelay;

        g_pKato->Log(LOG_COMMENT, TEXT("Set IIC_Delay to %x."), iSetDelay);

        // Set I2C delay
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_SET_DELAY,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_SET_DELAY failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }  

        // Get I2C delay        
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_GET_DELAY,
                                      bufferIn, 
                                      dwSize, 
                                      bufferOut, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_DELAY failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }    

        if( 0 == dwBytesWritten )
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_GET_DELAY failed, IOCTL bytes written = 0. "));
            tprResult = TPR_FAIL;
        }
        else
        {
            g_pKato->Log(LOG_COMMENT, TEXT("Updated IIC_Delay = %x."), bufferIn[0]);

            iGetDelay = bufferOut[0];

            // Compare delay data
            if( iGetDelay == iSetDelay )
            {
                 g_pKato->Log(LOG_COMMENT, TEXT("IoctlDelayVerify succeeded. "));
            }
            else
            {
                 g_pKato->Log(LOG_FAIL, TEXT("IoctlDelayVerify failed, data does not match. "));
                 tprResult = TPR_FAIL;
            }
        }
    }

Exit:    
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;   
    }

    if ( TPR_PASS == tprResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("IOCTL_IIC_SET_DELAY and IOCTL_IIC_GET_DELAY functions test succeeded"));
    }

    return tprResult;
}


////////////////////////////////////////////////////////////////////////////////
// ThreadProc_InvalidOpen
//  Thread for I2CTUX_InvalidOpen.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_InvalidOpen(LPVOID lpParameter)
{
    HANDLE hDriver     = INVALID_HANDLE_VALUE;
    BOOL bResult       = TRUE;
    PCMD_DATA pCmdData = (PCMD_DATA)lpParameter;
    
    // Negative value for access code test     
    hDriver = CreateFile(_T("IIC0:"),-1,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL != hDriver && INVALID_HANDLE_VALUE != hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open with negative value for access code test failed"));

        pCmdData->bTestResult = FALSE;

        bResult = CloseHandle(hDriver);

        if ( FALSE == bResult )
        {
            g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }

    // Large value for access code test     
    hDriver = CreateFile(_T("IIC0:"),0xFFFFFFFF,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL != hDriver && INVALID_HANDLE_VALUE != hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open with large value for access code test failed"));

        pCmdData->bTestResult = FALSE;

        bResult = CloseHandle(hDriver);

        if ( FALSE == bResult )
        {
            g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
            pCmdData->bTestResult=FALSE;
        }
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_POWER_CAPABILITIES
//  Thread for I2CTUX_Invalid_IOCTL_POWER_CAPABILITIES.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_POWER_CAPABILITIES(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    DWORD     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwSize                 = 0;
    DWORD     dwBufferOutSize        = (BUFFER_SIZE);
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_POWER_CAPABILITIES with NULL pointer to output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_POWER_CAPABILITIES,
                                  NULL, 
                                  0, 
                                  NULL, // lpOutBuffer => NULL
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_POWER_CAPABILITIES: 'NULL pointer to output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    dwBytesWritten = 0;

    // Test IOCTL_POWER_CAPABILITIES with size 0 for output buffer size
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_POWER_CAPABILITIES,
                                  NULL, 
                                  0, 
                                  bufferOut,
                                  0, // nOutBufferSize => 0
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_POWER_CAPABILITIES: 'Size 0 for output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    dwBytesWritten = 0;

    // Test IOCTL_POWER_CAPABILITIES with NULL pointer for bytes returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_POWER_CAPABILITIES,
                                  NULL, 
                                  0, 
                                  bufferOut,
                                  dwBufferOutSize,
                                  NULL, // lpBytesReturned => NULL
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_POWER_CAPABILITIES: 'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    // Test IOCTL_POWER_CAPABILITIES with correct parameters
    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_POWER_CAPABILITIES,
                                  NULL, 
                                  0, 
                                  bufferOut,
                                  dwBufferOutSize,
                                  &dwBytesWritten,
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_POWER_CAPABILITIES: 'Correct parameters test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_POWER_CAPABILITIES
//  Verify IOCTL - IOCTL_POWER_CAPABILITIES handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_POWER_CAPABILITIES(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData = NULL;
    HANDLE hThread     = INVALID_HANDLE_VALUE;
    DWORD  dwWaitRet   = 0;
    DWORD  IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_POWER_CAPABILITIES_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_POWER_CAPABILITIES,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_POWER_CAPABILITIES test timed out."));
        pCmdData->bTestResult = FALSE; 
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_POWER_CAPABILITIES test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_POWER_CAPABILITIES test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_WRITE
//  Thread for I2CTUX_Invalid_IOCTL_IIC_WRITE.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_WRITE(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;   
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    TCHAR     bufferIn[BUFFER_SIZE]  = {0};
    TCHAR     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferInSize         = BUFFER_SIZE;
    DWORD     dwBufferOutSize        = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_WRITE with NULL pointer to input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_WRITE,
                                  NULL, // lpInBuffer
                                  dwBufferInSize, 
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_WRITE: 'NULL pointer to input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_WRITE with size 0 for input buffer size
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_WRITE,
                                  bufferIn, 
                                  0, // nInBufferSize
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_WRITE: 'Size 0 for input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    // Test IOCTL_IIC_WRITE with NULL pointer for bytes returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_WRITE,
                                  bufferIn, 
                                  dwBufferInSize,
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  NULL, // lpBytesReturned
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_WRITE: 'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    } 

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_WRITE
//  Verify IOCTL - IOCTL_IIC_WRITE handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_WRITE(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_WRITE_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_WRITE,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_WRITE test timed out."));
        pCmdData->bTestResult = FALSE;
        
    } 

    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_WRITE test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_WRITE test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_READ
//  Thread for I2CTUX_Invalid_IOCTL_IIC_READ.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_READ(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;   
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA) lpParameter;
    TCHAR     bufferIn[BUFFER_SIZE]  = {0};
    TCHAR     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferInSize         = BUFFER_SIZE;
    DWORD     dwBufferOutSize        = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_READ with NULL pointer to input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_READ,
                                  NULL, // lpInBuffer 
                                  dwBufferInSize, 
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_READ: 'NULL pointer to input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_READ with NULL pointer to output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_READ,
                                  bufferIn, 
                                  dwBufferInSize, 
                                  NULL, // lpOutBuffer
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_READ: NULL pointer to output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_READ with size 0 for input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_READ,
                                  bufferIn, 
                                  0, // nInBufferSize
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_READ: 'Size 0 for input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    
    
    // Test IOCTL_IIC_READ with size 0 for output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_READ,
                                  bufferIn, 
                                  dwBufferInSize, 
                                  bufferOut, 
                                  0, // nOutBufferSize
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_READ: 'Size 0 for output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    // Test IOCTL_IIC_READ with NULL pointer to bytes returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_READ,
                                  bufferIn, 
                                  dwBufferInSize, 
                                  bufferOut, 
                                  dwBufferOutSize,
                                  NULL, // lpBytesResturned
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_READ: 'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }       

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_READ
//  Verify IOCTL - IOCTL_IIC_READ handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_READ(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_READ_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_READ,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest, TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_READ test timed out."));
        pCmdData->bTestResult = FALSE;
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_READ test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_READ test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_SET_CLOCK
//  Thread for I2CTUX_Invalid_IOCTL_IIC_SET_CLOCK.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_SET_CLOCK(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    DWORD     bufferIn[BUFFER_SIZE]  = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferInSize         = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_SET_CLOCK with NULL pointer input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_CLOCK,
                                  NULL, // lpInBuffer
                                  dwBufferInSize, 
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_CLOCK: 'NULL pointer to input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    } 

    // Test IOCTL_IIC_SET_CLOCK with size 0 for input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_CLOCK,
                                  bufferIn,
                                  0, // nInBufferSize
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_CLOCK: 'Size 0 for input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_SET_CLOCK
//  Verify IOCTL - IOCTL_IIC_SET_CLOCK handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_SET_CLOCK(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_SET_CLOCK_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_SET_CLOCK,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest, TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_SET_CLOCK test timed out."));
        pCmdData->bTestResult = FALSE;
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_CLOCK test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_CLOCK test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_GET_CLOCK
//  Thread for I2CTUX_Invalid_IOCTL_IIC_GET_CLOCK.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_GET_CLOCK(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;   
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    TCHAR     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferOutSize        = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_GET_CLOCK with NULL pointer to ouput buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_CLOCK,
                                  NULL,
                                  0, 
                                  NULL, // lpOutBuffer
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_CLOCK: 'NULL pointer to output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_GET_CLOCK with size 0 for output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_CLOCK,
                                  NULL,
                                  0, 
                                  bufferOut,
                                  0, // nOutBufferSize
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_CLOCK: 'Size 0 for output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    
    
    // Test IOCTL_IIC_GET_CLOCK with NULL pointer to bytest returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_CLOCK,
                                  NULL,
                                  0, 
                                  bufferOut,
                                  dwBufferOutSize, 
                                  NULL, //lpBytesReturned
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_CLOCK: 'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_GET_CLOCK
//  Verify IOCTL - IOCTL_IIC_GET_CLOCK handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_GET_CLOCK(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_GET_CLOCK_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_GET_CLOCK,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_GET_CLOCK test timed out."));
        pCmdData->bTestResult = FALSE;
        
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_GET_CLOCK test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_GET_CLOCK test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_SET_FILTER
//  Thread for I2CTUX_Invalid_IOCTL_IIC_SET_FILTER.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_SET_FILTER(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    DWORD     bufferIn[BUFFER_SIZE]  = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferInSize         = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_SET_FILTER with NULL pointer to input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_FILTER,
                                  NULL, // lpInBuffer
                                  dwBufferInSize, 
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_FILTER: 'NULL pointer to input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_SET_FILTER with size 0 for input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_FILTER,
                                  bufferIn, 
                                  0, // nInBufferSize
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_FILTER: 'Size 0 for input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    
    
    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_SET_FILTER
//  Verify IOCTL - IOCTL_IIC_SET_FILTER handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_SET_FILTER(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_SET_FILTER_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_SET_FILTER,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_SET_FILTER test timed out."));
        pCmdData->bTestResult = FALSE;
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_FILTER test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_FILTER test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_GET_FILTER
//  Thread for I2CTUX_Invalid_IOCTL_IIC_GET_FILTER.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_GET_FILTER(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;   
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    TCHAR     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferOutSize        = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);
    
    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_GET_FILTER with NULL pointer to output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_FILTER,
                                  NULL, 
                                  0, 
                                  NULL, // lpOutBuffer
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_FILTER: 'NULL pointer to output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_GET_FILTER with size 0 for output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_FILTER,
                                  NULL, 
                                  0, 
                                  bufferOut, 
                                  0, // nOutBufferSize
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_FILTER: 'Size 0 for output bugger test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_GET_FILTER with NULL pointer to bytes returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_FILTER,
                                  NULL, 
                                  0,
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  NULL, // lpBytesReturned
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_FILTER:'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_GET_FILTER
//  Verify IOCTL - IOCTL_IIC_GET_FILTER handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_GET_FILTER(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_GET_FILTER_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_GET_FILTER,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_GET_FILTER test timed out."));
        pCmdData->bTestResult = FALSE;
        
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_GET_FILTER test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_GET_FILTER test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_SET_DELAY
//  Thread for I2CTUX_Invalid_IOCTL_IIC_SET_DELAY.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_SET_DELAY(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;   
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    DWORD     bufferIn[BUFFER_SIZE]  = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferInSize         = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    bufferIn[0] = Clk_15;

    // Test IOCTL_IIC_SET_DELAY with NULL pointer to input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_DELAY,
                                  NULL, // lpInBuffer
                                  dwBufferInSize, 
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_DELAY: 'NULL pointer to input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_SET_DELAY with size 0 for input buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_SET_DELAY,
                                  bufferIn, 
                                  0, // nInBufferSize
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_SET_DELAY: 'Size 0 for input buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    } 

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_SET_DELAY
//  Verify IOCTL - IOCTL_IIC_SET_DELAY handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_SET_DELAY(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_SET_DELAY_Event"));

    hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_SET_DELAY,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_SET_DELAY test timed out."));
        pCmdData->bTestResult = FALSE;
        
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }

    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_DELAY test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_SET_DELAY test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// ThreadProc_Invalid_IOCTL_IIC_GET_DELAY
//  Thread for I2CTUX_Invalid_IOCTL_IIC_GET_DELAY.
//
// Parameters:
//  lpParameter            command data.
//  
DWORD  ThreadProc_Invalid_IOCTL_IIC_GET_DELAY(LPVOID lpParameter)
{
    HANDLE    hDriver                = INVALID_HANDLE_VALUE;
    BOOL      bResult                = FALSE;
    PCMD_DATA pCmdData               = (PCMD_DATA)lpParameter;
    TCHAR     bufferOut[BUFFER_SIZE] = {0};
    DWORD     dwBytesWritten         = 0;
    DWORD     dwBufferOutSize        = BUFFER_SIZE;
    
    hDriver = CreateFile(_T("IIC0:"),0,0,NULL,OPEN_EXISTING,0,NULL);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        pCmdData->bTestResult=FALSE;
    }

    // Test IOCTL_IIC_GET_DELAY with NULL pointer ouput buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_DELAY,
                                  NULL, 
                                  0, 
                                  NULL, // lpOutBuffer 
                                  dwBufferOutSize, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_DELAY: 'NULL pointer to output test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    // Test IOCTL_IIC_GET_DELAY with size 0 for output buffer
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_DELAY,
                                  NULL, 
                                  0, 
                                  bufferOut, 
                                  0, // nOutBufferSize
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_DELAY: 'Size 0 for output buffer test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    
    
    // Test IOCTL_IIC_GET_DELAY with NULL pointer to bytes returned
    if ( FALSE != DeviceIoControl(hDriver, 
                                  IOCTL_IIC_GET_DELAY,
                                  NULL, 
                                  0,
                                  bufferOut, 
                                  dwBufferOutSize, 
                                  NULL, // lpBytesReturned
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("IOCTL_IIC_GET_DELAY: 'NULL pointer to bytes returned test' FAILED"));
        pCmdData->bTestResult = FALSE;
    }    

    bResult = CloseHandle(hDriver);

    if ( FALSE == bResult )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C close failed"));
        pCmdData->bTestResult = FALSE;
    }

    SetEvent(pCmdData->hWaitInvalidTest);

    return 0;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_Invalid_IOCTL_IIC_GET_DELAY
//  Verify IOCTL - IOCTL_IIC_GET_DELAY handle invalid parameters properly
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_Invalid_IOCTL_IIC_GET_DELAY(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    CMD_DATA *pCmdData   = NULL;
    HANDLE   hThread     = INVALID_HANDLE_VALUE;
    DWORD    dwWaitRet   = 0;
    DWORD    IDThreadKey = 0;
    
    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }

    pCmdData = new CMD_DATA;

    if ( NULL == pCmdData )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Memory allocation for pCmdData failed"));
        return TPR_FAIL;
    }

    pCmdData->bTestResult = TRUE;
    pCmdData->hWaitInvalidTest = CreateEvent(0,FALSE,FALSE,_T("I2C_IOCTL_IIC_GET_DELAY_Event"));

    hThread = CreateThread(0,0,(LPTHREAD_START_ROUTINE)ThreadProc_Invalid_IOCTL_IIC_GET_DELAY,(LPVOID)pCmdData,0,&IDThreadKey);
    dwWaitRet = WaitForSingleObject(pCmdData->hWaitInvalidTest,TIMEOUT_CONSTANT);

    if ( WAIT_TIMEOUT == dwWaitRet )
    {
        g_pKato->Log(LOG_COMMENT, TEXT("I2C IOCTL_IIC_GET_DELAY test timed out."));
        pCmdData->bTestResult = FALSE;
    } 
 
    if( !CloseHandle(hThread) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        pCmdData->bTestResult = FALSE;
    }
 
    if ( pCmdData->bTestResult )
    {
        g_pKato->Log(LOG_PASS, TEXT("I2C IOCTL_IIC_GET_DELAY test succeeded"));
        delete pCmdData;
        return TPR_PASS;
    } 
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C IOCTL_IIC_GET_DELAY test failed"));
        delete pCmdData;
        return TPR_FAIL;
    }
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_ReadPerformance
// Read data from I2C and check performance. Performance matric will be (data size / time to complete read operation)
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_ReadPerformance(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver        = INVALID_HANDLE_VALUE;
    HANDLE hCamera        = INVALID_HANDLE_VALUE;
    DWORD  tprResult      = TPR_PASS;
    DWORD  dwBytesWritten = 0;
    DWORD  dwSize         = 0;
    DWORD  dwErr          = ERROR_SUCCESS;
    BYTE   page[2]        = {0};
    BYTE   sizeValue[2]   = {0};
    int    iStartTick     = 0;
    int    iEndTick       = 0;
    int    iCostTime      = 0;
    int    i              = 0;
    UCHAR  Buff[IIC_READ_BUFFER_SIZE] = {0};
    IIC_IO_DESC IIC_Data;
    IIC_IO_DESC IIC_AddressData;

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }
    
    hDriver = CreateFile(_T("IIC0:"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        return tprResult;
    }
    
    hCamera = CreateFile(_T("CAM1:"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0);

    if ( NULL == hCamera || INVALID_HANDLE_VALUE == hCamera )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Camera open failed"));
        tprResult = TPR_FAIL;
        return tprResult;
    } 

    dwSize = sizeof(IIC_IO_DESC);

    // Read data(image format register) from the camera    module for 10 times and calculate the reading performance.
    // We should write register address and page number to the camera module, and then we can read data(image format register) from the camera module. 
    page[0]               = CAMERA_REG_ADDRESS;
    page[1]               = CAMERA_COMMAND_PAGE;
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Count        = DATA_COUNT_2;
    IIC_Data.Data         = (PUCHAR)page;

    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_IIC_WRITE,
                                  &IIC_Data, 
                                  dwSize, 
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_WRITE failed, IOCTL return FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }   

    sizeValue[0] = IMAGE_FORMAT_ADDRESS;
    
    IIC_AddressData.SlaveAddress = CAMERA_WRITE;    
    IIC_AddressData.Data         = (PUCHAR)sizeValue;
    IIC_AddressData.Count        = DATA_COUNT_1;
    IIC_Data.SlaveAddress        = CAMERA_READ;    
    IIC_Data.Data                = Buff;
    IIC_Data.Count               = DATA_COUNT_1;

    iStartTick = GetTickCount();

    for( i = 0; i < READTIMES; i++ )
    {
        if ( FALSE == DeviceIoControl(hDriver, 
                                      IOCTL_IIC_READ,
                                      &IIC_AddressData, 
                                      dwSize, 
                                      &IIC_Data, 
                                      dwSize, 
                                      &dwBytesWritten, 
                                      NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_READ failed, IOCTL return FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        } 
    }

    iEndTick  = GetTickCount();
    iCostTime = (iEndTick-iStartTick);

    if( TPR_FAIL != tprResult ) 
    {
        FLOAT fBps = CALCULATE_READ_PERFORMANCE(READTIMES,iCostTime); //((FLOAT)READTIMES * 1000/ (FLOAT)iCostTime);
        g_pKato->Log(LOG_PASS, TEXT("[TUXI2C]:Success to Read Size = 10 Bytes. CostTime: %4d mSec. Rate= %3.2f Bytes/Sec \n"), iCostTime, fBps); 
        g_pKato->Log(LOG_FAIL, TEXT("[TUXI2C]:I2CTUX_ReadPerformance test success. \r\n"));
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXI2C]:I2CTUX_ReadPerformance test failed. \r\n"));
    }
            
Exit:
    if( !CloseHandle(hCamera) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;        
    }
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function return FALSE"));
        tprResult = TPR_FAIL;        
    }

    return tprResult;
}



////////////////////////////////////////////////////////////////////////////////
// I2CTUX_WritePerformance
// Write data to I2C and check performance. Performance matric will be (data size / time to complete write operation)
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI I2CTUX_WritePerformance(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver        = INVALID_HANDLE_VALUE;
    HANDLE hCamera        = INVALID_HANDLE_VALUE;
    DWORD  tprResult      = TPR_PASS;
    DWORD  dwBytesWritten = 0;
    DWORD  dwSize         = 0;
    DWORD  dwErr          = ERROR_SUCCESS;
    BYTE   page[2]        = {0};
    int    iStartTick     = 0;
    int    iEndTick       = 0;
    int    iCostTime      = 0;
    int    i              = 0;
    IIC_IO_DESC IIC_Data;       

    // The shell does not necessarily want us to execute the test. Make sure first
    if( uMsg != TPM_EXECUTE )
    {
        return TPR_NOT_HANDLED;
    }
    
       hDriver = CreateFile(_T("IIC0:"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0);

    if ( NULL == hDriver || INVALID_HANDLE_VALUE == hDriver )
    {
        g_pKato->Log(LOG_FAIL, TEXT("I2C open failed"));
        tprResult = TPR_FAIL;
        return tprResult;
    }
    
    hCamera = CreateFile(_T("CAM1:"),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,0);

    if ( NULL == hCamera || INVALID_HANDLE_VALUE == hCamera )
    {
        g_pKato->Log(LOG_FAIL, TEXT("Camera open failed"));
        tprResult = TPR_FAIL;
        goto Exit;
    } 
 
    dwSize = sizeof(IIC_IO_DESC);

    // Write data(image format register) to the camera    module for 10 times and calculate the writing performance.
    // We should write register address and page number to the camera module, and then we can write data(image format register) to the camera module.     
    page[0]               = CAMERA_REG_ADDRESS;
    page[1]               = CAMERA_COMMAND_PAGE;
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Count        = DATA_COUNT_2;
    IIC_Data.Data         = (PUCHAR)page;

    if ( FALSE == DeviceIoControl(hDriver, 
                                  IOCTL_IIC_WRITE,
                                  &IIC_Data, 
                                  dwSize, 
                                  NULL, 
                                  0, 
                                  &dwBytesWritten, 
                                  NULL))
    {
        g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_WRITE failed, IOCTL returned FALSE"));
        tprResult = TPR_FAIL;
        goto Exit;
    }   

    page[0] = IMAGE_FORMAT_ADDRESS;
    page[1] = IMAGE_FORMAT_QVGA;    
    
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Count        = 2;
    IIC_Data.Data         = (PUCHAR)page;
   
    iStartTick = GetTickCount();

    for( i = 0; i < WRITETIMES; i++ )
    {
         if ( FALSE == DeviceIoControl(hDriver, 
                                       IOCTL_IIC_WRITE,
                                       &IIC_Data, 
                                       dwSize, 
                                       NULL,
                                       0, 
                                       &dwBytesWritten, 
                                       NULL))
        {
            g_pKato->Log(LOG_FAIL, TEXT("DeviceIoControl-IOCTL_IIC_WRITE failed, IOCTL returned FALSE"));
            tprResult = TPR_FAIL;
            goto Exit;
        }   
    }

    iEndTick  = GetTickCount();
    iCostTime = (iEndTick-iStartTick);

    if( TPR_FAIL != tprResult ) 
    {
        FLOAT fBps = CALCULATE_WRITE_PERFORMANCE(WRITETIMES,iCostTime);//((FLOAT) WRITETIMES * 3 * 1000 / (FLOAT)iCostTime);
        g_pKato->Log(LOG_PASS, TEXT("[TUXI2C]:Success to Write Size = %d Bytes(%d byte device address, %d byte register address and %d byte data). CostTime: %4d mSec. Rate= %3.2f Bytes/Sec \n"), WRITETIMES*3, WRITETIMES, WRITETIMES, WRITETIMES, iCostTime, fBps); 
        g_pKato->Log(LOG_FAIL, TEXT("[TUXI2C]:I2CTUX_ReadPerformance test success. \r\n"));
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("[TUXI2C]:I2CTUX_ReadPerformance test failed. \r\n"));
    }              

Exit:    
    if( !CloseHandle(hCamera) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        tprResult = TPR_FAIL;        
    }
    if( !CloseHandle(hDriver) )
    {
        g_pKato->Log(LOG_FAIL, TEXT("CloseHandle function returned FALSE"));
        tprResult = TPR_FAIL;        
    }
     
    return tprResult;
}