//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//	file: PWRCTL_MENU.c
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2012. Hiteg Ltd  All rights reserved.

Module Name:  

Abstract:

    Menu to configure external power modules found on the TenByTen6410

rev:
	2012.03.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 

	directory drivers/ExtPowerCTL/INC has to be added to sources INCLUDES:
	INCLUDES=$(INCLUDES);$(_TARGETPLATROOT)\src\drivers\ExtPowerCTL\INC;

	ExtPowerCTL_LIB.lib has to be added to TARGETLIBS

	$(_TARGETPLATROOT)\lib\$(_CPUINDPATH)ExtPowerCTL_LIB.lib   \


--*/

#include "PWRCTL_MENU.h"
#include "ExtPowerCTL_LIB.h"


extern unsigned char getChar();
/**
*	Displays menu to configure external Power modules as found on TenByTen6410
*	changes made are applied immediately
*	Parameter: pubyte as read from TOC 
*	result:	   ubyte to be stored in TOC and BSP ARGS
*	dependencies: 
*				ExtPowerCTL_LIB.lib
*				getChar function (to be implemented in eBOOT)
**/
/////////////////////////////////////////////////////////////////////////////////
// EXAMPLE 
/**
*
* EdbgOutputDebugString ( "P) external Power modules config: [%s]\r\n",isDefault(&powerCTL));
*
* indicates (custom | default) configuration
* assignment of key "P" would be appriciated
* 
**/

void powerCTLMenu(DWORD *powerCTL)
{
	int n;
displayMenu:
	EdbgOutputDebugString("\r\n#############################################\r\n");
	for(n=0; n<max_devices; n++)
	{
		EdbgOutputDebugString("%d) %s: %s\r\n",n+1,PWRCTL_getPowerModuleName(n), PWRCTL_getStatusOnOff(*powerCTL,n));
	}
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("O) Switch All ON\r\n");
	EdbgOutputDebugString("F) Switch All OFF\r\n");
	EdbgOutputDebugString("D) Load Factory defaults\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("Q) Exit\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("Choose [1..%d] for toggle, Q to exit\r\n", max_devices);
	while (TRUE)     
    {        
        unsigned char key=getChar();
        
        if(((key-49)>=0) && ((key-49)<=max_devices))
        {
            PWRCTL_togglePower(powerCTL, (PWRCTL_POWERCTL) (key-49));			
			PWRCTL_setAllTo(powerCTL);
            goto displayMenu;
        }
        if(key=='O')
        {
            *powerCTL=0xff;			
			PWRCTL_setAllTo(powerCTL);
            goto displayMenu;
        }
        if(key=='F')
        {
			*powerCTL=0;
			PWRCTL_setAllTo(powerCTL);
            goto displayMenu;
        }
        if(key=='D')
        {
            PWRCTL_setDefaultPower(powerCTL);
			PWRCTL_setAllTo(powerCTL);
            goto displayMenu;
        }
        if(key=='Q')
            break;
    };
    PWRCTL_setAllTo(powerCTL);
}
