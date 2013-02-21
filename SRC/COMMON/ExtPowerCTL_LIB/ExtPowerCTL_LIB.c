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

    Low Level "C" - driver to control external power modules found on the TenByTen6410

rev:
	2012.03.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 
--*/   


#include "ExtPowerCTL_LIB.h"
#include <bsp.h>
#include <pmplatform.h>

#define EPC_DBG_MSG	  0

static char *PWRCTL_OnOFF[]={"OFF", "ON", "DEFAULTS","CUSTOM"};

volatile S3C6410_GPIO_REG *v_pIOP_PIO_regs;

static DWORD sleep_powerCTL;

/**
*	we maintain this table as the contrepid series of core boards
*	will feature a i2c I/O expander (8bit). Some outputs will be low active
*	thus we use the table to calculate the resulting output byte for the IC
**/

static PWRCTL_POWER_MODULES TenByTen6410_PWR_MOD[]=
{
    {"USB", HIGH},
    {"LCD", HIGH},
    {"DAC0", HIGH},
    {"DAC1", HIGH},
    {"ETH", HIGH},
    {"WIFI", HIGH},
    {"CAM", HIGH},
	{"ERROR", LOW},
};

/**
*		calculates the output byte
**/
#if 0 // we don't need that right now, just as example for later use
unsigned char PWRCTL_getOutputByte(unsigned char *power)
{
	unsigned char output;
	output = 0;
	for(int n=0; n<8; n++)
	{
		if((*power)&(1<<n) && TenByTen6410_PWR_MOD[n]==HIGH)
			output !=(1<<n);
		if(!((*power)&(1<<n)) && TenByTen6410_PWR_MOD[n]==LOW)
			output !=(1<<n);
	}
	return output;
}
#endif

char *PWRCTL_getPowerModuleName(unsigned int n)
{
	if(n>=0 && n<max_devices)
	{
		return TenByTen6410_PWR_MOD[n].name;
	}

	return TenByTen6410_PWR_MOD[max_devices].name;
}


/**
    @func   void | sets a bit to logical 1 in given byte at position bit.
    @comm
    @xref
**/

void PWRCTL_setPower(DWORD *power, PWRCTL_POWERCTL bit)
{
    *power|=(1<<(int) bit);
}

/**
    @func   void | clears a bit to logical 0 in given byte at position bit.
    @comm
    @xref
**/
void PWRCTL_clrPower(DWORD* power, PWRCTL_POWERCTL bit)
{
    *power &=~(1<<(int) bit);
}

/**
    @func   void | toggles a bit to in given byte at position bit.
    @comm
    @xref
**/
void PWRCTL_togglePower(DWORD *power, PWRCTL_POWERCTL bit)
{
    *power ^= (1<<(int) bit);
}

/**
    @func   bool | returns the value (0|1) of a bit at given byte at position bit.
    @comm
    @xref
**/

int PWRCTL_getPower(DWORD *power,PWRCTL_POWERCTL bit)
{
    return (*power&(1<<(int) bit))?1:0;
}
/**
    @func   bool | returns the value (0|1) of a bit at given byte at position bit.
    @comm
    @xref
**/

DWORD PWRCTL_getStatus(DWORD *power,PWRCTL_POWERCTL bit)
{
    return (*power&(1<<(int) bit))?1:0;
}

/**
    @func   STR | returns as string (ON|OFF) the bit of given byte at position bit.
    @comm
    @xref
**/
char *PWRCTL_getStatusOnOff(DWORD power, PWRCTL_POWERCTL bit)
{
    return PWRCTL_OnOFF[PWRCTL_getStatus(&power, bit)];
    
}
/**
    @func   STR | returns as string (DEFAULT| CUSTOM) comparing given byte with HITEG_FACTORY_DEFAULTS.
    @comm
    @xref
**/
char *PWRCTL_isDefault(DWORD power)
{
	if(power == (PWRCTL_HITEG_FACTORY_DEFAULTS))
		return PWRCTL_OnOFF[PWRCTL_DEFAULT_CONFIG];

	return PWRCTL_OnOFF[PWRCTL_CUSTOM_CONFIG];
}
/**
    @func   void | switches the external USB power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setUSBPower(int state)
{
    if(state)
    {
		RETAILMSG(EPC_DBG_MSG, (TEXT("++USB ExtPower ON\r\n")));
       v_pIOP_PIO_regs->GPEDAT |= (1 << 3);
        return;
    }
	RETAILMSG(EPC_DBG_MSG, (TEXT("++USB ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPEDAT &= ~(1 << 3);
}

/**
    @func   void | switches the external LCD power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setLCDPower(int state)
{
    if(state)
    {
		RETAILMSG(EPC_DBG_MSG, (TEXT("++LCD ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPLDAT |= (1 << 12);      
        return;
    }    
	RETAILMSG(EPC_DBG_MSG, (TEXT("++LCD ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPLDAT &= ~(1 << 12); 
}

/**
    @func   void | switches the external DAC1 power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setDAC1Power(int state)
{
    if(state)
    {    
		RETAILMSG(EPC_DBG_MSG, (TEXT("++DAC1 ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPMDAT |= (1 << 5);
        PWRCTL_setDAC0Power(state); // we also switch on the DAC0 if DAC1 is enabled as it doesn't make sense to 
							 // to have it alone running.
        return;
    }  
	RETAILMSG(EPC_DBG_MSG, (TEXT("++DAC1 ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPMDAT &= ~(1 << 5);  

}
/**
    @func   void | switches the external DAC0 power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setDAC0Power(int state)
{
    if(state)
    {
		RETAILMSG(EPC_DBG_MSG, (TEXT("++DAC0 ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPMDAT |= (1 << 4);   
        return;
    }
	RETAILMSG(EPC_DBG_MSG, (TEXT("++DAC0 ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPMDAT &= ~(1 << 4);

    PWRCTL_setDAC1Power(state); // S-Video needs DAC0 and DAC1, therefore DAC1 get's off if DAC0 is off.

}
/**
    @func   void | switches the external ETH power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setETHPower(int state)
{
    if(state)
    {   
		RETAILMSG(EPC_DBG_MSG, (TEXT("++ETH ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPPDAT |= (1 << 2);   
        return;
    }
	RETAILMSG(EPC_DBG_MSG, (TEXT("++ETH ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPPDAT &= ~(1 << 2); 
}
/**
    @func   void | switches the external CAM power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/

void PWRCTL_setCAMPower(int state)
{
    if(state)
    {
		RETAILMSG(EPC_DBG_MSG, (TEXT("++CAM ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPPDAT |= (1 << 14); 
		return;
    }
   RETAILMSG(EPC_DBG_MSG, (TEXT("++CAM ExtPower OFF\r\n")));
    v_pIOP_PIO_regs->GPPDAT &= ~(1 << 14);  
}

/**
    @func   void | switches the external WiFi power module ON or OFF depending on state (0|1).
    @comm
    @xref
**/
void PWRCTL_setWiFiPower(int state)
{
    if(state)
    {
        RETAILMSG(EPC_DBG_MSG, (TEXT("++WIFI ExtPower ON\r\n")));
        v_pIOP_PIO_regs->GPPDAT |= (1 << 13);  
		
        return;
    }
    RETAILMSG(EPC_DBG_MSG, (TEXT("++WIFI ExtPower OFF\r\n"))); 
    v_pIOP_PIO_regs->GPPDAT &= ~(1 << 13);  
}
/**
    @func   void | switches all external power modules ON or OFF depending on given byte.
    @comm
    @xref
**/
void PWRCTL_setAllTo(DWORD *power)
{
    PWRCTL_setUSBPower(PWRCTL_getStatus(power,USB));
    PWRCTL_setLCDPower(PWRCTL_getStatus(power,LCD));
    if(!PWRCTL_getStatus(power, DAC0) && PWRCTL_getStatus(power, DAC1)) // we don't need DAC1 running if DAC0 is off 
    {
       PWRCTL_clrPower(power,DAC1); // so we switch it off 
    }
    PWRCTL_setDAC1Power(PWRCTL_getStatus(power,DAC1));
    PWRCTL_setDAC0Power(PWRCTL_getStatus(power,DAC0));
    PWRCTL_setETHPower(PWRCTL_getStatus(power,ETH));
    PWRCTL_setCAMPower(PWRCTL_getStatus(power,CAM));
    PWRCTL_setWiFiPower(PWRCTL_getStatus(power,WIFI));
}

/**
    @func   void | sets factory defaults.
    @comm
    @xref
**/
void PWRCTL_setDefaultPower(DWORD *power)
{
 *power = PWRCTL_HITEG_FACTORY_DEFAULTS; // our default .... 
 PWRCTL_setAllTo(power);
}
/**
    @func   void | initializes the GPIO registers.
    @comm
    @xref
**/
void PWRCTL_powerCTLInit(volatile S3C6410_GPIO_REG *regs)
{
	v_pIOP_PIO_regs=regs;
    // USB HOST power module controled by GPE3
 
    v_pIOP_PIO_regs->GPECON &= ~(0xf << 12); // GPE3
    v_pIOP_PIO_regs->GPECON |= (1 << 12);  // GPE3 as output

 
    // LCD power module controlled by GPL12
  
    v_pIOP_PIO_regs->GPLCON1 &= ~(0xf << 16); // GPL12
    v_pIOP_PIO_regs->GPLCON1|= (1 << 16);  // GPL12 as output    
 

    // DAC0 power module controlled by GPM4
   
    v_pIOP_PIO_regs->GPMCON &= ~(0xf << 16); // GPM4
    v_pIOP_PIO_regs->GPMCON|= (1 << 16);  // GPM4 as output    

    // DAC1 power module controlled by GPM5

    v_pIOP_PIO_regs->GPMCON &= ~(0xf << 20); // GPM5
    v_pIOP_PIO_regs->GPMCON|= (1 << 20);  // GPM5 as output    
  
    // ETH power module controlled by GPP2

    v_pIOP_PIO_regs->GPPCON &= ~(0x3 << 4); // GPP2
    v_pIOP_PIO_regs->GPPCON|= (1 << 4);  // GPP2 as output    
  
    // CAM power module controlled by GPP14
  
    v_pIOP_PIO_regs->GPPCON &= ~(0x3 << 28); // GPP14
    v_pIOP_PIO_regs->GPPCON|= (1 << 28);  // GPP14 as output    
 
    // Wifi power module controlled by GPP13   
    v_pIOP_PIO_regs->GPPCON &= ~(0x3 << 26); // GPP13
    v_pIOP_PIO_regs->GPPCON|= (1 << 26);  // GPP13 as output    
    
    v_pIOP_PIO_regs->GPPCON &= ~(0x3 << 22); // GPP11 ( software power down, still draining, however...might not work with wifi module v1 as it doesn't feature sleep CLK)
    v_pIOP_PIO_regs->GPPCON|= (1 << 22);  // GPP11 as output    
    
    v_pIOP_PIO_regs->GPPCON &= ~(0x3 << 20); // GPP10 ( reset signal )
    v_pIOP_PIO_regs->GPPCON|= (1 << 20);  // GPP10 as output    
	v_pIOP_PIO_regs->GPPDAT &=~(0x1<<11);//GPP11  as output low->WiFi_POWER
	v_pIOP_PIO_regs->GPPDAT &=~(0x1<<10);//GPP11  as output low->WiFi_POWER
    v_pIOP_PIO_regs->GPPDAT &=~(0x1<<13);//GPP13  as output low->WiFi_POWER
	v_pIOP_PIO_regs->GPPPUD |= (0x01<<22);//GPP11  pull low
	v_pIOP_PIO_regs->GPPPUD |= (0x01<<20);//GPP10  pull low
}
void PWRCTL_deinit()
{
	v_pIOP_PIO_regs->GPEDAT &=~(0x1<<3); // GPE3 as output low> USB HOST POWER
	//v_pIOP_PIO_regs->GPPDAT &=~(0x1<<14); // GPP14  as output low> camera POWER
	v_pIOP_PIO_regs->GPLDAT &= ~(0x1<<12);//GPL12 LCD POWER
	v_pIOP_PIO_regs->GPMDAT &=~(0x1<<4);// GPM4 as output low -TV-OUT0 power	   
	v_pIOP_PIO_regs->GPMDAT &=~(0x1<<5);// GPM5 as output low -TV-OUT1 power
    v_pIOP_PIO_regs->GPPDAT &=~(0x1<<2);//Ethernet_POWER ,GPP2 output low;
	v_pIOP_PIO_regs->GPPDAT &=~(0x1<<10);//GPP10  as output low->WiFi_POWER
    v_pIOP_PIO_regs->GPPDAT &=~(0x1<<11);//GPP11  as output low->WiFi_POWER
    v_pIOP_PIO_regs->GPPDAT &=~(0x1<<13);//GPP13  as output low->WiFi_POWER
}

/**
    @func   void | initialize the Lib
    @comm
    @xref
**/
void PWRCTL_InitializePowerCTL(DWORD *power,volatile S3C6410_GPIO_REG *regs)
{
	PWRCTL_powerCTLInit(regs);
	PWRCTL_setAllTo(power);
}


void PWRCTL_Sleep()
{
	BSP_ARGS *pArgs;
	DWORD tmp_power;
	tmp_power=0;
	pArgs = (BSP_ARGS*)IMAGE_SHARE_ARGS_UA_START;
	sleep_powerCTL=pArgs->powerCTL;
	PWRCTL_powerCTLInit(v_pIOP_PIO_regs);
	PWRCTL_setAllTo(&tmp_power);
}

void PWRCTL_Awake()
{
	PWRCTL_setAllTo(&sleep_powerCTL);
}