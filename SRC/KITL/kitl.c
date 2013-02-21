//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//
// -----------------------------------------------------------------------------
#include <windows.h>
#include <bsp.h>
#include <kitl_cfg.h>
#include <devload.h>
#include <usbdbgser.h>
#include <usbdbgrndis.h>

static volatile S3C6410_GPIO_REG *g_pGPIOReg;
static void InitSROMC_DM9000(void);

void BSPKitlEthPowerOff(void);
void BSPKitlEthPowerOn(void);


//------------------------------------------------------------------------------
//
// Platform entry point for KITL. Called when KITLIoctl (IOCTL_KITL_STARTUP, ...) is called.
//

BOOL OEMKitlStartup(void)
{
    OAL_KITL_ARGS KITLArgs;
    OAL_KITL_ARGS *pKITLArgs;
    BOOL bRet = FALSE;
    UCHAR *szDeviceId,buffer[OAL_KITL_ID_SIZE]="\0";

    OALMSG(OAL_KITL&&OAL_FUNC, (L"[KITL] ++OEMKitlStartup()\r\n"));

    // Look for bootargs left by the bootloader or left over from an earlier boot.
    //
    pKITLArgs = (OAL_KITL_ARGS *)OALArgsQuery(OAL_ARGS_QUERY_KITL);
    szDeviceId = (UCHAR*)OALArgsQuery(OAL_ARGS_QUERY_DEVID);

    // If no KITL arguments were found (typically provided by the bootloader), then select
    // some default settings.
    //
    if (pKITLArgs == NULL)
    {
        memset(&KITLArgs, 0, sizeof(OAL_KITL_ARGS));
        
        // By default, enable KITL and use USB Serial
        KITLArgs.flags |= OAL_KITL_FLAGS_ENABLED;

        KITLArgs.devLoc.IfcType     = Internal;
        KITLArgs.devLoc.BusNumber   = 0;
        KITLArgs.devLoc.PhysicalLoc = (PVOID)S3C6410_BASE_REG_PA_USBOTG_LINK;
        KITLArgs.devLoc.LogicalLoc  = (DWORD)KITLArgs.devLoc.PhysicalLoc;

        pKITLArgs = &KITLArgs;
    }

    if (pKITLArgs->devLoc.LogicalLoc == BSP_BASE_REG_PA_DM9000A_IOBASE)
    {
        // Ethernet specific initialization

        //configure chipselect for cs8900a
        InitSROMC_DM9000();

        //setting EINT10 as IRQ_LAN
        if (!(pKITLArgs->flags & OAL_KITL_FLAGS_POLL))
        {
            g_pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
        }

        // Setup pointers to the power on and power off functions to enable KITL
        // functionality across suspend/resume
        
        // Modify the g_kitlEthCS8900A structure defined in kitl_cfg.h
        g_kitlEthDM9000A.pfnPowerOff = (OAL_KITLETH_POWER_OFF)BSPKitlEthPowerOff;
        g_kitlEthDM9000A.pfnPowerOn = (OAL_KITLETH_POWER_ON)BSPKitlEthPowerOn;
    }
    
    bRet = OALKitlInit ((LPCSTR)szDeviceId, pKITLArgs, g_kitlDevices);

    OALMSG(OAL_KITL&&OAL_FUNC, (L"[KITL] --OEMKitlStartup() = %d\r\n", bRet));

    return bRet;
}


DWORD OEMKitlGetSecs (void)
{
    SYSTEMTIME st;
    DWORD dwRet;
    static DWORD dwBias;
    static DWORD dwLastTime;

    OEMGetRealTime( &st );
    dwRet = ((60UL * (60UL * (24UL * (31UL * st.wMonth + st.wDay) + st.wHour) + st.wMinute)) + st.wSecond);
    dwBias = dwRet;

    if (dwRet < dwLastTime)
    {
        KITLOutputDebugString("[KITL] Time went backwards (or wrapped): cur: %u, last %u\n", dwRet,dwLastTime);
    }

    dwLastTime = dwRet;

    return (dwRet);
}

//------------------------------------------------------------------------------
//
//  Function:  OALGetTickCount
//
//  This function is called by some KITL libraries to obtain relative time
//  since device boot. It is mostly used to implement timeout in network
//  protocol.
//
UINT32 OALGetTickCount()
{
    return OEMKitlGetSecs () * 1000;
}

//------------------------------------------------------------------------------
//
//  Function:  OEMKitlIoctl
//
//  This function is called by some KITL libraries to process platform specific
//  KITL IoCtl calls.
//
BOOL OEMKitlIoctl (DWORD code, VOID * pInBuffer, DWORD inSize, VOID * pOutBuffer, DWORD outSize, DWORD * pOutSize)
{
    BOOL fRet = FALSE;

    switch (code) {
        case IOCTL_HAL_INITREGISTRY:
            OALKitlInitRegistry();
            break;
        default:
            fRet = OALIoCtlVBridge (code, pInBuffer, inSize, pOutBuffer, outSize, pOutSize);
    }
    return fRet;
}


//------------------------------------------------------------------------------
//
//  Function:  OALKitlInitRegistry
//
//  This function is called during the initialization process to allow the
//  OAL to denote devices which are being used by the KITL connection
//  and thus shouldn't be touched during the OS initialization process.  The
//  OAL provides this information via the registry.
//

VOID OALKitlInitRegistry()
{
    HKEY Key;
    DWORD Status;
    DWORD Disposition;
    DWORD Value;
    DWORD Flags;
    DEVICE_LOCATION devLoc;

    // Get KITL device location
    if (!OALKitlGetDevLoc(&devLoc))
        goto CleanUp;

    if (devLoc.LogicalLoc == S3C6410_BASE_REG_PA_USBOTG_LINK)
    {
        // Disable the UsbFn driver since it is used for KITL
        //

        Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\SC6410USBFN", 0, NULL, 0, 0, NULL, &Key, &Disposition);

        if (Status == ERROR_SUCCESS)
        {
            Disposition = DEVFLAGS_NOLOAD;
            // Set Flags value to indicate no loading of driver for this device
            Status = NKRegSetValueEx(Key, DEVLOAD_FLAGS_VALNAME, 0, DEVLOAD_FLAGS_VALTYPE, (PBYTE)&Disposition, sizeof(Disposition));
        }

        // Close the registry key.
        NKRegCloseKey(Key);

        if (Status != ERROR_SUCCESS)
        {
            KITL_RETAILMSG(ZONE_INIT, ("OALKitlInitRegistry: failed to set \"no load\" key for Usbfn driver.\r\n"));
            goto CleanUp;
        }

        KITL_RETAILMSG(ZONE_INIT, ("INFO: USB being used for KITL - disabling Usbfn driver...\r\n"));
    }

    if (devLoc.LogicalLoc == BSP_BASE_REG_PA_DM9000A_IOBASE)
    {
        // If KITL is over Ethernet and interrupt is enabled, let the 
        // PowerButton Driver know, so that it can disable the Reset Button 
        // (The Reset button interrupt clashes with the Ethernet KITL interrupt)

        // Get the KITL flags
        if (!OALKitlGetFlags(&Flags))
            goto CleanUp;

        if (!(Flags & OAL_KITL_FLAGS_POLL))
        {
			/*
            Status = NKRegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\PowerButton", 0, NULL, 0, 0, NULL, &Key, &Disposition);

            if (Status == ERROR_SUCCESS)
            {
                Value = 1;
                Status = NKRegSetValueEx(Key, L"EthernetKITLInterruptEnabled", 0, REG_DWORD, (PBYTE)&Value, sizeof(Value));
            }

            // Close the registry key.
            NKRegCloseKey(Key);

            if (Status != ERROR_SUCCESS)
            {
                KITL_RETAILMSG(ZONE_INIT, ("OALKitlInitRegistry: failed to set key to disable reset button.\r\n"));
                goto CleanUp;
            }
			*/
            KITL_RETAILMSG(ZONE_INIT, ("INFO: Ethernet KITL Interrupt enabled - disabling reset button in button driver.\r\n"));

        }
    }

CleanUp:
    return;
}


// for SMDK6410
#define CS8900_Tacs (0x0)   // 0clk
#define CS8900_Tcos (0x4)   // 4clk
#define CS8900_Tacc (0xd)   // 14clk
#define CS8900_Tcoh (0x1)   // 1clk
#define CS8900_Tah  (0x4)   // 4clk
#define CS8900_Tacp (0x6)   // 6clk
#define CS8900_PMC  (0x0)   // normal(1data)
// TenByTen6410
#define DM9000A_Tacs    (0x0)    // 0clk
#define DM9000A_Tcos    (0x0)    // 4clk
#define DM9000A_Tacc    (0x7)    // 14clk
#define DM9000A_Tcoh    (0x0)    // 1clk
#define DM9000A_Tah     (0x0)    // 4clk
#define DM9000A_Tacp    (0x0)    // 6clk
#define DM9000A_PMC     (0x0)    // normal(1data)

static void InitSROMC_DM9000(void)
{
    volatile S3C6410_SROMCON_REG *s6410SROM = (S3C6410_SROMCON_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_SROMCON, FALSE);

    s6410SROM->SROM_BW = (s6410SROM->SROM_BW & ~(0xF<<4)) |
                            (1<<7)| // nWBE/nBE(for UB/LB) control for Memory Bank1(0=Not using UB/LB, 1=Using UB/LB)
                            (1<<6)| // Wait enable control for Memory Bank1 (0=WAIT disable, 1=WAIT enable)
                            (1<<4); // Data bus width control for Memory Bank1 (0=8-bit, 1=16-bit)

    s6410SROM->SROM_BC1 = ((DM9000A_Tacs<<28)+(DM9000A_Tcos<<24)+(DM9000A_Tacc<<16)+(DM9000A_Tcoh<<12)  \
                            +(DM9000A_Tah<<8)+(DM9000A_Tacp<<4)+(DM9000A_PMC));
}


void BSPKitlEthPowerOn(void)
{
    OAL_KITL_ARGS *pKITLArgs;
    PBYTE  pBaseIOAddress = NULL;
    UINT32 MemoryBase = 0;   

    KITL_RETAILMSG(ZONE_INIT, ("BSPKitlEthPowerOn: Powering On Ethernet KITL.\r\n"));

    pKITLArgs = (OAL_KITL_ARGS *)OALArgsQuery(OAL_ARGS_QUERY_KITL);

    if (pKITLArgs == NULL)
    {
        // This should do not happen, since this function is part of the Eth KITL driver structure
        // But still, just in case...
        KITL_RETAILMSG(ZONE_ERROR, ("BSPKitlEthPowerOn: No KITL args, returning.\r\n"));
        return;
    }

    //configure chipselect for DM9000
    InitSROMC_DM9000();

    pBaseIOAddress   = (PBYTE)OALPAtoVA(pKITLArgs->devLoc.LogicalLoc, FALSE);
    MemoryBase       = (UINT32)OALPAtoVA(BSP_BASE_REG_PA_DM9000A_MEMBASE, FALSE);

    // Initialize the controller.
    if (!DM9000AInit((PBYTE)pBaseIOAddress, MemoryBase, pKITLArgs->mac))
    {
        KITL_RETAILMSG(ZONE_INIT, ("BSPKitlEthPowerOn: ERROR: Failed to re-initialize Ethernet controller.\r\n"));
    }
}


void BSPKitlEthPowerOff(void)
{
    // Nothing to do here.
    KITL_RETAILMSG(ZONE_INIT, ("\r\nBSPKitlEthPowerOff: Ethernet KITL Powered Off\r\n"));
}


