//
// Copyright (c) Hiteg Ltd.  All rights reserved.
// Copyright (c) Microsoft Corporation
// Copyright (c) SAMSUNG Electronics
//	file: utils.h
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
Copyright (c) 2012. Hiteg Ltd  All rights reserved.

Module Name:  
			utils
Abstract:

			container for various functions found spread out in the sources

rev:
	2012.03.30	: initial version, by Sven Riemann sven.riemann@hiteg.com

Notes: 



--*/
#ifndef UTILS_H
#define UTILS_H

#include <bsp.h>

unsigned char toUpper(unsigned char ch);
unsigned char getChar();
BOOL ConfirmProcess(const char *msg);
USHORT InputNumericalString(CHAR *szCount, UINT32 length);
USHORT InputNumericalHex(CHAR *szCount, UINT32 length);
ULONG mystrtoul(PUCHAR pStr, UCHAR nBase);
USHORT GetIPString(char *szDottedD);
void Delay(UINT32 usec);
DWORD getNumber(int hex);
#endif