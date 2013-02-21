//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//	file: DisplayMenu.c
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2012. Hiteg Ltd  All rights reserved.

Module Name:  
			Display
Abstract:

    Menu to configure Display attached to the TenByTen6410

rev:
	2012.03.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 
	
	provides function DisplayMenu(DWORD *displayType)
	depends on:
		getChar()
		initializeDisplay()

--*/
#include "DisplayMenu.h"
#include "s3c6410_ldi.h"
#include "utils.h"

// we call this one each time the user chooses a display
// forward declartion, tbf at main.c
extern void identifyDisplay();
extern  void initDisplay();
extern unsigned char getChar();
extern void OEMsetDisplayType(DWORD value);
extern void OEMsetLCDBpp(DWORD value);
extern void OEMclrLogoHW();
extern void editDisplayMenu(unsigned type);

DWORD enterBGColor()
{
	char szCount[7];
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("\r\n####   Enter WEB FORMAT RGB, like FFFFFF ####\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("(eg: FFFFFF for white) >#");
	InputNumericalHex(szCount,6);
	return (mystrtoul(szCount, 16));
}

void DisplayMenu(DWORD *displayType)
{
	DWORD display;
	int n;
displayMenu:
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("I) Identify Display (not implemented, yet)\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("S) Set Display manually\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	for(n=0; n<MAX_DISPLAYS; n++)
	{
		EdbgOutputDebugString("%d) %s%s\r\n",n+1,(n==*displayType)?"*":"",LDI_getDisplayName(n));
	}
	
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("N) No display connected\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("1) %sDepth 16BIT RGB565\r\n",(OEMgetLCDBpp()==16)?"*":"");
	EdbgOutputDebugString("2) %sDepth 24BIT RGB888\r\n",(OEMgetLCDBpp()==24)?"*":"");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("C) Change background Color [%x]\r\n",OEMgetBGColor());
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("R) Test-Fill with red\r\n");
	EdbgOutputDebugString("G) Test-Fill with green\r\n");
	EdbgOutputDebugString("B) Test-Fill with blue\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("D) Drop logo\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("Q) Exit\r\n");
	EdbgOutputDebugString("\r\n#############################################\r\n");
	EdbgOutputDebugString("Choose, Q or ESC to exit\r\n");
	while (TRUE)     
    {        
        unsigned char key=getChar();
        
		if(key=='S')
        {
			EdbgOutputDebugString("Type number [1..%d], press RETURN when done > ", MAX_DISPLAYS);
			
			display=getNumber(0);
			display--;
			if(display<MAX_DISPLAYS)
				OEMsetDisplayType(display);
			initDisplay();
			LDI_clearScreen(IMAGE_FRAMEBUFFER_PA_START, OEMgetBGColor());
            goto displayMenu;
        }
        if(key=='N')
        {
            OEMsetDisplayType(NO_DISPLAY); // defined in bsp_cfg.h
			initDisplay();
            goto displayMenu;
        }
        if(key=='1')
        {
            OEMsetLCDBpp(16); // defined in bsp_cfg.h
			initDisplay();
            goto displayMenu;
        }
        if(key=='C')
        {
			OEMsetBGColor(enterBGColor());
			LDI_clearScreen(IMAGE_FRAMEBUFFER_PA_START, OEMgetBGColor());
            goto displayMenu;
        }
        if(key=='R')
        {
			LDI_clearScreen(IMAGE_FRAMEBUFFER_PA_START, 0x00FF0000);
            goto displayMenu;
        }
        if(key=='G')
        {
			LDI_clearScreen(IMAGE_FRAMEBUFFER_PA_START, 0x0000FF00);
            goto displayMenu;
        }
        if(key=='D')
        {
			OEMclrLogoHW();
            goto displayMenu;
        }
        if(key=='B')
        {
			LDI_clearScreen(IMAGE_FRAMEBUFFER_PA_START, 0x000000FF);
            goto displayMenu;
        }
		if(key=='E')
		{
		 editDisplayMenu(*displayType);
		 goto displayMenu;
		}
        if(key=='2')
        {
            OEMsetLCDBpp(24); // defined in bsp_cfg.h
			initDisplay();
            goto displayMenu;
        }
		if(key=='Q' || key==27){
			initDisplay();
            break;
		}
		if(key=='I'){
			identifyDisplay();
			goto displayMenu;
		}
    };
	
}