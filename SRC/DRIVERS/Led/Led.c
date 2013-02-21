/*
**************************************************************************
FileName   : Led.c
Description: S3C6410 (C), Led driver
Date       : 11/05/2009
**************************************************************************
*/
#include <windows.h>
#include <s3c6410.h>
#include <ceddk.h>
#include "bsp_cfg.h"

#define LED_DBG_MSG	  0
#define LED_ERR_MSG		0

typedef enum _LED_IO_CONTROL_VALUE {
    LED_VERSION,
    IO_LED_ON    = 0x10,
    IO_LED_OFF   = 0x11,
} ENUM_LED_IO_CONTROL, *PENUM_LED_IO_CONTROL;

volatile S3C6410_GPIO_REG *v_pIOP_LED_regs = NULL;

#define     GPM0		0
#define     GPM1		4
#define     GPM2		8
#define     GPM3		12

#define     LED1           GPM0
#define     LED2           GPM1
#define     LED3           GPM2
#define     LED4           GPM3
// LED Control  bit

#define     LED1_BIT   0
#define     LED2_BIT   1
#define     LED3_BIT   2
#define     LED4_BIT   3
#define     Led_On(x)		(v_pIOP_LED_regs->GPMDAT |= (0x1 << (x)))
#define     Led_Off(x)		(v_pIOP_LED_regs->GPMDAT &= ~(0x1 << (x)))

void  InitLedAddr()
{
	//DWORD dwIOBase;
	DWORD dwIOSize;
	PHYSICAL_ADDRESS  IOPhyAdr= { S3C6410_BASE_REG_PA_GPIO, 0 };
	RETAILMSG(LED_DBG_MSG, (TEXT("++InitLedAddr\r\n")));
	dwIOSize = sizeof(S3C6410_GPIO_REG);
	if ( NULL == v_pIOP_LED_regs)
	{
		v_pIOP_LED_regs = (volatile S3C6410_GPIO_REG *)MmMapIoSpace(IOPhyAdr, (ULONG)dwIOSize, FALSE);	
		if ( NULL == v_pIOP_LED_regs)
		{
			RETAILMSG(LED_ERR_MSG, (TEXT("LED init address failed\r\n")));
		}
	}
}

void GPIOInit()
{

	if (v_pIOP_LED_regs)
	{
		//GPM0~GPM3
		v_pIOP_LED_regs->GPMPUD &= ~((0x3<<0)|(0x3<<2)|(0x3<<4)|(0x3<<6));//Disable GPD0, GPD4 pull up/down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<0);//Disable GPM0 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<2);//Disable GPM1 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<4);//Disable GPM2 pull down
		v_pIOP_LED_regs->GPMPUD |= (0x1<<6);//Disable GPM3 pull down
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<0);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<4);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<8);
		v_pIOP_LED_regs->GPMCON &= ~(0xF<<12);
		v_pIOP_LED_regs->GPMCON |= (0x1<<0);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPMCON |= (0x1<<4);//Disable GPD0, GPD4 as output
		v_pIOP_LED_regs->GPMCON |= (0x1<<8);
		v_pIOP_LED_regs->GPMCON |=(0x1<<12);
		/*
		v_pIOP_LED_regs->GPMDAT |= (0x1<<0);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT |= (0x1<<1);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT |= (0x1<<2);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT |= (0x1<<3);//Turn The led1, led2 off 
		*/
		
		v_pIOP_LED_regs->GPMDAT &= ~(0x1<<0);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT &= ~(0x1<<1);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT &= ~(0x1<<2);//Turn The led1, led2 off 
		v_pIOP_LED_regs->GPMDAT &= ~(0x1<<3);//Turn The led1, led2 off 
	}
}

void ControlLed(UINT8 u8Index, BOOL bHigh)
{
	if( bHigh)
		Led_On(u8Index);
	else
		Led_Off(u8Index);
}
	
BOOL ProcLed (UINT8 u8Index, BOOL bHigh)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("ProcLed+++\r\n")));
	if (u8Index>2)
	{
		RETAILMSG(LED_ERR_MSG, (TEXT("input error,exit function")));
		return FALSE;
	}
	
	switch(u8Index)
	{
		case  1:
			ControlLed( LED1_BIT, bHigh);
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
			break;
		case  2:
			ControlLed( LED2_BIT, bHigh);
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
		break;
		case  3:
			ControlLed( LED3_BIT, bHigh);
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
			break;
		case  4:
			ControlLed( LED4_BIT, bHigh);
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMCON=%x\r\n "),v_pIOP_LED_regs->GPMCON ));
			RETAILMSG(LED_DBG_MSG, (TEXT("******v_pIOP_LED_regs->GPMDAT=%x\r\n "),v_pIOP_LED_regs->GPMDAT));
		break;

		default:
			break;
	}
	RETAILMSG(LED_DBG_MSG, (TEXT("Operate LED: %d\r\n"), u8Index));
	RETAILMSG(LED_DBG_MSG, (TEXT("ProcLed---\r\n")));
	return TRUE;
}
BOOL WINAPI DllMain(HANDLE hinstDll, ULONG Reason, LPVOID Reserved)
{
	switch(Reason)
	{
		case DLL_PROCESS_ATTACH:
			RETAILMSG(LED_DBG_MSG,(TEXT("S3C6410 LED: DLL_PROCESS_ATTCH\r\n")));
			break;
			
		case DLL_PROCESS_DETACH:
			RETAILMSG(LED_DBG_MSG,(TEXT("S3C6410 LED: DLL_PROCESS_DETACH\r\n")));
			break;
			
		default:
			//RETAILMSG(LED_DBG_MSG,(TEXT("LED: default\r\n")));
			break;
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_Close(DWORD dwOpenContext)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Close+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Close---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_Deinit(DWORD hDeviceContext)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Deinit+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Deinit---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Init(
  LPCTSTR pContext, 
  LPCVOID lpvBusContext
)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Init+++\r\n")));
	InitLedAddr();
	GPIOInit();
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Init---\r\n")));

	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
BOOL LED_IOControl(DWORD dwOpenContext, DWORD dwIoControlCode, LPBYTE lpInBuf, 
                   DWORD nInBufSize, LPBYTE lpOutBuf, DWORD nOutBufSize, 
                   LPDWORD lpBytesReturned)
{
	RETAILMSG(LED_DBG_MSG,(TEXT("LED_IOControl+++\r\n")));
	RETAILMSG(LED_DBG_MSG,(TEXT("dwIoControlCode = %d.\r\n"), dwIoControlCode));
	switch(dwIoControlCode)
	{
		case IO_LED_ON:
			if(NULL != lpInBuf)
			{
				ProcLed(*lpInBuf, TRUE);
				RETAILMSG(LED_DBG_MSG,(TEXT("LED_IOControl: IO_LED_ON\r\n")));
			}
			else
				RETAILMSG(LED_ERR_MSG, (TEXT("The lpInBuf is NULL\r\n")));
			break;
			
		case IO_LED_OFF:
			if(NULL != lpInBuf)
			{
				ProcLed(*lpInBuf, FALSE);
				RETAILMSG(LED_DBG_MSG,(TEXT("LED_IOControl: IO_LED_OFF\r\n")));
			}
			else
				RETAILMSG(LED_ERR_MSG, (TEXT("The lpInBuf is NULL\r\n")));
			break;
			
		default:
			RETAILMSG(LED_ERR_MSG,(TEXT("LED_IOControl: default\r\n")));
			break;
	}
	RETAILMSG(LED_DBG_MSG,(TEXT("LED_IOControl---\r\n")));
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Open(DWORD hDeviceContext, DWORD AccessCode, DWORD ShareMode)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Open+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Open---\r\n")));	
	return TRUE;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LED_PowerDown(DWORD hDeviceContext)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_PowerDown+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_PowerDown---\r\n")));	
	return;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void LED_PowerUp(DWORD hDeviceContext)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_PowerUp+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_PowerUp---\r\n")));	
	return;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Read(DWORD hOpenContext, LPVOID pBuffer, DWORD Count)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Read+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Read---\r\n")));	
    return TRUE;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Seek(DWORD hOpenContext, long Amount, DWORD Type)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Seek+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Seek---\r\n")));	
    return TRUE;
} 

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
DWORD LED_Write(DWORD hOpenContext, LPCVOID pSourceBytes, DWORD NumberOfBytes)
{
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Write+++\r\n")));
	RETAILMSG(LED_DBG_MSG, (TEXT("LED_Write---\r\n")));	
    return TRUE;
}