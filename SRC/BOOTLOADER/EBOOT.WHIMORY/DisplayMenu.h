//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//	file: DisplayMenu.h
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

	directory drivers/display/INC has to be added to sources INCLUDES:
	INCLUDES=$(INCLUDES);$(_TARGETPLATROOT)\src\drivers\Display\INC;

	DISPCON_LIB.lib, LDI_LIB.lib has to be added to TARGETLIBS

	$(_TARGETPLATROOT)\lib\$(_CPUINDPATH)LDI_LIB.lib   \
	$(_TARGETPLATROOT)\lib\$(_CPUINDPATH)DISPCON_LIB.lib   \

--*/
#ifndef DISPLAYMENU_H
#define DISPLAYMENU_H

#include <bsp.h>
#include "MegaDisplay.h"

void DisplayMenu(DWORD *displayType);

#endif