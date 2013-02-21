//
// Copyright (c) Hiteg Ltd.  All rights reserved.
// Copyright (c) Microsoft Corporation
// Copyright (c) SAMSUNG Electronics
//	file: utils.c
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
#include "utils.h"

unsigned char toUpper(unsigned char ch)
{
	if(ch>='A' && ch<='Z') return ch;
	if(ch>='a' && ch<='z') return ch-32; // if small (97, 98,...) we subtract 32

	return ch;
}
unsigned char getChar()
{
	unsigned char KeySelect = 0;
        while (! ( ( (KeySelect == 27) ||  (KeySelect == 8  ) ) ||
				   ( (KeySelect >= '0') && (KeySelect <= '9') ) ||
                   ( (KeySelect == 'A') || (KeySelect == 'a') ) ||
                   ( (KeySelect == 'B') || (KeySelect == 'b') ) ||
                   ( (KeySelect == 'C') || (KeySelect == 'c') ) ||
                   ( (KeySelect == 'D') || (KeySelect == 'd') ) ||
				   ( (KeySelect == 'E') || (KeySelect == 'e') ) ||
                   ( (KeySelect == 'F') || (KeySelect == 'f') ) ||
                   ( (KeySelect == 'G') || (KeySelect == 'g') ) ||
                   ( (KeySelect == 'H') || (KeySelect == 'h') ) ||
                   ( (KeySelect == 'I') || (KeySelect == 'i') ) ||
                   ( (KeySelect == 'J') || (KeySelect == 'j') ) ||
                   ( (KeySelect == 'K') || (KeySelect == 'k') ) ||
                   ( (KeySelect == 'L') || (KeySelect == 'l') ) ||
                   ( (KeySelect == 'M') || (KeySelect == 'm') ) ||
                   ( (KeySelect == 'N') || (KeySelect == 'n') ) ||
                   ( (KeySelect == 'O') || (KeySelect == 'o') ) ||
                   ( (KeySelect == 'P') || (KeySelect == 'p') ) ||
                   ( (KeySelect == 'Q') || (KeySelect == 'q') ) ||  
                   ( (KeySelect == 'R') || (KeySelect == 'r') ) ||
                   ( (KeySelect == 'S') || (KeySelect == 's') ) ||
                   ( (KeySelect == 'T') || (KeySelect == 't') ) ||                   
                   ( (KeySelect == 'U') || (KeySelect == 'u') ) ||     
                   ( (KeySelect == 'V') || (KeySelect == 'v') ) ||
                   ( (KeySelect == 'W') || (KeySelect == 'w') ) || 
                   ( (KeySelect == 'X') || (KeySelect == 'x') ) ||     
                   ( (KeySelect == 'Y') || (KeySelect == 'y') ) ||
                   ( (KeySelect == 'Z') || (KeySelect == 'z') ) ))
        {
            KeySelect = OEMReadDebugByte();
        }

        EdbgOutputDebugString ( "%c\r\n", KeySelect);

		return toUpper(KeySelect);
}

/*
    @func   BOOL | ConfirmProcess | Check if continue or not
    @rdesc  TRUE == Success and FALSE == Failure.
    @comm
    @xref
*/
BOOL ConfirmProcess(const char *msg)
{
    BYTE KeySelect = 0;

    EdbgOutputDebugString ( msg);            
    while (! ( ( (KeySelect == 'Y') || (KeySelect == 'y') ) ||
               ( (KeySelect == 'N') || (KeySelect == 'n') ) ))
    {
        KeySelect = OEMReadDebugByte();
    }
    
    if(KeySelect == 'Y' || KeySelect == 'y')
    {
        return TRUE;
    }
    return FALSE;
}

USHORT InputNumericalString(CHAR *szCount, UINT32 length)
{
    USHORT cwNumChars = 0;
    USHORT InChar = 0;
    
    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA) 
        {
            // If it's a number or a period, add it to the string.
            //
            if ((InChar >= '0' && InChar <= '9')) 
            {
                if (cwNumChars < length) 
                {
                    szCount[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8) 
            {
                if (cwNumChars > 0) 
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }
 
    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars) 
    {
        szCount[cwNumChars] = '\0';
    }
    else
    {
        szCount[0] = '\0';
    }
    return cwNumChars;
}
USHORT InputNumericalHex(CHAR *szCount, UINT32 length)
{
    USHORT cwNumChars = 0;
    USHORT InChar = 0;
    
    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = toUpper(OEMReadDebugByte());
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA) 
        {
            // If it's a number or a period, add it to the string.
            //
            if ((InChar >= '0' && InChar <= '9')) 
            {
                if (cwNumChars < length) 
                {
                    szCount[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            if ((InChar >= 'A' && InChar <= 'F')) 
            {
                if (cwNumChars < length) 
                {
                    szCount[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8) 
            {
                if (cwNumChars > 0) 
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }
 
    // If it's a carriage return with an empty string, don't change anything.
    //
    if (cwNumChars) 
    {
        szCount[cwNumChars] = '\0';
    }
    else
    {
        szCount[0] = '\0';
    }
    return cwNumChars;
}
ULONG mystrtoul(PUCHAR pStr, UCHAR nBase)
{
    UCHAR nPos=0;
    BYTE c;
    ULONG nVal = 0;
    UCHAR nCnt=0;
    ULONG n=0;

    // fulllibc doesn't implement isctype or iswctype, which are needed by
    // strtoul, rather than including coredll code, here's our own simple strtoul.

    if (pStr == NULL)
        return(0);

    for (nPos=0 ; nPos < strlen(pStr) ; nPos++)
    {
        c = tolower(*(pStr + strlen(pStr) - 1 - nPos));
        if (c >= '0' && c <= '9')
            c -= '0';
        else if (c >= 'a' && c <= 'f')
        {
            c -= 'a';
            c  = (0xa + c);
        }

        for (nCnt = 0, n = 1 ; nCnt < nPos ; nCnt++)
        {
            n *= nBase;
        }
        nVal += (n * c);
    }

    return(nVal);
}
DWORD getNumber(int hex)
{
	char buffer[65];
	int amount;
	DWORD target;
	if(hex==0)
	{
		amount=InputNumericalString(buffer, 64);
		target=mystrtoul(buffer, 10);
	}
	else
	{
		amount=InputNumericalHex(buffer, 64);
		target=mystrtoul(buffer, 16);
	}

	return target;
}
USHORT GetIPString(char *szDottedD)
{
    USHORT InChar = 0;
    USHORT cwNumChars = 0;

    while(!((InChar == 0x0d) || (InChar == 0x0a)))
    {
        InChar = OEMReadDebugByte();
        if (InChar != OEM_DEBUG_COM_ERROR && InChar != OEM_DEBUG_READ_NODATA)
        {
            // If it's a number or a period, add it to the string.
            //
            if (InChar == '.' || (InChar >= '0' && InChar <= '9'))
            {
                if (cwNumChars < 16)        // IP string cannot over 15.  xxx.xxx.xxx.xxx
                {
                    szDottedD[cwNumChars++] = (char)InChar;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
            // If it's a backspace, back up.
            //
            else if (InChar == 8)
            {
                if (cwNumChars > 0)
                {
                    cwNumChars--;
                    OEMWriteDebugByte((BYTE)InChar);
                }
            }
        }
    }

    return cwNumChars;

}
 void Delay(UINT32 usec)
{
    volatile int i, j = 0;
    volatile static int loop = S3C6410_ACLK/100000;
    
    for(;usec > 0;usec--)
        for(i=0;i < loop; i++) { j++; }
}

