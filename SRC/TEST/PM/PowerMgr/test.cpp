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
//  PMTux TUX DLL
//
//  Module: test.cpp
//          Contains the test functions.
//
//  Revision History:
//
////////////////////////////////////////////////////////////////////////////////

#include "main.h"
#include "globals.h"
#include "pm.h"
#include "winioctl.h"

#define ICLASS_SINGLE_SIZE 40       // ICLASS string size
#define ICLASS_DOUBLE_SIZE 80       // Two ICLASS string size
#define DEVICE_PREFIX_SIZE 3        // Device prefix string size
#define INT_TO_ASCII_SHIFT 48       // number to ASCII char
#define DEVICE_POWER_STATE 4        // Device power state. D0~D4

////////////////////////////////////////////////////////////////////////////////
// PMTUX_DevicePowerStateVerify
//  Verify all devices under power management can be set to all supported power state
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI PMTUX_DevicePowerStateVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD tprResult = TPR_PASS;
    DWORD dwErr;
    CEDEVICE_POWER_STATE dxState;
    DWORD dwIndex = 0;
    TCHAR szNewKey[MAX_PATH];
    TCHAR szPrefix[10];
    TCHAR szIClass[ICLASS_DOUBLE_SIZE];
    TCHAR szBuffer1[ICLASS_SINGLE_SIZE];
    TCHAR szBuffer2[ICLASS_SINGLE_SIZE];
    TCHAR szPMIclass1[ICLASS_SINGLE_SIZE]=TEXT("{A32942B7-920C-486b-B0E6-92A702A99B35}");  // for generic power-managed devices.
    TCHAR szPMIclass2[ICLASS_SINGLE_SIZE]=TEXT("{8DD679CE-8AB4-43c8-A14A-EA4963FAA715}");  // for power-managed block devices.
    TCHAR szPMIclass3[ICLASS_SINGLE_SIZE]=TEXT("{98C5250D-C29A-4985-AE5F-AFE5367E5006}");  // for power-managed NDIS miniports.
    DWORD dwPrifixIndex=0;
    DWORD dwNewKeySize=MAX_PATH;
    HKEY hKey,hKey2;
    DWORD dwValueType,dwValueLength;
    DWORD dwIndexValueType,dwIndexValueLength;
    TCHAR bufferIn[256];   // Input buffer for DeviceIoControl
    TCHAR bufferOut[256];  // Output buffer for DeviceIoControl
    DWORD dwBytesWritten = 0;
    PPOWER_CAPABILITIES ppc;
    TCHAR strDevice[6];
    int i,j;
        


    // The shell does not necessarily want us to execute the test. Make sure first
    if(uMsg != TPM_EXECUTE)
    {
        return TPR_NOT_HANDLED;
    }


    dwErr=RegOpenKeyEx(HKEY_LOCAL_MACHINE,L"Drivers\\BuiltIn",0,0,&hKey);
    if(dwErr != ERROR_SUCCESS)
    {
        g_pKato->Log(LOG_FAIL, TEXT("RegOpenKeyEx failed, Error Code: %d"),dwErr);
        return TPR_FAIL;
    }

    if(ERROR_SUCCESS== RegEnumKeyEx(hKey,dwIndex,szNewKey,&dwNewKeySize,NULL,NULL,NULL,NULL))
    {
      do
      {
        /////g_pKato->Log(LOG_FAIL, TEXT("FIND:%s"),szNewKey);


        dwIndex+=1;
        dwNewKeySize= (sizeof(szNewKey) / sizeof(TCHAR));
        
        if(ERROR_SUCCESS != RegOpenKeyEx(hKey,szNewKey,0,0,&hKey2))
        {
                g_pKato->Log(LOG_FAIL, TEXT("RegOpenKeyEx error"));
        }
        else
        {
            dwValueLength= sizeof(szIClass);
            dwValueType=REG_MULTI_SZ;
            for(i=0;i<ICLASS_DOUBLE_SIZE;i++)
            {
                 szIClass[i]='\0';
            }
            if(ERROR_SUCCESS == RegQueryValueEx(hKey2,L"IClass",NULL,&dwValueType,(PBYTE)szIClass,&dwValueLength))
            {
                 
                  if(dwValueType==REG_MULTI_SZ)
                  {
                      szBuffer1[0]='\0';
                      szBuffer2[0]='\0';
                      for(i=0;i<(int)dwValueLength;i++)
                      {               
                          szBuffer1[i]= szIClass[i];
                          if(szIClass[i]=='}')
                          {
                              szBuffer1[i+1]='\0';
                              break;
                          }
                      }
                      for(;i<(int)dwValueLength;i++)
                      {
                          if(szIClass[i]=='{')
                             {
                             break;
                          }
                      }
                      for(j=0;i<(int)dwValueLength;i++,j++)
                      {               
                          szBuffer2[j]= szIClass[i];
                          if(szIClass[i]=='}')
                          {
                              szBuffer2[j+1]='\0';
                              break;
                          }
                      }
                      

                      if(  wcscmp((const wchar_t *)szBuffer1,(const wchar_t *)szPMIclass1)!=0 
                           && wcscmp((const wchar_t *)szBuffer1,(const wchar_t *)szPMIclass2)!=0  
                           && wcscmp((const wchar_t *)szBuffer1,(const wchar_t *)szPMIclass3)!=0 
                           && wcscmp((const wchar_t *)szBuffer2,(const wchar_t *)szPMIclass1)!=0 
                           && wcscmp((const wchar_t *)szBuffer2,(const wchar_t *)szPMIclass2)!=0  
                           && wcscmp((const wchar_t *)szBuffer2,(const wchar_t *)szPMIclass3)!=0 )
                      {
                         continue;
                      }
             
                  }
                  else
                  {
                     
                      if(  wcscmp((const wchar_t *)szIClass,(const wchar_t *)szPMIclass1)!=0 
                           && wcscmp((const wchar_t *)szIClass,(const wchar_t *)szPMIclass2)!=0  
                           && wcscmp((const wchar_t *)szIClass,(const wchar_t *)szPMIclass3)!=0 )
                      {                      
                         continue;
                      }
                  } 
                 
            
                  dwValueLength= sizeof(szPrefix);
                  dwValueType=REG_SZ;
                  if(ERROR_SUCCESS != RegQueryValueEx(hKey2,L"Prefix",NULL,&dwValueType,(PBYTE)szPrefix,&dwValueLength))
                  {
                       szPrefix[0]='\0';
                  }
                  dwIndexValueLength= sizeof(dwPrifixIndex);
                  dwIndexValueType=REG_DWORD;
                  if(ERROR_SUCCESS == RegQueryValueEx(hKey2,L"Index",NULL,&dwIndexValueType,(PBYTE)&dwPrifixIndex,&dwIndexValueLength))
                  {
                        // make device name
                        for(int i=0;i<DEVICE_PREFIX_SIZE;i++)
                        {
                            strDevice[i]=szPrefix[i];
                        }
                        strDevice[3]=(TCHAR)(INT_TO_ASCII_SHIFT+dwPrifixIndex);   // convert index from dword to char
                        strDevice[4]=':';
                        strDevice[5]='\0';

                        hDriver = CreateFile(strDevice ,  0, 0, NULL,  OPEN_EXISTING, 0, NULL );
    
                        if ( hDriver == NULL || hDriver == INVALID_HANDLE_VALUE )
                        {
                            g_pKato->Log(LOG_FAIL, TEXT("%s open failed"),strDevice);
                            tprResult = TPR_FAIL;
                            //return tprResult;
                            continue;
                        }

                        DWORD dwSize = sizeof(POWER_CAPABILITIES);

                        if (FALSE == DeviceIoControl(hDriver, 
                                 IOCTL_POWER_CAPABILITIES,
                                 bufferIn, 
                                 dwSize, 
                                 bufferOut, 
                                 dwSize, 
                                 &dwBytesWritten, 
                                 NULL))
                        {
                             g_pKato->Log(LOG_FAIL, TEXT("%s IOCTL_POWER_CAPABILITIES return FALSE"),strDevice);
                             ppc = NULL;

                             if(  wcscmp((const wchar_t *)szPrefix,(const wchar_t *)TEXT("MFC"))==0 
                                  || wcscmp((const wchar_t *)szPrefix,(const wchar_t *)TEXT("NDS"))==0)
                             {    
                                // MFC and NDIS driverse do not support calling IOCTL_POWER_CAPABILITIES IOControl by applications.
                                g_pKato->Log(LOG_COMMENT, TEXT("%s driver does not support calling IOCTL_POWER_CAPABILITIES IOControl by applications."),szPrefix); 
                             }
                            
                             else
                             {
                                tprResult = TPR_FAIL;
                             }
                        }    
                        else
                        {
                             ppc = (PPOWER_CAPABILITIES )bufferOut;
                        }
    
                        CloseHandle(hDriver);

                        
                        if(ppc != NULL)
                        {
                            
                             for(int i=0;i<=DEVICE_POWER_STATE;i++)
                             {
                                if(ppc->DeviceDx & DX_MASK(i))
                                {
                                    dwErr= SetDevicePower(strDevice,POWER_NAME,(CEDEVICE_POWER_STATE)i);
                                       if(dwErr != ERROR_SUCCESS)
                                    {
                                        g_pKato->Log(LOG_FAIL, TEXT("SetDevicePower D%d to %s error"),i,strDevice);
                                        tprResult = TPR_FAIL;
                                    }
                                    else
                                    {
                                        Sleep(100);
                                        dwErr=GetDevicePower(strDevice,  POWER_NAME, &dxState );
                                        if(dwErr != ERROR_SUCCESS)
                                        {
                                           g_pKato->Log(LOG_FAIL, TEXT("%s GetDevicePower failed , Error Code: %d"),strDevice,dwErr);
                                           dxState=PwrDeviceUnspecified ;
                                           tprResult = TPR_FAIL;
                                        }

                                        if(dxState!=i)
                                        {
                                             g_pKato->Log(LOG_FAIL, TEXT("%s power state (D%d)not match."),strDevice,i);
                                             tprResult = TPR_FAIL;
                                        }
                                        else
                                        {
                                             g_pKato->Log(LOG_COMMENT, TEXT("%s power state (D%d) succeeded."),strDevice,i);
                                        }

                                    }

                                }
                             }
                             dwErr=SetDevicePower( strDevice,  POWER_NAME, PwrDeviceUnspecified);
                             if(dwErr != ERROR_SUCCESS)
                             {
                                 g_pKato->Log(LOG_FAIL, TEXT("SetDevicePower to PwrDeviceUnspecified failed, Error Code: %d"),dwErr);
        
                             }

                        }
                        

                  }
                  
            }
        }

      }while(ERROR_SUCCESS == RegEnumKeyEx(hKey,dwIndex,szNewKey,&dwNewKeySize,NULL,NULL,NULL,NULL));
    }
    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("RegEnumKeyEx error."),szPrefix);
    }

    if (tprResult == TPR_PASS)
    {
        g_pKato->Log(LOG_PASS, TEXT("PMTUX_DevicePowerStateVerify test succeeded"));
    }        
    return tprResult;

}


////////////////////////////////////////////////////////////////////////////////
// PMTUX_PowerRequirementVerify
//  Verify Windows CE power management APIs
//
// Parameters:
//  uMsg            Message code.
//  tpParam         Additional message-dependent data.
//  lpFTE           Function table entry that generated this call.
//
// Return value:
//  TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//  special conditions.
TESTPROCAPI PMTUX_PowerRequirementVerify(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
    HANDLE hDriver = INVALID_HANDLE_VALUE;
    DWORD tprResult = TPR_PASS;  
    DWORD dwErr;
    HANDLE hResult;
    CEDEVICE_POWER_STATE dxState;

    // The shell does not necessarily want us to execute the test. Make sure first
    if(uMsg != TPM_EXECUTE)
    {
        tprResult = TPR_NOT_HANDLED;
        goto exit;
    }

    dwErr=SetSystemPowerState(NULL,POWER_STATE_ON,POWER_FORCE);
    if(ERROR_SUCCESS !=dwErr)
    {
        g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState failed, Error Code: %d"),dwErr);        
        tprResult = TPR_FAIL;
        goto exit;
    }
    
    hResult=SetPowerRequirement ( _T("BKL1:"),D0, POWER_NAME | POWER_FORCE, NULL, 0 );
    if(hResult==NULL)
    {
        g_pKato->Log(LOG_FAIL, TEXT("BKL1: SetPowerRequirement to D0 failed"));
        tprResult = TPR_FAIL;
        goto exit;
    }

    Sleep(100);
  

    dwErr=SetSystemPowerState(NULL,POWER_STATE_USERIDLE,POWER_NAME);
    if(ERROR_SUCCESS !=dwErr)
    {
        g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState failed, Error Code: %d"),dwErr);
        tprResult = TPR_FAIL;
        goto exit;
    }

    Sleep(100);



    dwErr=GetDevicePower( _T("BKL1:"),  POWER_NAME, &dxState );
    if(dwErr != ERROR_SUCCESS)
    {
        g_pKato->Log(LOG_FAIL, TEXT("GetDevicePower failed "));
        tprResult = TPR_FAIL;
        goto exit;
    }

    if(dxState==D0)
    {
       tprResult = TPR_PASS;
    }

    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("PMTUX_PowerRequirementVerify test failed, power state not match. State:D%d"),dxState==D1?1:-1);
        tprResult = TPR_FAIL;
        goto exit;
    }

    dwErr=ReleasePowerRequirement ( hResult );
    if(dwErr != ERROR_SUCCESS)
    {
        g_pKato->Log(LOG_FAIL, TEXT("ReleasePowerRequirement failed, Error Code: %d"),dwErr);
        tprResult = TPR_FAIL;
        goto exit;
    }

    Sleep(100);

    dwErr=GetDevicePower( _T("BKL1:"),  POWER_NAME, &dxState );
    if(dwErr != ERROR_SUCCESS)
    {
        g_pKato->Log(LOG_FAIL, TEXT("GetDevicePower failed "));
        tprResult = TPR_FAIL;
        goto exit;
    }

    if(dxState==D1)
    {
       tprResult = TPR_PASS;
    }

    else
    {
        g_pKato->Log(LOG_FAIL, TEXT("PMTUX_PowerRequirementVerify test failed, power state not match. State:D%d"),dxState==D0?0:-1);
        tprResult = TPR_FAIL;
        goto exit;
    }


    dwErr=SetSystemPowerState(NULL,POWER_STATE_ON,POWER_NAME);
    if(ERROR_SUCCESS !=dwErr)
    {
        g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState failed, Error Code: %d"),dwErr);
        tprResult = TPR_FAIL;
        goto exit;
    }
 

    if (tprResult == TPR_PASS)
    {
        g_pKato->Log(LOG_PASS, TEXT("PMTUX_PowerRequirementVerify test succeeded"));
    }        
    
exit:    
    return tprResult;

}

////////////////////////////////////////////////////////////////////////////////
