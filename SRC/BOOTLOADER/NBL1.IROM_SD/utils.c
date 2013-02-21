#include <windows.h>
#include <bsp_cfg.h>
#include "s3c6410_addr.h"
#include <soc_cfg.h>


//***************************[ PORTS ]****************************************************
void Port_Init(void)
{
    // GPN[15:12] for LED
//    rGPNCON = (rGPNCON & ~(0xff<<24))|(0x55<<24);    // GPN[15:12] as output
//   rGPNPUD = (rGPNPUD & ~(0xff<<24));                // Pull-Down/Up Disable
}


void Led_Display(int data)
{
    // Active is low.(LED On)
    // GPN15  GPN14  GPN13  GPN12
    // nLED_8 nLED4 nLED_2 nLED_1
    //
//    rGPNDAT = (rGPNDAT & ~(0xf<<12)) | ((data & 0xf)<<12);
}

static void Delay(void)
{
    volatile int i;

    for(i=0 ; i < 1000 ; i++)
    {
    }
}

const UINT32 aSlotTable[16] =
{
    0x0000, 0x0080, 0x0808, 0x0888, 0x2222, 0x4924, 0x4a52, 0x54aa,
    0x5555, 0xd555, 0xd5d5, 0xddd5, 0xdddd, 0xdfdd, 0xdfdf, 0xffdf
};

//***************************[ UART ]******************************
void Uart_Init(void)
{
    UINT32 DivSlot;
    float Div;

    // UART I/O port initialize (RXD0 : GPA0, TXD0: GPA1)
    rGPACON = (rGPACON & ~(0xff<<0)) | (0x22<<0);    // GPA0->RXD0, GPA1->TXD0
    rGPAPUD = (rGPAPUD & ~(0xf<<0)) | (0x1<<0);        // RXD0: Pull-down, TXD0: pull up/down disable

    // Initialize UART Ch0
    rULCON0 = (0<<6)|(0<<3)|(0<<2)|(3<<0);                    // Normal Mode, No Parity, 1 Stop Bit, 8 Bit Data
    rUCON0 = (0<<10)|(1<<9)|(1<<8)|(0<<7)|(0<<6)|(0<<5)|(0<<4)|(1<<2)|(1<<0);    // PCLK divide, Polling Mode
    rUFCON0 = (0<<6)|(0<<4)|(0<<2)|(0<<1)|(0<<0);            // Disable FIFO
    rUMCON0 = (0<<5)|(0<<4)|(0<<0);                        // Disable Auto Flow Control

    Div = (float)((float)S3C6410_PCLK/(16.0*(float)DEBUG_BAUDRATE)) - 1;
    DivSlot = (UINT32)((Div-(int)Div)*16);

    rUBRDIV0 = (UINT32)Div;                                    // Baud rate
    rUDIVSLOT0 = aSlotTable[DivSlot];
}

//=====================================================================
void Uart_SendByte(int data)
{
    if(data=='\n')
    {
        while (!(rUTRSTAT0 & 0x2))
		{
			;
        }
        Delay();                 //because the slow response of hyper_terminal
        WrUTXH0('\r');
    }

    while (!(rUTRSTAT0 & 0x2))
	{
		;   //Wait until THR is empty.
    }
    Delay();
    WrUTXH0(data);
}


//====================================================================
void Uart_SendString(char *pt)
{
    while (*pt)
    {
		Uart_SendByte(*pt++);
    }
}

//====================================================================
void Uart_SendDWORD(DWORD d, BOOL cr)
{
    Uart_SendString("0x");
    Uart_SendString(hex2char((d & 0xf0000000) >> 28));
    Uart_SendString(hex2char((d & 0x0f000000) >> 24));
    Uart_SendString(hex2char((d & 0x00f00000) >> 20));
    Uart_SendString(hex2char((d & 0x000f0000) >> 16));
    Uart_SendString(hex2char((d & 0x0000f000) >> 12));
    Uart_SendString(hex2char((d & 0x00000f00) >> 8));
    Uart_SendString(hex2char((d & 0x000000f0) >> 4));
    Uart_SendString(hex2char((d & 0x0000000f) >> 0));
    if (cr)
    {
        Uart_SendString("\n");
    }
}
void Uart_SendBYTE(BYTE d, BOOL cr)
{
	//Uart_SendString("0x");
	Uart_SendString(hex2char((d & 0x000000f0) >> 4));
	Uart_SendString(hex2char((d & 0x0000000f) >> 0));
	Uart_SendString(" ");
	if (cr)
	{
		Uart_SendString("\n");
	}
}
//====================================================================
char *hex2char(unsigned int val)
{
    static char str[2];

    str[1]='\0';

    if(val<=9)
        str[0]='0'+val;
    else
        str[0]=('a'+val-10);

    return str;
}

void CDataAbortHandler(DWORD r0, DWORD r1, DWORD r2, DWORD r3)
{
    Uart_SendString(hex2char(r0));
    Uart_SendString(hex2char(r1));
    Uart_SendString(hex2char(r2));
    Uart_SendString(hex2char(r3));    
}
