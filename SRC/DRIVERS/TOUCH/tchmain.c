//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

@doc EX_TOUCH_DDI INTERNAL DRIVERS MDD TOUCH_PANEL

Module Name:  

@module tchmain.c

Abstract:  
    This module contains the DDI implementation and the supporting administriva.
    if DBGCAL is defined, the results of the calibration calculations are
    displayed following the setting of calibration data. <nl>


Functions:
TouchPanelpDetach
TouchPanelpAttach
TouchPanelpISR
TouchPanelGetDeviceCaps
TouchPanelSetMode
TouchPanelPowerHandler
TouchPanelEnable
TouchPanelDisable
TouchPanelReadCalibrationPoint
TouchPanelReadCalibrationAbort
Notes: 


--*/

#include    <windows.h>
#include    <types.h>
#include    <memory.h>
#include    <nkintr.h>
#include    <tchddi.h>
#include    <tchddsi.h>

#ifndef _PREFAST_
#pragma warning(disable: 4068) // Disable pragma warnings
#endif

extern PFN_TOUCH_PANEL_CALLBACK v_pfnCgrPointCallback;
extern PFN_TOUCH_PANEL_CALLBACK v_pfnCgrCallback;
// To track if we are in OOM state.
BOOL _bTchThreadHighPriority = FALSE;
//
// Run at high priority so that we can service our interrupts quickly.
//
#define DEFAULT_THREAD_PRIORITY         109
#define DEFAULT_THREAD_HIGH_PRIORITY    109

#define CAL_DELTA_RESET         20
#define CAL_HOLD_STEADY_TIME    1500


// If we are a DLL, we can define our dpCurSettings
#ifdef DEBUG
DBGPARAM dpCurSettings = {
    TEXT("Touch"), { 
    TEXT("Samples"),TEXT("Calibrate"),TEXT("Stats"),TEXT("Thread"),
    TEXT("TipState"),TEXT("Init"),TEXT(""),TEXT(""),
    TEXT(""),TEXT("Misc"),TEXT("Delays"),TEXT("Timing"),
    TEXT("Alloc"),TEXT("Function"),TEXT("Warning"),TEXT("Error") },
    0xC020              // warning, error, init
};
#endif

//
// Calibration State defintions
//
// @const LONG | CalibrationAvailable |
// Indicates that calibration is not in progress and is available for use.
//
// @const LONG | CalibrationActive |
// Indicates that calibration is active.
//
// @const LONG | CalibrationInactive |
// Indicates that calibration is inactive and waiting for completion.
//
// @const LONG | CalibrationAborted |
// Indicates that calibration is in the process of aborting via user request.
//

#define CalibrationInactive     0x00
#define CalibrationWaiting      0x01
#define CalibrationDown         0x02
#define CalibrationValid        0x03
#define CalibrationAborted      0x04


// Scale factor to support sub-pixel resolutions
#define X_SCALE_FACTOR 4
#define Y_SCALE_FACTOR 4

//
// Macro for absolute value.
//

#define ABS(x)  ((x) >= 0 ? (x) : (-(x)))

//
// Internal Function Prototypes
//

static ULONG
TouchPanelpISR(
    PVOID   Reserved
    );


ULONG   culReferenceCount;              //@globalvar ULONG | culReferenceCount | Count of attached threads

HANDLE  hThread=NULL;                        //@globalvar HANDLE | hThread | Handle of attached thread

CRITICAL_SECTION    csMutex;            //@globalvar CRITICAL_SECTION | csMutex | Critical section

HANDLE  hTouchPanelEvent;               //@globalvar HANDLE | hTouchPanelEvent | Holds the event handle for
                                            // touch panel event notification.

HANDLE  hCalibrationSampleAvailable;    //@globalvar HANDLE  | hCalibrationSampleAvailable | Holds the event handle for
                                            // notification that a calibration
                                            // mapping point is available.

INT     CalibrationState;             //@globalvar INT | CalibrationState | Flag which indicates the
                                            // state of calibration: Available,
                                            // Active or Inactive.

static DWORD gThreadPriority;           //@globalvar DWORD | gThreadPriority | Interrupt thread normal priority
static DWORD gThreadHighPriority;       //@globalvar DWORD | gThreadHighPriority | Interrupt thread high priority
static BOOL bTerminate=FALSE;


// The MIN_CAL_COUNT is defined in the PDD, since each touch panel
// has different characteristics.  The value should be high enough
// to prevent spurious touches, but low enough that the user doesn't
// have to hold the pen on each crosshair too long.
extern int MIN_CAL_COUNT;

LONG    lCalibrationXCoord;             //@globalvar LONG | lCalibrationXCoord | Holds the X coordinate
                                            // corresponding to the touch.
LONG    lCalibrationYCoord;             //@globalvar  LONG | lCalibrationYCoord | Holds the Y coordinate
                                            // corresponding to the touch.

HANDLE  ghevCalibrationActivity;        // activity event used to notify the Power Manager of touch 
                                        // events during calibration

INT32 DisplayWidth;
INT32 DisplayHeight;

DWORD gdwTouchIstTimeout = INFINITE;    // hold csMutex when accessing this

//
//@globalvar PFN_TOUCH_PANEL_CALLBACK | v_pfnPointCallback |
// Pointer to the application supplied function for receiving points.
//
PFN_TOUCH_PANEL_CALLBACK v_pfnPointCallback;

//**********************************************************************
// The following routines are internal helpers, and are not visible to
// the DDI layer.
// @doc IN_TOUCH_DDI INTERNAL DRIVERS MDD TOUCH_PANEL
//**********************************************************************

/*++

Autodoc Information:

    @func VOID | TouchPanelpDetach |
    Performs cleanup and frees memory when owning process detaches.

    @devnote
    We let ExitProcess handle the shutting down of the ISR thread.

--*/
static VOID
TouchPanelpDetach(
    VOID
    )
{

}


/*++


Autodoc Information:

    @func BOOL | TouchPanelpAttach |
    This routine performs the initialization for the touch panel.

    @rdesc
    If the function succeeds the return value is TRUE, otherwise, it is FALSE.

--*/
static BOOL
TouchPanelpAttach(
    VOID
    )
{
    //
    // Create the event for touch panel events.
    // If creation fails, return failure.
    //

    hTouchPanelEvent = CreateEvent( NULL,
                                                     FALSE,     //  Not manual reset
                                                     FALSE,     //  Not signalled
                                                     NULL
                                                     );

    if ( !hTouchPanelEvent )
        return ( FALSE );

    //
    // Create the event for signaling when a calibration sample has been sent.
    //

    hCalibrationSampleAvailable =
        CreateEvent( NULL,
                     FALSE,     //  Not manual reset
                     FALSE,     //  Not signalled
                     NULL
                    );

    if ( !hCalibrationSampleAvailable )
        return ( FALSE );

    DdsiTouchPanelDisable();    // Insure the device is disabled

    //
    // Initialize the critical section, flags, callbacks, reference count,
    // sample rate.
    //

    InitializeCriticalSection( &csMutex );
    CalibrationState = CalibrationInactive;
    v_pfnPointCallback = NULL;
    culReferenceCount = 0;


    //
    // Initialize calibration info used to convert uncalibrated to calibrated
    // points so that function is a noop.
    //

    TouchPanelSetCalibration( 0, NULL, NULL, NULL, NULL );

    return ( TRUE );
}

/*++

Autodoc Information:

    @func ULONG | TouchPanelpISR |
    This routine is the thread which handles touch panel events.
    The event that this thread synchronizes on is signaled by the PDD based on
    the sampling rate, typically 10ms.

    @rdesc
    Never returns.

--*/
static ULONG
TouchPanelpISR(
    PVOID   Reserved  //@parm Reserved, not used.
    )
{
    TOUCH_PANEL_SAMPLE_FLAGS    SampleFlags = 0;
    INT32                       RawX, CalX;
    INT32                       RawY, CalY;
    UINT32                      MaxX =  DisplayWidth * X_SCALE_FACTOR;
    UINT32                      MaxY =  DisplayHeight * Y_SCALE_FACTOR;
    UINT32                      CurrentDown = 0;
    static LONG CX;
    static LONG CY;
    static LONG XBase;
    static LONG YBase;
    static int  CalibrationSampleCount;
    static BOOL     fSetBase;
    static DWORD    BaseTime;
    static BOOL     fGotSample;

    PFN_TOUCH_PANEL_CALLBACK pfnCallback;

    // Need to be all kmode so that we can write to shared memory.
    //

    while  ( !bTerminate )
    {

        WaitForSingleObject( hTouchPanelEvent, gdwTouchIstTimeout );
        EnterCriticalSection( &csMutex );
        DEBUGMSG(ZONE_THREAD, (TEXT("TCH_INTR\r\n")) );

        // Give the pdd the down state of the previous sample
        if ( CurrentDown )
            SampleFlags |= TouchSamplePreviousDownFlag;
        else
            SampleFlags &= ~TouchSamplePreviousDownFlag;

        DdsiTouchPanelGetPoint( &SampleFlags, &RawX, &RawY );    // Get the point info
        
        if ( SampleFlags & TouchSampleIgnore )
        {
            // do nothing, not a valid sample
            LeaveCriticalSection( &csMutex );
            continue;
        }

        if ( SampleFlags & TouchSampleValidFlag )
            {
            // Set the previous down state for our use, since the pdd may not
            // have preserved it.
            if ( CurrentDown )
                SampleFlags |= TouchSamplePreviousDownFlag;
            else
                SampleFlags &= ~TouchSamplePreviousDownFlag;

            CurrentDown = SampleFlags & TouchSampleDownFlag;
            }

        if ( CalibrationState )
        {
            //
            // At this point we know that calibration is active.
            //
            // Typically, the user touches the panel then converges to the
            // displayed crosshair. When the tip state transitions to
            // the up state, we forward the last valid point to the callback
            // function.
            //
            DEBUGMSG(ZONE_SAMPLES, (TEXT("**** Calibration point (%d, %d), flags 0x%4.4X\r\n"),
                   RawX, RawY, SampleFlags) );

//  Skip if not valid.
            if ( !(SampleFlags & TouchSampleValidFlag) )
                {
                LeaveCriticalSection( &csMutex );
                continue;
                }

//  Signal the Power Manager activity event if one has been set up
            if ( ghevCalibrationActivity != NULL) 
                {
                SetEvent(ghevCalibrationActivity);
                }

//  Must see down transition.
            if ( (SampleFlags & (TouchSampleDownFlag|TouchSamplePreviousDownFlag)) ==
                        TouchSampleDownFlag )
                {
                CalibrationState = CalibrationDown;
                fSetBase = TRUE;
                CalibrationSampleCount = 0;
                fGotSample = FALSE;
                }

//  Only look at stuff if we saw a down transition.
            if ( (CalibrationState == CalibrationDown) && !fGotSample )
                {
                if ( SampleFlags & TouchSampleDownFlag )
                    {
                    long DeltaX, DeltaY;

                    CalibrationSampleCount++;
                    CX = RawX;
                    CY = RawY;
                    if ( fSetBase )
                        {
                        XBase = CX;
                        YBase = CY;
                        BaseTime = GetTickCount();
                        fSetBase = FALSE;
                        }
                    DeltaX = CX - XBase;
                    DeltaY = CY - YBase;
                    if ( (GetTickCount() - BaseTime) > CAL_HOLD_STEADY_TIME )
                        {
                        fGotSample = TRUE;
                        }
                    else if ( ( ABS(DeltaX) > CAL_DELTA_RESET ) ||
                              ( ABS(DeltaY) > CAL_DELTA_RESET ) )
                        {
                        RETAILMSG(1, (TEXT("M %ld,%ld  %ld,%ld  %ld,%ld"),
                            XBase,YBase, CX,CY, DeltaX,DeltaY));
                        fSetBase = TRUE;
                        }
                    }
                else
                    {
                     // They lifted the pen, see if we will accept coordinate.
                    if ( CalibrationSampleCount >= MIN_CAL_COUNT )
                        {
                        fGotSample = TRUE;
                        }
                    else
                        {
                        CalibrationState = CalibrationWaiting;
                        }
                    }

                if ( fGotSample )
                    {
                    CalibrationState = CalibrationValid;
                    lCalibrationXCoord = CX;
                    lCalibrationYCoord = CY;
                    SetEvent(hCalibrationSampleAvailable);
                    }
                }
            LeaveCriticalSection( &csMutex );
        }
        else
        {
            pfnCallback = v_pfnPointCallback;
            if ( pfnCallback != NULL )
            {
                if( SampleFlags & TouchSampleIsCalibratedFlag )
                {   // Sample already calibrated by PDD
                    CalX = RawX;
                    CalY = RawY;
                }
                else
                {   // Not previously calibrated, do it now.
                    TouchPanelCalibrateAPoint( RawX, RawY, &CalX, &CalY );
                    SampleFlags |= TouchSampleIsCalibratedFlag;
                }
                
                LeaveCriticalSection( &csMutex );

                 // Bounds check this value
                if( CalX < 0 )
                    CalX = 0;
                else if( MaxX && ((UINT32)CalX >= MaxX) )
                    CalX = MaxX - X_SCALE_FACTOR;
                if( CalY < 0 )
                    CalY = 0;
                else if( MaxY && ((UINT32)CalY >= MaxY) )
                    CalY = MaxY - Y_SCALE_FACTOR ;
                
                DEBUGMSG( ZONE_SAMPLES,
                          (TEXT("**** Queuing point (%d, %d), flags 0x%4.4X\r\n"),
                           CalX, CalY, SampleFlags) );
               RETAILMSG( 0,
                          (TEXT("**** Queuing point (%d, %d), flags 0x%4.4X\r\n"),
                           CalX, CalY, SampleFlags) );           
#ifdef DEBUG
                {
                    static DWORD SampleCt;
                    
                    if( SampleFlags & TouchSampleDownFlag )
                        SampleCt++;
                    else
                    {
                        DEBUGMSG( ZONE_TIMING,
                                  (TEXT("%d down samples queued\r\n"),
                                   SampleCt) );
                        SampleCt = 0;
                    }
                }
                
#endif                
                (pfnCallback)( SampleFlags, CalX, CalY);
            }
            else
            {
                LeaveCriticalSection( &csMutex );
            }

        }
    }
    ExitThread(1);
    return ( TRUE );
}


#define KEYNAME_TOUCH_DRIVER        TEXT("\\Drivers\\BuiltIn\\Touch")
#define VALNAME_THREAD_PRIO         TEXT("Priority256")
#define VALNAME_THREAD_HIGH_PRIO    TEXT("HighPriority256")

/*++

Autodoc Information:

    @func DWORD | TouchPanelpGetPriority |
    This routine reads the TouchPanelpISR thread priority from the registry.

--*/
static VOID TouchPanelpGetPriority(DWORD *ThrdPrio, DWORD *ThrdHighPrio)
{
    HKEY hKey;
    DWORD dwType;
    DWORD dwSize;
    DWORD dwStatus;

    dwStatus = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    KEYNAME_TOUCH_DRIVER,
                    0,
                    0,
                    &hKey
                    );
                    
    if (dwStatus)
    {
        DEBUGMSG(ZONE_INIT | ZONE_WARN,
            (TEXT("TOUCH:TouchPanelpGetPriority - RegOpenKeyEx(%s) failed %d, using default thread priorities\n"),
            KEYNAME_TOUCH_DRIVER, dwStatus));
        *ThrdPrio = DEFAULT_THREAD_PRIORITY;
        *ThrdHighPrio = DEFAULT_THREAD_HIGH_PRIORITY;
        return;
    }

    dwSize = sizeof(DWORD);
    dwStatus = RegQueryValueEx(
                    hKey,
                    VALNAME_THREAD_PRIO,
                    0,
                    &dwType,
                    (PUCHAR)ThrdPrio,
                    &dwSize
                    );
                    
    if (dwStatus)
    {
        DEBUGMSG(ZONE_INIT | ZONE_WARN,
            (TEXT("TOUCH:TouchPanelpGetPriority - Failed to get %s value, defaulting to %d\r\n"),
            VALNAME_THREAD_PRIO, DEFAULT_THREAD_PRIORITY));
        *ThrdPrio = DEFAULT_THREAD_PRIORITY;
    }

    dwSize = sizeof(DWORD);
    dwStatus = RegQueryValueEx(
                    hKey,
                    VALNAME_THREAD_HIGH_PRIO,
                    0,
                    &dwType,
                    (PUCHAR)ThrdHighPrio,
                    &dwSize
                    );
                    
    if (dwStatus)
    {
        DEBUGMSG(ZONE_INIT | ZONE_WARN,
            (TEXT("TOUCH:TouchPanelpGetPriority - Failed to get %s value, defaulting to %d\r\n"),
            VALNAME_THREAD_HIGH_PRIO, DEFAULT_THREAD_HIGH_PRIORITY));
        *ThrdHighPrio = DEFAULT_THREAD_HIGH_PRIORITY;
    }

    RegCloseKey(hKey);

    RETAILMSG(1, (TEXT("TOUCH:ThrdPrio = %d, ThrdHighPrio = %d\n"), *ThrdPrio, *ThrdHighPrio));
    
    return;
}   // TouchPanelpGetPriority


//**********************************************************************
// The following routines provide the exported DDI interface.
// @doc EX_TOUCH_DDI EXTERNAL DRIVERS MDD TOUCH_PANEL
//**********************************************************************



#ifdef DEBUG
PFN_TOUCH_PANEL_SET_CALIBRATION v_pfnSetCalibration = TouchPanelSetCalibration;
#endif


/*++

 @func BOOL | TouchPanelGetDeviceCaps |
 Queries capabilities about the physical touch panel device.

 @parm ULONG | iIndex |
 Specifies the capability to query. They are one of the following:

 @flag DDI_TPDC_SAMPLERATE |
 The sample rate.
 @flag DDI_TPDC_CALIBRATIONPOINTS |
 The X and Y coordinates used for calibration.
 @flag DDI_TPDC_CALIBRATIONDATA |
 The X and Y coordinates used for calibration mapping.
 @flag DDI_TPDC_SAMPLERATE |
 Size of the digitizer panel (X, Y).

 @parm LPVOID | lpOutput |
 Points to the memory location(s) where the queried information
 will be placed. The format of the memory referenced depends on
 the setting of iIndex. If 0, returns the number of words
 required for the output data.

 @rdesc
 The return value is TRUE if the information was retrieved succesfully,
 and FALSE otherwise.
 

--*/
BOOL
TouchPanelGetDeviceCaps(
    INT     iIndex,
    LPVOID  lpOutput
    )
{
    struct TPDC_CALIBRATION_POINT *pTCP;
    BOOL fGotCaps = FALSE;
    

    EnterCriticalSection( &csMutex );

    if( lpOutput != NULL)
    {
        fGotCaps = DdsiTouchPanelGetDeviceCaps(iIndex, lpOutput);

         // We want to remember the screen size for later use.  Some day,
         // we might change this so that screen size is passed in as
         // part of setup.  But for now, it is part of getCalibrationPoint
        if( iIndex == TPDC_CALIBRATION_POINT_ID )
        {
            pTCP = (struct TPDC_CALIBRATION_POINT *)lpOutput;
            DisplayWidth  = pTCP -> cDisplayWidth;
            DisplayHeight = pTCP -> cDisplayHeight;
			RETAILMSG(0, (TEXT("TouchPanelGetDeviceCaps :%d,%d\r\n"), DisplayWidth, DisplayHeight));
        }
    }
    
    LeaveCriticalSection( &csMutex );

    return ( fGotCaps );
}
                
#ifdef DEBUG
PFN_TOUCH_PANEL_GET_DEVICE_CAPS v_pfnGetDeviceCaps = TouchPanelGetDeviceCaps;
#endif



/*++

Autodoc Information:

 @func BOOL | TouchPanelSetMode |
 Sets information about the abstract touch panel device.

 @parm ULONG | iIndex |
 Specifies the mode to set. They are one of the following:

 @flag DDI_TPSM_SAMPLERATE_HIGH |
 Set the sample rate to the high rate.

 @flag DDI_TPSM_SAMPLERATE_LOW |
 Set the sample rate to the low rate.

 @parm LPVOID | lpInput |
 Points to the memory location(s) where the update information
 resides. The format of the memory referenced depends on the
 mode.
 
 @rdesc
 If the function succeeds the return value is TRUE, otherwise, it is FALSE.
 Extended error information is available via the GetLastError function.

--*/
BOOL
TouchPanelSetMode(
    INT     iIndex,
    LPVOID  lpInput
    )
{
    BOOL    ReturnValue = TRUE;  // Assume it worked until someone says othewise

    EnterCriticalSection( &csMutex );
    switch( iIndex )
    {
        // The thread priority functions were provided so that OOM could
        // raise our priority as needed.
        case TPSM_PRIORITY_HIGH_ID:
            // Set the flag so that no more points are send to transcriber while system in OOM state.
            _bTchThreadHighPriority = TRUE;
            CeSetThreadPriority (hThread, gThreadHighPriority);
            break;
            
        case TPSM_PRIORITY_NORMAL_ID:
            CeSetThreadPriority (hThread, gThreadPriority);
            // We are no longer in OOM state.
            _bTchThreadHighPriority = FALSE;
            break;
            
        default:
            // If we can't handle it, give the PDD a chance
            ReturnValue = DdsiTouchPanelSetMode(iIndex, lpInput);
            break;
    }
    LeaveCriticalSection( &csMutex );
    
    return ( ReturnValue );
}


#ifdef DEBUG
PFN_TOUCH_PANEL_SET_MODE v_pfnSetMode = TouchPanelSetMode;
#endif



/*++

 @func VOID | TouchPanelPowerHandler |
 System power state notification.

 @parm BOOL | bOff | TRUE, the system is powering off; FALSE, the system is powering up.

 @comm
 This routine is called in a kernel context and may not make any system 
 calls whatsoever.  It may read and write its own memory and that's about 
 it.

--*/
void
TouchPanelPowerHandler(
    BOOL    bOff
    )
{
    DdsiTouchPanelPowerHandler( bOff );
    return;
}

#ifdef DEBUG
PFN_TOUCH_PANEL_POWER_HANDLER v_pfnPowerHandler = TouchPanelPowerHandler;
#endif


/*++

 @func BOOL | TouchPanelEnable |
 Powers up and initializes the touch panel for operation.

 @rdesc
 If the function succeeds, the return value is TRUE; otherwise, it is FALSE.

 @comm
 Following the call to this function the touch panel device generates
 pen tip events and samples.

--*/
BOOL
TouchPanelEnable(
    PFN_TOUCH_PANEL_CALLBACK    pfnCallback
    )
{
    BOOL    ReturnValue;

     //
     // Do the 'attach' code.  Normally, this would have been
     // done in the ThreadAttach block, but this driver is set
     // up to be statically linked to GWE, in which case none of
     // the DLL related calls would even be invoked.
     //
    TouchPanelpAttach();

    EnterCriticalSection( &csMutex );

    //
    // Insure the device is disabled and no one is attached to the logical
    // interrupt.
    // Power on the device.
    // Connect the logical interrupt to the device.
    //

    InterruptDone( gIntrTouch );
    InterruptDisable( gIntrTouch );
    if( SYSINTR_NOP != gIntrTouchChanged ) {
        InterruptDone( gIntrTouchChanged );
        InterruptDisable( gIntrTouchChanged );
    }

    v_pfnCgrPointCallback = pfnCallback;
    if (v_pfnCgrCallback != NULL)
    v_pfnPointCallback = v_pfnCgrCallback;
    else
        v_pfnPointCallback = pfnCallback;

    ghevCalibrationActivity = NULL;

    ReturnValue = DdsiTouchPanelEnable();

    if (ReturnValue && !InterruptInitialize(gIntrTouch, hTouchPanelEvent, NULL, 0)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("TouchPanelEnable: InterruptInitialize(gIntrTouch %d failed\r\n"),
                              gIntrTouch));
        DdsiTouchPanelDisable();
        ReturnValue = FALSE;
    }
    if ( ( SYSINTR_NOP != gIntrTouchChanged ) &&
        ReturnValue && !InterruptInitialize( gIntrTouchChanged, hTouchPanelEvent, NULL, 0)) {
        DEBUGMSG(ZONE_ERROR, (TEXT("TouchPanelEnable: InterruptInitialize(gIntrTouchChanged %d failed\r\n"),
                              gIntrTouchChanged));
        InterruptDisable(gIntrTouch);
        DdsiTouchPanelDisable();
        ReturnValue = FALSE;
    }
    if (ReturnValue) {
        // Create the ISR thread.  If creation fails, perform cleanup and return failure.
        //
        bTerminate=FALSE;
        if (!(hThread = CreateThread( NULL, 0, TouchPanelpISR, 0, 0, NULL))) {
            TouchPanelpDetach();
            InterruptDisable(gIntrTouch);
            if( SYSINTR_NOP != gIntrTouchChanged )
                InterruptDisable(gIntrTouchChanged);
            DdsiTouchPanelDisable();
            ReturnValue = FALSE;
        } else {
            // Get thread priority from registry
            TouchPanelpGetPriority(&gThreadPriority, &gThreadHighPriority);
            
            // Set our interrupt thread's priority
            CeSetThreadPriority(hThread, gThreadPriority);
        }
    }
    LeaveCriticalSection(&csMutex);
    return(ReturnValue);
}
#ifdef DEBUG
PFN_TOUCH_PANEL_ENABLE v_pfnEnableTest = TouchPanelEnable;
#endif





/*++

 @func BOOL | TouchPanelDisable |
 Powers down the touch panel. Following the call to this function
 the touch panel device no longer generates pen tip events or samples.

 @rdesc
 If the function succeeds, the return value is TRUE; otherwise, it is FALSE.

 @comm
 Following the call to this function the touch panel device no longer
 generates pen tip events or samples.


--*/
VOID
TouchPanelDisable(
    VOID
    )
{
    if ( hThread ) {
        DWORD dwReturn=WAIT_TIMEOUT;
        int count;
        bTerminate=TRUE;
        for (count=0;count<4;count++) {
            SetEvent(hTouchPanelEvent);
            if ((dwReturn=WaitForSingleObject(hThread,100))==WAIT_OBJECT_0)
                break;
        };
        if (dwReturn!=WAIT_OBJECT_0) { // Can not kill normally. then force it 
            DEBUGCHK(FALSE);
        }
        CloseHandle(hThread);
        hThread=NULL;
    }
    EnterCriticalSection( &csMutex );

    DdsiTouchPanelDisable();                // power off the panel.
    InterruptDone( gIntrTouch );         // Unregister the logical interrupt.
    InterruptDisable( gIntrTouch );
    if( SYSINTR_NOP != gIntrTouchChanged ) {
        InterruptDone( gIntrTouchChanged ); // Unregister the logical interrupt.
        InterruptDisable( gIntrTouchChanged );
    }
    hThread = NULL;
    ghevCalibrationActivity = NULL;


    if ( hTouchPanelEvent )
    {
        //
        // We created the touch panel event:
        //  close the event handle
        //  reset our bookkeeping information
        //

        CloseHandle( hTouchPanelEvent );
        hTouchPanelEvent = NULL;
    }

    if ( hCalibrationSampleAvailable )
    {
        //
        // We created the calibration sample available event:
        //  close the event handle
        //  reset our bookkeeping information
        //

        CloseHandle( hCalibrationSampleAvailable );
        hCalibrationSampleAvailable = NULL;
    }

    LeaveCriticalSection( &csMutex );

    DeleteCriticalSection( &csMutex );
}

#ifdef DEBUG
PFN_TOUCH_PANEL_DISABLE v_pfnDisableTest = TouchPanelDisable;
#endif


/*++

 @func VOID | TouchPanelReadCalibrationPoint |
 Initates the process of getting a calibration point.  This function
 causes the device driver to foward the last valid x and y coordinates
 between the tip states of initial down and up to the calibration callback
 function. The tip state of the forwarded coordinates is reported as
 initial down.

 @parm LONG | UcalX |
 The uncalibrated X coordinate under calibration.
 @parm LONG | UcalY |
 The uncalibrated Y coordinate under calibration.

 @rdesc
 If the function succeeds, TRUE; otherwise FALSE. Extended error information
 is available via the GetLastError function.


--*/
BOOL
TouchPanelReadCalibrationPoint(
    INT *pRawX,
    INT *pRawY
    )
{
    BOOL    retval;
    HANDLE  hevActivity = NULL;
    DWORD   dwStatus;
    HKEY    hk;


    if(!pRawX  || !pRawY ) {
        SetLastError(ERROR_INVALID_PARAMETER);        
        return ( FALSE ) ;
    }

    // Get a path to the GWES activity event.  We need to set it ourselves
    // because calibration is essentially a modal loop and doesn't put
    // events onto the user input queue, which is where this event gets
    // set inside GWES.
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, L"System\\GWE", 0, 0, &hk);
    if(dwStatus == ERROR_SUCCESS) {
        WCHAR szEventPath[MAX_PATH];
        DWORD dwSize = sizeof(szEventPath);
        DWORD dwType;

        // Read the path to the event and open it -- do this on every call
        // to this API so that we're not forced to keep an event open that
        // is almost never used.
        dwStatus = RegQueryValueEx(hk, L"ActivityEvent", NULL, &dwType, (LPBYTE) szEventPath, &dwSize);
        szEventPath[MAX_PATH - 1] = 0;      // enforce null termination
        if(dwStatus == ERROR_SUCCESS && dwType == REG_SZ) {
            hevActivity = OpenEvent(EVENT_ALL_ACCESS, FALSE, szEventPath);
        }
        RegCloseKey(hk);
    }
    
    EnterCriticalSection( &csMutex );

    //
    // If a calibration is already active, error.
    //

    if ( CalibrationState )
    {
        SetLastError( ERROR_POSSIBLE_DEADLOCK );
        LeaveCriticalSection( &csMutex );
        if(hevActivity != NULL) {
            CloseHandle(hevActivity);
        }
        return ( FALSE );
    }

    //
    // Set sample count and active flag.
    // Wait for calibration to happen.
    // Update the memory with the x and y coordinates.
    // Clear active flag.
    // We be done.
    CalibrationState = CalibrationWaiting;

    // let the IST know about the event
    ghevCalibrationActivity = hevActivity;

    LeaveCriticalSection( &csMutex );

    WaitForSingleObject( hCalibrationSampleAvailable, INFINITE );
    EnterCriticalSection( &csMutex );

    *pRawX = lCalibrationXCoord;
    *pRawY = lCalibrationYCoord;

    retval = ( CalibrationState == CalibrationValid );
    CalibrationState = CalibrationInactive;

    // done with the event
    ghevCalibrationActivity = NULL;

    LeaveCriticalSection( &csMutex );

    // close the event handle
    CloseHandle(hevActivity);

    return retval;
}
#ifdef DEBUG
PFN_TOUCH_PANEL_READ_CALIBRATION_POINT v_pfnReadCalibrationPointTest = TouchPanelReadCalibrationPoint;
#endif

/*++

 @func VOID | TouchPanelReadCalibrationAbort |
 Aborts the currently active <f TouchPanelCalibratePoint>.

 @rdesc
 If the function succeeds, TRUE; FALSE is returned if there is no active
 <f TouchPanelCalibrateAPoint> in progress.

--*/
VOID
TouchPanelReadCalibrationAbort(
    VOID
    )
{
    EnterCriticalSection( &csMutex );

    if ( ( CalibrationState == CalibrationValid ) ||
         ( CalibrationState == CalibrationInactive ) )
    {
        LeaveCriticalSection( &csMutex );
        return;
    }

    CalibrationState = CalibrationAborted;

    SetEvent( hCalibrationSampleAvailable );

    LeaveCriticalSection( &csMutex );

    return;
}
#ifdef DEBUG
PFN_TOUCH_PANEL_READ_CALIBRATION_ABORT v_pfnCalibrationPointAbortTest = TouchPanelReadCalibrationAbort;
#endif


