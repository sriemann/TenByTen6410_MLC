#include <bsp.h>
#include "MegaDisplay.h"
#include "s3c6410_ldi.h"
#include "s3c6410_display_con.h"
#include "usb.h"

static unsigned g_initIIC=FALSE;
static unsigned g_hasPCA9530=FALSE;
static unsigned g_initPCA9530=FALSE;
static unsigned g_hasPWM=FALSE;
static unsigned g_minDuty=0;
static unsigned g_maxDuty=0;
static unsigned g_dutyPolarity=0;

// we need a s-curve pwm table to compensate linear PWM generated 
static const unsigned char pwmtable[32]  =
{
    0, 1, 2, 2, 2, 3, 3, 4, 5, 6, 7, 8, 10, 11, 13, 16, 19, 23,
    27, 32, 38, 45, 54, 64, 76, 91, 108, 128, 152, 181, 215, 255
};

static void InitIIC()
{
	if(g_initIIC==FALSE)
	{
		InitializeInterrupt();
		IIC_Open(100000);
		g_initIIC=TRUE;

	}
}
#define PCA9530A	0xC0
#define INPUT	0
#define PCS0	1
#define PWM0	2
#define PCS1	3
#define PWM1	4
#define LS0		5

static unsigned tmpPSC0=0;

void identifyDisplay()
{
	BACKLIGHT_BOOST *boost;
	if(g_initIIC==FALSE)
	{
		InitIIC();
		g_initIIC=TRUE;
	}

	if(IIC_Write(PCA9530A,INPUT,0xFF)==TRUE) // a write to input register has no effect
	{
		EdbgOutputDebugString(", but found PCA9530 PWM [0xC0] device\r\n");
		g_hasPCA9530=PCA9530A;
	}
	
	boost = LDI_getBoost(LDI_getDisplayType());
	g_minDuty=boost->min_duty;
	g_maxDuty=boost->max_duty;
	g_dutyPolarity=boost->polarity;
	EdbgOutputDebugString("Display has LED backlight boost: %s\r\n", boost->name);
	backlight(255);
}
static void delayLoop(int count)
{
    volatile int j,i;
    for(j = 0; j < count; j++)
        for(i=0;i<1000;i++);
}

void testPWM()
{
	unsigned i=0,tmp=0;
	unsigned ch;
	while(1)
	{
		ch=getChar();
		if(ch==27 || ch=='Q' || ch=='q') break;
		tmp=( i++) % 256;
		backlight(tmp );
		EdbgOutputDebugString ("pwm: %d\r\n",tmp);
		delayLoop(500);
	}
}
void beep(unsigned freq, unsigned delay)
{
	
	IIC_Write(g_hasPCA9530,PCS1,0); // maximum Frequenzy
	IIC_Write(g_hasPCA9530,PWM1,freq);
	Delay(delay);
	IIC_Write(g_hasPCA9530,PWM1,0);	
}

static void initPCA9530(unsigned char duty)
{
	unsigned char ls0;

	IIC_Write(g_hasPCA9530,PCS0,tmpPSC0); // maximum Frequenzy
	IIC_Write(g_hasPCA9530,PWM0,duty %256);
	IIC_Write(g_hasPCA9530,PCS1,tmpPSC0); // maximum Frequenzy
	IIC_Write(g_hasPCA9530,PWM1,255-(duty %256));
	ls0=14;
	IIC_Write(g_hasPCA9530,LS0,ls0);// Output blinks
	g_initPCA9530=TRUE;
}
void initPWMBacklight()
{
	
}
static void setPCA9530PWM(unsigned char duty)
{
	unsigned char pcDuty;
	pcDuty= ( (duty*100) / 255);
	if(pcDuty<=g_minDuty)
	{
		IIC_Write(g_hasPCA9530,LS0,(g_dutyPolarity)?0:1);// Output On
		return;
	}
	if(pcDuty>=g_maxDuty)
	{
		IIC_Write(g_hasPCA9530,LS0,(g_dutyPolarity)?1:0);// Output Off
		return;
	}
	IIC_Write(g_hasPCA9530,PCS0,tmpPSC0); // variable Frequenzy
	IIC_Write(g_hasPCA9530,PWM0,duty);
	IIC_Write(g_hasPCA9530,PCS1,tmpPSC0); // variable Frequenzy
	IIC_Write(g_hasPCA9530,PWM1,pwmtable[255-(duty)]);
}

void backlight(unsigned char duty)
{
	unsigned char polDuty;

	if(g_dutyPolarity) 
		polDuty=duty; 
	else 
		polDuty=(255-duty);

	polDuty=pwmtable[polDuty];

	if(g_hasPCA9530!=FALSE)
	{	
		if(g_initPCA9530==FALSE)
		{
			initPCA9530(polDuty);
		}
		else
		{
			setPCA9530PWM(polDuty);
		}
	}
}


void editDisplayMenu(unsigned type)
{
	HITEG_DISPLAY tmp;
	
	static tDevInfo pDeviceInfo;
	unsigned current=1;
	unsigned ch;
	unsigned data;
	int dir=1;
	LDI_copyDisplay(&tmp,(HITEG_DISPLAY_TYPE)type );
	while(1)
	{
		EdbgOutputDebugString ("Display: %s\r\n",tmp.name);
		EdbgOutputDebugString ("%s 1) VBPD: ",(current==1)?"*":""); EdbgOutputDebugString ("[%d]\r\n",tmp.VBPD);
		EdbgOutputDebugString ("%s 2) VFPD: [%u]\r\n",(current==2)?"*":"", tmp.VFPD);
		EdbgOutputDebugString ("%s 3) VSPW: [%u]\r\n",(current==3)?"*":"", tmp.VSPW);
		EdbgOutputDebugString ("%s 4) HBPD: [%u]\r\n",(current==4)?"*":"", tmp.HBPD);
		EdbgOutputDebugString ("%s 5) HFPD: [%u]\r\n",(current==5)?"*":"", tmp.HFPD);
		EdbgOutputDebugString ("%s 6) HSPW: [%u]\r\n",(current==6)?"*":"", tmp.HSPW);
		EdbgOutputDebugString ("%s 7) FRAME_RATE: [%u]\r\n",(current==7)?"*":"", tmp.FRAME_RATE);
		EdbgOutputDebugString ("%s 8) PSC0: [%u]\r\n",(current==8)?"*":"", tmpPSC0);
		EdbgOutputDebugString ("###############################################\r\n");
		IIC_Read(g_hasPCA9530,INPUT,&data);
		EdbgOutputDebugString ("read INPUT: %u\r\n",data%0xFF);
		IIC_Read(g_hasPCA9530,PCS0,&data);
		EdbgOutputDebugString ("read PSC0: %u\r\n",data%0xFF);
		IIC_Read(g_hasPCA9530,PWM0,&data);
		EdbgOutputDebugString ("read PWM0: %u\r\n",data%0xFF);
		IIC_Read(g_hasPCA9530,PCS1,&data);
		EdbgOutputDebugString ("read PSC1: %u\r\n",data%0xFF);
		IIC_Read(g_hasPCA9530,PWM1,&data);
		EdbgOutputDebugString ("read PWM1: %u\r\n",data%0xFF);
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read LS0: %u\r\n",data%0xFF);
		EdbgOutputDebugString ("###############################################\r\n");
		ch=getChar();

		if(ch=='P' | ch=='p') dir=1;
		if(ch=='B' | ch=='b') testPWM();
		if(ch=='M' | ch=='p') dir=-1;
		if(ch=='q' || ch=='Q') return;
		if(ch>='1' && ch<='8') current=ch-'0';
		if(ch=='u' || ch=='U') {
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read %u\r\n",data%0xFF);
		IIC_Write(g_hasPCA9530,LS0,0);// Output OFF
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read back %u\r\n",data%0xFF);
		}
		if(ch=='i' || ch=='I') {
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read %u\r\n",data%0xFF);
		IIC_Write(g_hasPCA9530,LS0,(1<<2) | 1);// Output OFF
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read back %u\r\n",data%0xFF);
		}
		if(ch=='o' || ch=='O') {
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read %u\r\n",data%0xFF);
		IIC_Write(g_hasPCA9530,LS0,(2<<2) | 3);// Output OFF
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read back %u\r\n",data%0xFF);
		}
		if(ch=='y' || ch=='Y') {
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read %u\r\n",data%0xFF);
		IIC_Write(g_hasPCA9530,LS0,(3<<2) | 2);// Output OFF
		IIC_Read(g_hasPCA9530,LS0,&data);
		EdbgOutputDebugString ("read back %u\r\n",data%0xFF);
		}
		if(ch=='p' || ch=='P' || ch=='m' || ch=='M')
		{
			switch(current)
			{
				case 1:
					tmp.VBPD+=dir;
					break;
				case 2:
					tmp.VFPD+=dir;
					break;
				case 3:
					tmp.VSPW+=dir;
					break;
				case 4:
					tmp.HBPD+=dir;
					break;
				case 5:
					tmp.HFPD+=dir;
					break;
				case 6:
					tmp.HSPW+=dir;
					break;
				case 7:
					tmp.FRAME_RATE+=dir;
					break;
				case 8:
					tmpPSC0+=dir;
					break;
				default:
					break;
			}

		
		pDeviceInfo.VideoOutMode = tmp.mode;
		pDeviceInfo.RGBOutMode = tmp.intrfc;
		pDeviceInfo.uiWidth = tmp.width;
		pDeviceInfo.uiHeight = tmp.height;
		pDeviceInfo.VBPD_Value = tmp.VBPD;
		pDeviceInfo.VFPD_Value = tmp.VFPD;
		pDeviceInfo.VSPW_Value = tmp.VSPW;
		pDeviceInfo.HBPD_Value = tmp.HBPD;
		pDeviceInfo.HFPD_Value = tmp.HFPD;
		pDeviceInfo.HSPW_Value = tmp.HSPW;
		pDeviceInfo.VCLK_Polarity = tmp.VCLK_POL;
		pDeviceInfo.HSYNC_Polarity = tmp.HSYNC_POL;
		pDeviceInfo.VSYNC_Polarity = tmp.VSYNC_POL;
		pDeviceInfo.VDEN_Polarity = tmp.VDEN_POL;
		pDeviceInfo.PNR_Mode = tmp.PNR_MODE;
		pDeviceInfo.VCLK_Source = tmp.VCLK_SRC;
		pDeviceInfo.VCLK_Direction = tmp.VCLK_DIR;
		pDeviceInfo.Frame_Rate = tmp.FRAME_RATE;

		Disp_set_output_device_information(&pDeviceInfo);
		Disp_initialize_output_interface(DISP_VIDOUT_RGBIF);
		Disp_set_window_mode(DISP_WIN1_DMA, DISP_16BPP_565, tmp.width, tmp.height, 0, 0);
		Disp_set_framebuffer(DISP_WIN1,IMAGE_FRAMEBUFFER_PA_START);
		Disp_window_onfoff(DISP_WIN1, DISP_WINDOW_ON);
		Disp_envid_onoff(DISP_ENVID_ON); 
		}
	}
}