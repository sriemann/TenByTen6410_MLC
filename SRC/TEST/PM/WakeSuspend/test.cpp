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
//  WakeupSuspendTUX TUX DLL
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

BOOL g_bResult=FALSE;   //  Time gap exist.
BOOL g_bStop=FALSE;     //  Stop the ThreadProc_TimeCheck thread.
#define TIME_GAP 5      //  Time gap value.


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ThreadProc_TimeCheck
//    Check current system time and previous system time in while loop. If time gap exist, the system was indeed in suspend mode.
//
// Parameters:
//    NONE
//  
DWORD  ThreadProc_TimeCheck(void)
{
		
   SYSTEMTIME systime;
   WORD wPreMinute=0;     // Minute of previous system time
   WORD wPreSecond=0;     // Second of previous system time

   GetLocalTime(&systime);
   wPreMinute=systime.wMinute;   
   wPreSecond=systime.wSecond;   

   while(g_bStop==FALSE)
   { 
       GetLocalTime(&systime);
		
       if(systime.wMinute==wPreMinute )
       {
         if( (systime.wSecond - wPreSecond) > TIME_GAP )
         {
             g_bResult = TRUE;
         }
       }
       else
       {
           g_bResult = TRUE;
       }

       wPreMinute=systime.wMinute;
       wPreSecond=systime.wSecond;
       Sleep(1000);		  
   }

   return 0;
}

////////////////////////////////////////////////////////////////////////////////
// WakeupSuspendTUX_WakeupSuspend
//    Verify wakeup/suspend functions work correctly.
//
// Parameters:
//    uMsg            Message code.
//    tpParam         Additional message-dependent data.
//    lpFTE           Function table entry that generated this call.
//
// Return value:
//    TPR_PASS if the test passed, TPR_FAIL if the test fails, or possibly other
//    special conditions.

TESTPROCAPI WakeupSuspendTUX_WakeupSuspend(UINT uMsg, TPPARAM tpParam, LPFUNCTION_TABLE_ENTRY lpFTE)
{
   DWORD tprResult = TPR_PASS;
   HANDLE hThread;
   DWORD  IDThreadKey;
   DWORD  dwErr;
   WCHAR  state[1024] = {0};
   LPWSTR pState = &state[0];
   DWORD  dwStateFlags = 0;
   DWORD dwBufChars = (sizeof(state) / sizeof(state[0]));


   // The shell does not necessarily want us to execute the test. Make sure first
   if(uMsg != TPM_EXECUTE)
   {
       tprResult= TPR_NOT_HANDLED;
       goto exit;
   }

   // POWER_STATE_USERIDLE check
   dwErr=SetSystemPowerState(NULL,POWER_STATE_USERIDLE,POWER_FORCE);
   if (ERROR_SUCCESS != dwErr) 
   {
       g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_USERIDLE failed. Error code : %x "),dwErr);
       MessageBox(NULL,TEXT("SetSystemPowerState to POWER_STATE_USERIDLE failed. "),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
       goto exit;
   } 
   Sleep(1000);


   dwErr = GetSystemPowerState(pState, dwBufChars, &dwStateFlags);
   if (ERROR_SUCCESS != dwErr) 
   {
       g_pKato->Log(LOG_FAIL, TEXT("GetSystemPowerState failed."));
       MessageBox(NULL,TEXT("GetSystemPowerState failed. "),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
       goto exit;
   } 
   else 
   {
       g_pKato->Log(LOG_COMMENT, TEXT("System Power state is '%s', flags 0x%08x, %d \n"), state, dwStateFlags,(dwStateFlags & POWER_STATE_USERIDLE));

       if ((dwStateFlags & POWER_STATE_USERIDLE) == 0)
       {
           g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_USERIDLE failed.  System state not match."));
           MessageBox(NULL,TEXT("SetSystemPowerState to POWER_STATE_USERIDLE failed.  System state not match."),TEXT("test failed"),MB_OK);
           tprResult = TPR_FAIL;
           goto exit;
       }
       else
       {
           g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_USERIDLE succeeded."));
       }
   }
    
   // POWER_STATE_ON check 
   dwErr=SetSystemPowerState(NULL,POWER_STATE_ON,POWER_FORCE);
   if (ERROR_SUCCESS != dwErr) 
   {
       g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_ON failed. Error code : %x "),dwErr);
       MessageBox(NULL,TEXT("SetSystemPowerState to POWER_STATE_ON failed. "),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
       goto exit;
   } 
   Sleep(1000);


   dwErr = GetSystemPowerState(pState, dwBufChars, &dwStateFlags);
   if (ERROR_SUCCESS != dwErr) 
   {
       g_pKato->Log(LOG_FAIL, TEXT("GetSystemPowerState failed."));
       MessageBox(NULL,TEXT("GetSystemPowerState failed. "),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
       goto exit;
   } 
   else 
   {
       g_pKato->Log(LOG_COMMENT, TEXT("System Power state is '%s', flags 0x%08x\n"), state, dwStateFlags);

       if ((POWER_STATE(dwStateFlags) & POWER_STATE_ON )==0)
       {
           g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_ON failed.  System state not match."));
           MessageBox(NULL,TEXT("SetSystemPowerState to POWER_STATE_ON failed.  System state not match."),TEXT("test failed"),MB_OK);
           tprResult = TPR_FAIL;
           goto exit;
       }
       else
       {
           g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_ON succeeded."));
       }
   }


   hThread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)ThreadProc_TimeCheck, NULL, 0,&IDThreadKey);

   MessageBox(NULL,TEXT("Please press [ok] and system will suspend, wait for 10 second and then press power button to wakeup the system. "),TEXT("Wakeup/Suspend test"),MB_OK);
	
   // POWER_STATE_SUSPEND check 		
   dwErr=SetSystemPowerState(NULL,POWER_STATE_SUSPEND,POWER_FORCE);
   if (ERROR_SUCCESS != dwErr) 
   {
       g_pKato->Log(LOG_FAIL, TEXT("SetSystemPowerState to POWER_STATE_SUSPEND failed."));
       MessageBox(NULL,TEXT("SetSystemPowerState to POWER_STATE_SUSPEND failed. "),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
       goto exit;
   } 
	
   Sleep(1000);

   MessageBox(NULL,TEXT("Test complete, please press [ok] "),TEXT("test complete"),MB_OK);
   g_bStop = TRUE;



   if (g_bResult==FALSE)
   {
       g_pKato->Log(LOG_FAIL, TEXT("Wakeup/Suspend test failed"));
       MessageBox(NULL,TEXT("Wakeup/Suspend test failed."),TEXT("test failed"),MB_OK);
       tprResult = TPR_FAIL;
   }

   
   if (tprResult == TPR_PASS)
   {
       g_pKato->Log(LOG_PASS, TEXT("Wakeup/Suspend test succeeded."));
       MessageBox(NULL,TEXT("Wakeup/Suspend test succeeded."),TEXT("test succeeded"),MB_OK);
   }
   
exit:
   if( hThread!=NULL) 
   {
   	if(!CloseHandle(hThread))
   	{
   	  g_pKato->Log(LOG_COMMENT, TEXT("Can not close the handle of thread!"));   	
   	}
   }       
   return tprResult;
}

////////////////////////////////////////////////////////////////////////////////
