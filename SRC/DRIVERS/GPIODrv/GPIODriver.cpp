//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// 
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    GPIODrv

Abstract:       Device Driver to controll GPIO PIN 22 to PIN 37 on TenByTen6410 GPIO connector

Functions:
				

Notes:

		Verion	1.0			2013		sven.riemann@hiteg.com

--*/
#include <windows.h>
#include <ceddk.h>
#include <nkintr.h>
#include <pm.h>
#include <s3c6410.h>
//#include "BSP.h"

#include "GPIODriver.h"

#ifdef DEBUG
#define ZONE_GPIOCONTROL	DEBUGZONE(0)
#define ZONE_FUNCTION		DEBUGZONE(1)
#define ZONE_ERROR			DEBUGZONE(15)
#else
#define ZONE_GPIOCONTROL	1
#define ZONE_FUNCTION		1
#define ZONE_ERROR			1
#endif

#if 0
#define DEBUG_GPIO			1
#else
#define DEBUG_GPIO			0
#endif

static int active_port	=	0;
static BOOL bulk_active =   FALSE;
static unsigned char portIsOutput[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

volatile static S3C6410_GPIO_REG *v_pGPIORegs=NULL;

BOOL mInitialized;
BOOL InitializeAddresses(VOID);						// Virtual allocation


BOOL InitializeAddresses(VOID)
{
	BOOL	RetValue = TRUE;
	PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};

	ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_GPIO;
	v_pGPIORegs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_GPIO_REG), FALSE);
	if (v_pGPIORegs == NULL)
	{
		RETAILMSG(ZONE_ERROR, (TEXT("[GIO] v_pGPIORegs MmMapIoSpace() Failed \r\n")));
		RetValue = FALSE;
	}
	return(RetValue);

}


BOOL WINAPI  
DllEntry(HANDLE	hinstDLL, DWORD dwReason, LPVOID /* lpvReserved */)
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


BOOL GIO_Deinit(DWORD hDeviceContext)
{
	BOOL bRet = TRUE;

	if(v_pGPIORegs != NULL)
	{
		MmUnmapIoSpace((PVOID)v_pGPIORegs, sizeof(S3C6410_GPIO_REG));
		v_pGPIORegs = NULL;
	}
	RETAILMSG(DEBUG_GPIO,(TEXT("GPIO_Control: GIO_Deinit\r\n")));

	return TRUE;
} 


DWORD GIO_Init(DWORD dwContext)
{

	RETAILMSG(DEBUG_GPIO, (TEXT("GPIO Initialize ...")));

	if (!InitializeAddresses())
		return (FALSE);

	mInitialized = TRUE;
	RETAILMSG(DEBUG_GPIO, (TEXT("OK !!!\r\n")));
	return TRUE;
}

BOOL setPortToOutput(DWORD port)
{
	switch(port & 0xF)
	{
		case IO_CTL_GPE4_PIN22:
			v_pGPIORegs->GPECON |= (1<<16);
			portIsOutput[IO_CTL_GPE4_PIN22]=1;
		break;
		case IO_CTL_GPK0_PIN23:
			v_pGPIORegs->GPKCON0 |= (1<<0);
			portIsOutput[IO_CTL_GPK0_PIN23]=1;
		break;
		case IO_CTL_GPK1_PIN24:
			v_pGPIORegs->GPKCON0 |= (1<<4);
			portIsOutput[IO_CTL_GPK1_PIN24]=1;
		break;
		case IO_CTL_GPK2_PIN25:
			v_pGPIORegs->GPKCON0 |= (1<<8);
			portIsOutput[IO_CTL_GPK2_PIN25]=1;
		break;
		case IO_CTL_GPK3_PIN26:
			v_pGPIORegs->GPKCON0 |= (1<<12);
			portIsOutput[IO_CTL_GPK3_PIN26]=1;
		break;
		case IO_CTL_GPK4_PIN27:
			v_pGPIORegs->GPKCON0 |= (1<<16);
			portIsOutput[IO_CTL_GPK4_PIN27]=1;
		break;
		case IO_CTL_GPK5_PIN28:
			v_pGPIORegs->GPKCON0 |= (1<<20);
			portIsOutput[IO_CTL_GPK5_PIN28]=1;
		break;
		case IO_CTL_GPL8_PIN29:
			v_pGPIORegs->GPLCON1 |= (1<<0);
			portIsOutput[IO_CTL_GPL8_PIN29]=1;
		break;
		case IO_CTL_GPL9_PIN30:
			v_pGPIORegs->GPLCON1 |= (1<<4);
			portIsOutput[IO_CTL_GPL9_PIN30]=1;
		break;
		case IO_CTL_GPL10_PIN31:
			v_pGPIORegs->GPLCON1 |= (1<<8);
			portIsOutput[IO_CTL_GPL10_PIN31]=1;
		break;
		case IO_CTL_GPL11_PIN32:
			v_pGPIORegs->GPLCON1 |= (1<<12);
			portIsOutput[IO_CTL_GPL11_PIN32]=1;
		break;

		case IO_CTL_GPP8_PIN33:
			v_pGPIORegs->GPPCON |= (1<<16);
			portIsOutput[IO_CTL_GPP8_PIN33]=1;
		break;
		case IO_CTL_GPP9_PIN34:
			v_pGPIORegs->GPPCON |= (1<<18);
			portIsOutput[IO_CTL_GPP9_PIN34]=1;
		break;
		case IO_CTL_GPP12_PIN35:
			v_pGPIORegs->GPPCON |= (1<<24);
			portIsOutput[IO_CTL_GPP12_PIN35]=1;
		break;

		case IO_CTL_GPE0_PIN36:
			v_pGPIORegs->GPECON |= (1<<0);
			portIsOutput[IO_CTL_GPE0_PIN36]=1;
		break;
		case IO_CTL_GPE1_PIN37:
			v_pGPIORegs->GPECON |= (1<<4);
			portIsOutput[IO_CTL_GPE1_PIN37]=1;
		break;

		default:
			return FALSE;
	};
	return TRUE;
}



BOOL setPortToInput(DWORD port)
{
	switch(port & 0xF)
	{
		case IO_CTL_GPE4_PIN22:
			v_pGPIORegs->GPECON &= ~(1<<16);
		break;
		case IO_CTL_GPK0_PIN23:
			v_pGPIORegs->GPKCON0 &= ~(1<<0);
		break;
		case IO_CTL_GPK1_PIN24:
			v_pGPIORegs->GPKCON0 &= ~(1<<4);
		break;
		case IO_CTL_GPK2_PIN25:
			v_pGPIORegs->GPKCON0 &= ~(1<<8);
		break;
		case IO_CTL_GPK3_PIN26:
			v_pGPIORegs->GPKCON0 &= ~(1<<12);
		break;
		case IO_CTL_GPK4_PIN27:
			v_pGPIORegs->GPKCON0 &= ~(1<<16);
		break;
		case IO_CTL_GPK5_PIN28:
			v_pGPIORegs->GPKCON0 &= ~(1<<20);
		break;
		case IO_CTL_GPL8_PIN29:
			v_pGPIORegs->GPLCON1 &= ~(1<<0);
		break;
		case IO_CTL_GPL9_PIN30:
			v_pGPIORegs->GPLCON1 &= ~(1<<4);
		break;
		case IO_CTL_GPL10_PIN31:
			v_pGPIORegs->GPLCON1 &= ~(1<<8);
		break;
		case IO_CTL_GPL11_PIN32:
			v_pGPIORegs->GPLCON1 &= ~(1<<12);
		break;

		case IO_CTL_GPP8_PIN33:
			v_pGPIORegs->GPPCON &= ~(1<<16);
		break;
		case IO_CTL_GPP9_PIN34:
			v_pGPIORegs->GPPCON &= ~(1<<18);
		break;
		case IO_CTL_GPP12_PIN35:
			v_pGPIORegs->GPPCON &= ~(1<<24);
		break;

		case IO_CTL_GPE0_PIN36:
			v_pGPIORegs->GPECON &= ~(1<<0);
		break;
		case IO_CTL_GPE1_PIN37:
			v_pGPIORegs->GPECON &= ~(1<<4);
		break;

		default:
			return FALSE;
	};
	return TRUE;
}



BOOL setPortPullUp(DWORD port, int value)
{
	switch(port & 0xF)
	{
		case IO_CTL_GPE4_PIN22:
			if(value)
				v_pGPIORegs->GPEPUD |= ((value)<<8);
			else
				v_pGPIORegs->GPEPUD &= ~(3<<8);
		break;
		case IO_CTL_GPK0_PIN23:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<0);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<0);
		break;
		case IO_CTL_GPK1_PIN24:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<2);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<2);
		break;
		case IO_CTL_GPK2_PIN25:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<4);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<4);
		break;
		case IO_CTL_GPK3_PIN26:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<6);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<6);
		break;
		case IO_CTL_GPK4_PIN27:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<8);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<8);
		break;
		case IO_CTL_GPK5_PIN28:
			if(value)
				v_pGPIORegs->GPKPUD |= (value<<10);
			else
				v_pGPIORegs->GPKPUD &= ~(3<<10);
		break;

		case IO_CTL_GPL8_PIN29:
			if(value)
				v_pGPIORegs->GPLPUD |= (value<<0);
			else
				v_pGPIORegs->GPLPUD &= ~(3<<0);
		break;
		case IO_CTL_GPL9_PIN30:
			if(value)
				v_pGPIORegs->GPLPUD |= (value<<4);
			else
				v_pGPIORegs->GPLPUD &= ~(3<<4);
		break;
		case IO_CTL_GPL10_PIN31:
			if(value)
				v_pGPIORegs->GPLCON1 &= (value<<8);
			else
				v_pGPIORegs->GPLPUD &= ~(3<<8);
		break;
		case IO_CTL_GPL11_PIN32:
			if(value)
				v_pGPIORegs->GPLPUD |= (value<<12);
			else
				v_pGPIORegs->GPLPUD &= ~(3<<12);
		break;

		case IO_CTL_GPP8_PIN33:
			if(value)
				v_pGPIORegs->GPPPUD |= (value<<16);
			else
				v_pGPIORegs->GPPPUD &= ~(3<<16);
		break;
		case IO_CTL_GPP9_PIN34:
			if(value)
				v_pGPIORegs->GPPPUD |= (value<<18);
			else
				v_pGPIORegs->GPPPUD &= ~(3<<18);
		break;
		case IO_CTL_GPP12_PIN35:
			if(value)
				v_pGPIORegs->GPPPUD |= (value<<24);
			else
				v_pGPIORegs->GPPPUD &= ~(3<<24);
		break;

		case IO_CTL_GPE0_PIN36:
			if(value)
				v_pGPIORegs->GPEPUD |= (value<<0);
			else
				v_pGPIORegs->GPEPUD &= ~(3<<0);
		break;
		case IO_CTL_GPE1_PIN37:
			if(value)
				v_pGPIORegs->GPEPUD |= (value<<4);
			else
				v_pGPIORegs->GPEPUD &= ~(3<<4);
		break;

		default:
			return FALSE;
	};
	return TRUE;
}


BOOL bulkSetPortsToOutput(DWORD ports)
{
	int n;
	int ret=0;
	for(n=0;n<16;n++)
	{
		if(ports&(1<<n))
			ret|=setPortToOutput(n);
	}

	return (ret)?FALSE:TRUE;
}

BOOL bulkSetPortsToInput(DWORD ports)
{
	int n, ret=0;
	for(n=0;n<16;n++)
	{
		if(ports&(1<<n))
			ret|=setPortToInput(n);
	}
	return (ret)?FALSE:TRUE;
}
BOOL bulkSetPortsToPullUpDown(DWORD ports, int state)
{
	int n,ret=0;
	for(n=0;n<16;n++)
	{
		if(ports&(1<<n))
			ret|=setPortPullUp(n, state);
	}
	return (ret)?FALSE:TRUE;
}



DWORD getDatum(DWORD port)
{
	DWORD ret=~0; // error value
	
	if(portIsOutput[port&0xF]) // if this port is output, nothing to read
		return ret; 

	switch(port & 0xF)
	{
		case IO_CTL_GPE4_PIN22:
			ret =(v_pGPIORegs->GPEDAT & (1<<4) )?1:0;
		break;
		case IO_CTL_GPK0_PIN23:
			ret = (v_pGPIORegs->GPKDAT & (1<<0) )?1:0;
		break;
		case IO_CTL_GPK1_PIN24:
			ret = (v_pGPIORegs->GPKDAT & (1<<1))?1:0;
		break;
		case IO_CTL_GPK2_PIN25:
			ret = (v_pGPIORegs->GPKDAT & (1<<2))?1:0;
		break;
		case IO_CTL_GPK3_PIN26:
			ret = (v_pGPIORegs->GPKDAT & (1<<3))?1:0;
		break;
		case IO_CTL_GPK4_PIN27:
			ret = (v_pGPIORegs->GPKDAT & (1<<4))?1:0;
		break;
		case IO_CTL_GPK5_PIN28:
			ret = (v_pGPIORegs->GPKDAT & (1<<20))?1:0;
		break;
		case IO_CTL_GPL8_PIN29:
			ret = (v_pGPIORegs->GPLDAT & (1<<8))?1:0;
		break;
		case IO_CTL_GPL9_PIN30:
			ret = (v_pGPIORegs->GPLDAT & (1<<9))?1:0;
		break;
		case IO_CTL_GPL10_PIN31:
			ret = (v_pGPIORegs->GPLDAT & (1<<10))?1:0;
		break;
		case IO_CTL_GPL11_PIN32:
			ret = (v_pGPIORegs->GPLDAT & (1<<11))?1:0;
		break;

		case IO_CTL_GPP8_PIN33:
			ret = (v_pGPIORegs->GPPDAT & (1<<8))?1:0;
		break;
		case IO_CTL_GPP9_PIN34:
			ret = (v_pGPIORegs->GPPDAT & (1<<9))?1:0;
		break;
		case IO_CTL_GPP12_PIN35:
			ret = (v_pGPIORegs->GPPDAT & (1<<12))?1:0;
		break;

		case IO_CTL_GPE0_PIN36:
			ret =(v_pGPIORegs->GPEDAT & (1<<0) )?1:0;
		break;
		case IO_CTL_GPE1_PIN37:
			ret =(v_pGPIORegs->GPEDAT & (1<<1) )?1:0;
		break;

		default:
			break;
	};

	return ret;
}

BOOL setDatum(DWORD port, BOOL HighLow)
{
	
	if(!portIsOutput[port&0xF]) // if this port is output, nothing to read
		return FALSE; 
	
	switch(port & 0xF)
	{
		case IO_CTL_GPE4_PIN22:
			if(HighLow)
			    v_pGPIORegs->GPEDAT |=  (1<<4);
			else
				v_pGPIORegs->GPEDAT &=  ~(1<<4);
		break;
		case IO_CTL_GPK0_PIN23:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<0);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<0);
		break;
		case IO_CTL_GPK1_PIN24:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<1);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<1);
		break;
		case IO_CTL_GPK2_PIN25:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<2);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<2);
		break;
		case IO_CTL_GPK3_PIN26:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<3);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<3);
		break;
		case IO_CTL_GPK4_PIN27:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<4);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<4);
		break;
		case IO_CTL_GPK5_PIN28:
			if(HighLow)
				v_pGPIORegs->GPKDAT |= (1<<20);
			else
				v_pGPIORegs->GPKDAT &= ~(1<<20);
		break;
		case IO_CTL_GPL8_PIN29:
			if(HighLow)
				v_pGPIORegs->GPLDAT |= (1<<8);
			else
				v_pGPIORegs->GPLDAT &= ~(1<<8);
		break;
		case IO_CTL_GPL9_PIN30:
			if(HighLow)
				v_pGPIORegs->GPLDAT |= (1<<9);
			else
				v_pGPIORegs->GPLDAT &= ~(1<<9);
		break;
		case IO_CTL_GPL10_PIN31:
			if(HighLow)
				v_pGPIORegs->GPLDAT |= (1<<10);
			else
				v_pGPIORegs->GPLDAT &= ~(1<<10);
		break;
		case IO_CTL_GPL11_PIN32:
			if(HighLow)
				v_pGPIORegs->GPLDAT |=(1<<11);
			else
				v_pGPIORegs->GPLDAT &=~(1<<11);
		break;

		case IO_CTL_GPP8_PIN33:
			if(HighLow)
				v_pGPIORegs->GPPDAT |= (1<<8);
			else
				v_pGPIORegs->GPPDAT &= ~(1<<8);
		break;
		case IO_CTL_GPP9_PIN34:
			if(HighLow)
				v_pGPIORegs->GPPDAT |= (1<<9);
			else
				v_pGPIORegs->GPPDAT &= ~(1<<9);
		break;
		case IO_CTL_GPP12_PIN35:
			if(HighLow)
				v_pGPIORegs->GPPDAT |= (1<<12);
			else
				v_pGPIORegs->GPPDAT &= ~(1<<12);
		break;

		case IO_CTL_GPE0_PIN36:
			if(HighLow)
				v_pGPIORegs->GPEDAT |= (1<<0);
			else
				v_pGPIORegs->GPEDAT &= ~(1<<0);
		break;
		case IO_CTL_GPE1_PIN37:
			if(HighLow)
				v_pGPIORegs->GPEDAT |= (1<<1);
			else
				v_pGPIORegs->GPEDAT &= ~(1<<1);
		break;

		default:
			break;
	};

	return TRUE;	
}
BOOL GIO_IOControl(DWORD hOpenContext, DWORD dwCode, PBYTE pBufIn, DWORD dwLenIn, PBYTE pBufOut, DWORD dwLenOut, PDWORD pdwActualOut)
{
	int cmd;

	cmd=(dwCode&0xf0);
	active_port=(dwCode&0x0f);
	DWORD *pArgs=0L;
	if(dwLenIn!=0 && dwLenIn==4)
	{
		pArgs=(DWORD *)pBufIn;
	}
	switch(cmd)
	{
		case IO_CTL_GPIO_SELECT:
			if(pArgs!=0L)
				active_port=(*pArgs&0x0f);
		break;
		case IO_CTL_GPIO_SET_OUTPUT:
			if(pArgs==0L)
				return setPortToOutput(active_port);
			else
				return bulkSetPortsToOutput(*pArgs & 0xFF);
		break;
		case IO_CTL_GPIO_SET_INPUT:
			if(pArgs==0L)
				return setPortToInput(active_port);
			else
				return bulkSetPortsToInput(*pArgs & 0xFF);
		break;
		case IO_CTL_GPIO_PULL_UP:
			if(pArgs==0L)
				return setPortPullUp(active_port, 2);
			else
				return bulkSetPortsToPullUpDown((*pArgs) & 0xFF,2);
		break;
		case IO_CTL_GPIO_PULL_DOWN:
			if(pArgs==0L)
				return setPortPullUp(active_port, 1);
			else
				return bulkSetPortsToPullUpDown((*pArgs) & 0xFF,1);
		break;
		case IO_CTL_GPIO_NO_PUD:
			if(pArgs==0L)
				return setPortPullUp(active_port, 0);
			else
				return bulkSetPortsToPullUpDown((*pArgs) & 0xFF,0);
		break;

		case IO_CTL_GPIO_RW_MODE_BULK:
			bulk_active=TRUE;
		break;

		case IO_CTL_GPIO_RW_MODE_SINGLE:
			bulk_active=FALSE;
		break;

		default:
			break;
	}

	//RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control:Ioctl code = 0x%x\r\n"), dwCode));
	return TRUE;
}

DWORD GIO_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode)
{

	RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_Open\r\n")));
	return TRUE;
}

BOOL GIO_Close(DWORD hOpenContext)
{
	RETAILMSG(DEBUG_GPIO,(TEXT("GPIO_Control: GPIO_Close\r\n")));
	return TRUE;
}

void GIO_PowerDown(DWORD hDeviceContext)
{
	RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_PowerDown\r\n")));
}

void GIO_PowerUp(DWORD hDeviceContext)
{
	RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_PowerUp\r\n")));
}

DWORD GIO_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	// depending on Single or Bulk mode, returns status of GPIO(s)[High/Low]
	DWORD value=~0;
	if(bulk_active)
	{
		for(int n=0;n<16;n++)
		{
			value |= ((getDatum(n)==1)?1:0)<<n;
		}
	}
	else
	{
		value=getDatum(active_port);
	}
	if(Count!=4)
	{
		RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_Read needs four buffer bytes ( sizeof(DWORD) )\r\n")));
		return FALSE;
	}
	*(DWORD *)pBuffer =value;
	
	return TRUE;
}

DWORD GIO_Seek(DWORD hOpenContext, long Amount, DWORD Type)
{
	RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_Seek\r\n")));
	return 0;
}

DWORD GIO_Write(DWORD hOpenContext, LPCVOID pSourceBytes, DWORD NumberOfBytes)
{
	DWORD value;
	// depending on Single or Bulk mode, sets status of GPIO(s) [High/Low]
	if(NumberOfBytes!=4)
	{
		RETAILMSG(DEBUG_GPIO, (TEXT("GPIO_Control: GPIO_Write expects four buffer bytes ( sizeof(DWORD) )\r\n")));
		return FALSE;
	}
	value=*(DWORD *)pSourceBytes;

	if(bulk_active)
	{
		for(int n=0;n<16;n++)
		{
			setDatum(n, (value & 1<<n)?TRUE:FALSE);
		}
	}
	else
	{
		setDatum(active_port, (value)?1:0);
	}

	return TRUE;
}




