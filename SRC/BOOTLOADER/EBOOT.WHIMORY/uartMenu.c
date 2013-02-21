//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//	file: uartMenu.c
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2012. Hiteg Ltd  All rights reserved.

Module Name:  
			UART
Abstract:

    Menu to set debuig UART attached to the TenByTen6410

rev:
	2012.10.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 
	
--*/

#include "debugUART.h"
static unsigned uart;

const static char *debugUARTNames[]={
"COM1","COM2","COM3","COM4","NONE",
};

char * getDebugUARTName(unsigned port)
{
	if(port<4) return debugUARTNames[port];

	return debugUARTNames[4];
}


unsigned toggleDebugUART(unsigned port)
{
	port++;
	if(port>4) port=0;
	return port;
}

