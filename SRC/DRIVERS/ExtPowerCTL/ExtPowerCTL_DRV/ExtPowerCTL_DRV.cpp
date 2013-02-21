//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2012. Hiteg Ltd  All rights reserved.

Module Name:  

Abstract:

    WinCE driver to control external power modules found on the TenByTen6410

rev:
	2012.03.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 
	.dll has to be added in platform.bib
		EPCTL.dll  $(_FLATRELEASEDIR)\EPCTL.dll            NK  SHK

	EPCTL.REG file should be included into final .reg file ( this should happen automatically ...(M$)... please check)
--*/

#include <bsp.h>
#include <windows.h>
#include <s3c6410.h>
#include <ceddk.h>
#include "bsp_cfg.h"
#include "bsp_args.h"

#include "ExtPowerCTL_DRV.h"

extern "C" {
#include "ExtPowerCTL_LIB.h"
}

static DWORD power;
volatile S3C6410_GPIO_REG *v_pIOP_PIO_regs = NULL;

#define EPCTL_DBG_MSG	  0
#define EPCTL_ERR_MSG		0

/**
    @func   void | retrievs and sets the GPIO base.
    @comm
    @xref
**/
void  InitGPIOBaseAddr()
{
	//DWORD dwIOBase;
	DWORD dwIOSize;
	PHYSICAL_ADDRESS  IOPhyAdr= { S3C6410_BASE_REG_PA_GPIO, 0 };
	// RETAILMSG(PIO_DBG_MSG, (TEXT("++InitGPIOAddr\r\n")));
	dwIOSize = sizeof(S3C6410_GPIO_REG);
	if (NULL == v_pIOP_PIO_regs)
	{
		v_pIOP_PIO_regs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(IOPhyAdr, (ULONG)dwIOSize, FALSE);	
		if ( NULL == v_pIOP_PIO_regs)
		{
			RETAILMSG(EPCTL_ERR_MSG, (TEXT("GPIO init address failed PIO\r\n")));
		}
	}
}

/**
    @func   BOOL | standard DLLEntry 
    @comm
    @xref
**/
BOOL WINAPI DllEntry(HANDLE	hinstDLL, DWORD dwReason, LPVOID /* lpvReserved */)
{
	switch(dwReason)
	{
	case DLL_PROCESS_ATTACH:
		DEBUGREGISTER((HINSTANCE)hinstDLL);
		return TRUE;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
#ifdef UNDER_CE
	case DLL_PROCESS_EXITING:
		break;
	case DLL_SYSTEM_STARTED:
		break;
#endif
	}

	return TRUE;
}

/**
    @func   BOOL | DLL's main function
    @comm
    @xref
**/
BOOL WINAPI DllMain(HANDLE hinstDll, ULONG Reason, LPVOID Reserved)
{
	switch(Reason)
	{
		case DLL_PROCESS_ATTACH:
		RETAILMSG(EPCTL_DBG_MSG,(TEXT("S3C6410 EPCTL: DLL_PROCESS_ATTCH\r\n")));
			break;
			
		case DLL_PROCESS_DETACH:
		RETAILMSG(EPCTL_DBG_MSG,(TEXT("S3C6410 EPCTL: DLL_PROCESS_DETACH\r\n")));
			break;
			
		default:
			
			break;
	}
	
	return TRUE;
}
/**
    @func   BOOL | deinitializes the driver.
    @comm
    @xref
**/
BOOL EPCTL_Deinit(DWORD hDeviceContext)
{
	RETAILMSG(1,(TEXT("EPCTL: EPCTL_Deinit\r\n")));
	return TRUE;
} 
/**
    @func   DWORD | initializes the GPIO registers.
    @comm
    @xref
**/
DWORD EPCTL_Init(DWORD dwContext, 
  LPCVOID lpvBusContext)
{
	DWORD powerCTL;
	DWORD dwBytesRet = 0;
	RETAILMSG(1,(TEXT("Initializing EPCTL driver...\r\n")));
	if (KernelIoControl(IOCTL_HAL_POWERCTL, NULL, 0, &powerCTL, sizeof(powerCTL), &dwBytesRet) // get data from BSP_ARGS via KernelIOCtl
                        && (dwBytesRet == sizeof(powerCTL)))
	{
		RETAILMSG(1,(TEXT("--------------ExtPowerCTL driver read: %d\r\n"),powerCTL));
	}
	else
	{
		RETAILMSG(1,(TEXT("Error getting ExtPowerCTL data from args section via Kernel IOCTL!!!\r\n")));
	}
	power=(powerCTL & 0xff);							// get values from eboot
	InitGPIOBaseAddr();								// get the base address for register access
	PWRCTL_powerCTLInit(v_pIOP_PIO_regs);			// initialize all GPIOs used 
	PWRCTL_setAllTo(&power);						// and set to given states
	RETAILMSG(EPCTL_DBG_MSG, (TEXT("EPCTL_Init---\r\n")));
	return TRUE;
}
// IMPORT the ioctl codes

#include "extpwrctl.h"
/**
    @func   BOOL | IOControll dispatch.
    @comm
    @xref
**/	
BOOL EPCTL_IOControl(	DWORD dwOpenContext, 
						DWORD dwIoControlCode, 
						LPBYTE lpInBuf, 
						DWORD nInBufSize, 
						LPBYTE lpOutBuf, 
						DWORD nOutBufSize, 
						LPDWORD lpBytesReturned)
{

	RETAILMSG(EPCTL_DBG_MSG,(TEXT("EPCTL_IOControl+++\r\n")));
	RETAILMSG(EPCTL_DBG_MSG,(TEXT("dwIoControlCode = %d.\r\n"), dwIoControlCode));

	PWRCTL_powerCTLInit(v_pIOP_PIO_regs); // again, make sure everything is setup as we need it

	switch(dwIoControlCode)
	{
	case IOCTL_LCD_ON_SET:
		PWRCTL_setPower(&power, LCD);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_USB_ON_SET:
		PWRCTL_setPower(&power, USB);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_CAMERA_ON_SET:
		PWRCTL_setPower(&power, CAM);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_DAC0_ON_SET:
		PWRCTL_setPower(&power, DAC0);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_DAC1_ON_SET:
		PWRCTL_setPower(&power, DAC1);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_ETH_ON_SET:
		PWRCTL_setPower(&power, ETH);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_WIFI_ON_SET:
		PWRCTL_setPower(&power, WIFI);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_LCD_OFF_SET:
		PWRCTL_clrPower(&power, LCD);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_USB_OFF_SET:
		PWRCTL_clrPower(&power, USB);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_CAMERA_OFF_SET:
		PWRCTL_clrPower(&power, CAM);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_DAC0_OFF_SET:
		PWRCTL_clrPower(&power, DAC0);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_DAC1_OFF_SET:
		PWRCTL_clrPower(&power, DAC1);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_ETH_OFF_SET:
		PWRCTL_clrPower(&power, ETH);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_WIFI_OFF_SET:
		PWRCTL_clrPower(&power, WIFI);
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_ALL_OFF_SET:
		power=0x0;
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_SLEEP_SET:							// powers all modules down, w\o changing the current setting
		{
			DWORD null_power=0x0;
			PWRCTL_setAllTo(&null_power);
		break;
		}
	case IOCTL_ALL_ON_SET:
		power=0xFF;
		PWRCTL_setAllTo(&power);
		break;
	case IOCTL_LCD_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, LCD);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_USB_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, USB);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_CAMERA_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, CAM);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_DAC0_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, DAC0);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_DAC1_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, DAC1);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_ETH_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, ETH);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_WIFI_GET:
		*((DWORD *)lpOutBuf)=PWRCTL_getStatus(&power, WIFI);
		*((DWORD *)lpBytesReturned)=sizeof(DWORD);
		break;
	case IOCTL_ALL_GET:
		*((DWORD *)lpOutBuf)=power;
		*((DWORD *)lpBytesReturned)=sizeof(power);
		break;
	case IOCTL_AWAKE_SET:							// sets all modules to configured state
	default:
		PWRCTL_setAllTo(&power);
		break;
	};
 	return TRUE;
} 
/**
    @func   DWORD | "Opens" our device.
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD EPCTL_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode)
{
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_Open\r\n")));
	return TRUE;
} 

/**
    @func   BOOL | "closes" our device. We deinit on this event
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL EPCTL_Close(DWORD hOpenContext)
{
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_Close\r\n")));
	PWRCTL_deinit();
	return TRUE;
} 

/**
    @func   void | We use this event to go to sleep by setting all modules
				   to 0, but without changing the current configuration
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void EPCTL_PowerDown(DWORD hDeviceContext)
{
	/**
	* set all temporary to sleep, the current configuration is not changed, thus awaking is easy
	**/
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_PowerDown\r\n")));
	DWORD null_power=0x0;
	PWRCTL_setAllTo(&null_power);
	} 
/**
    @func   void | We use this event to "awake" and set all modules to current configuration
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void EPCTL_PowerUp(DWORD hDeviceContext)
{
	/**
	* sets all modules to configured state.
	**/
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_PowerUp\r\n")));
	PWRCTL_setAllTo(&power);	
} 
/**
    @func   DWORD | give current config as byte
			Count has to be at least 1 byte
    @comm
    @xref
**/

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD EPCTL_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_Read\r\n")));
	if(Count>=1)
	{	*((unsigned char *)pBuffer)=(unsigned char)(power & 0xFF);
		return TRUE;
	}
	return FALSE;
} 
/**
    @func   DWORD | As we handle one byte only, seek is not really useful
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD EPCTL_Seek(DWORD hOpenContext, long Amount, DWORD Type)
{
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_Seek\r\n")));
	return 0;
} 
/**
    @func   DWORD | You can overwrite the current configuration.
    @comm
    @xref
**/
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD EPCTL_Write(DWORD hOpenContext, LPCVOID pSourceBytes, DWORD NumberOfBytes)
{
	RETAILMSG(1,(TEXT("EPCTL : EPCTL_Write\r\n")));
	if(NumberOfBytes==0) return 0; // amount of bytes written ????
	power=*((unsigned char *)pSourceBytes);
	PWRCTL_setAllTo(&power); // apply immediately
	return 1; // amount of bytes written ????
}