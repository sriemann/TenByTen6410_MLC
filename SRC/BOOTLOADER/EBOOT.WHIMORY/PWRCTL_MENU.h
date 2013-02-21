//
// Copyright (c) Hiteg Ltd.  All rights reserved.
//
//	file: PWRCTL_MENU.h
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
#ifndef PWRCTL_MENU_H
#define PWRCTL_MENU_H
#include <bsp.h>

void powerCTLMenu(DWORD *powerCTL);

#endif