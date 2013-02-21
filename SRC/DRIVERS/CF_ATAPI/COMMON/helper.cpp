//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES.
//
#include <atamain.h>

BOOL AtaGetRegistryValue(HKEY hKey, PTSTR szValueName, PDWORD pdwValue)
{
    
    DWORD               dwValType, dwValLen;
    LONG                lStatus;
            
    dwValLen = sizeof(DWORD);
        
    lStatus = RegQueryValueEx( hKey, szValueName, NULL, &dwValType, (PBYTE)pdwValue, &dwValLen);
        
    if ((lStatus != ERROR_SUCCESS) || (dwValType != REG_DWORD)) {           
        DEBUGMSG( ZONE_HELPER , (TEXT("ATAConfig: RegQueryValueEx(%s) failed -returned %d  Error=%08X\r\n"), szValueName, lStatus, GetLastError()));
        *pdwValue = -1;
        return FALSE;
    } 
    DEBUGMSG( ZONE_HELPER, (TEXT("ATAPI: AtaGetRegistryValue(%s) Value(%x) hKey: %x\r\n"), szValueName,*pdwValue,hKey));
    return TRUE;
}

BOOL AtaGetRegistryString( HKEY hKey, PTSTR szValueName, PTSTR *pszValue, DWORD dwSize)
{
    DWORD             dwValType, dwValLen;
    LONG                lStatus;
    
    dwValLen = 0;
    lStatus = RegQueryValueEx( hKey, szValueName, NULL, &dwValType, NULL, &dwValLen);

    if (dwSize && (dwValLen > dwSize)) {
        DEBUGMSG( ZONE_HELPER, (TEXT("ATAConfig: AtaGetRegistryString size specified is too small!!!\r\n")));
        return FALSE;
    }   
        
    if ((lStatus != ERROR_SUCCESS) || (dwValType != REG_SZ)) {          
        DEBUGMSG( ZONE_HELPER , (TEXT("ATAConfig: RegQueryValueEx(%s) failed -returned %d  Error=%08X\r\n"), szValueName, lStatus, GetLastError()));
        *pszValue = NULL;
        return FALSE;
    }
    
    if (!dwSize) 
        *pszValue = (PTSTR)LocalAlloc( LPTR, dwValLen);
    
    lStatus = RegQueryValueEx( hKey, szValueName, NULL, &dwValType, (PBYTE)*pszValue, &dwValLen);

    if (lStatus != ERROR_SUCCESS) {
        DEBUGMSG( ZONE_HELPER , (TEXT("ATAConfig: RegQueryValueEx(%s) failed -returned %d  Error=%08X\r\n"), szValueName, lStatus, GetLastError()));
        LocalFree( *pszValue);
        *pszValue = NULL;
        return FALSE;
    }    
    DEBUGMSG( ZONE_HELPER, (TEXT("ATAPI: AtaGetRegistryString(%s) Value(%s) hKey: %x\r\n"), szValueName,*pszValue, hKey));
    return TRUE;
}


BOOL AtaSetRegistryValue(HKEY hKey, PTSTR szValueName, DWORD dwValue)
{
    
    DWORD              dwValLen;
    LONG                lStatus;
            
    dwValLen = sizeof(DWORD);
        
    lStatus = RegSetValueEx( hKey, szValueName, 0, REG_DWORD, (PBYTE)&dwValue, dwValLen);
        
    if (lStatus != ERROR_SUCCESS) {
        DEBUGMSG( ZONE_HELPER , (TEXT("ATAConfig: RegQueryValueEx(%s) failed -returned %d  Error=%08X\r\n"), szValueName, lStatus, GetLastError()));
        return FALSE;
    } 
    DEBUGMSG( ZONE_HELPER, (TEXT("ATAPI: AtaSetRegistryValue(%s) Value(%x) hKey: %x\r\n"), szValueName, dwValue,hKey));
    return TRUE;
}


BOOL AtaSetRegistryString( HKEY hKey, PTSTR szValueName, PTSTR szValue)
{
    DWORD             dwValLen;
    LONG               lStatus;
    
    dwValLen = (wcslen( szValue)+1)*sizeof(WCHAR);
    lStatus = RegSetValueEx( hKey, szValueName, 0, REG_SZ, (PBYTE)szValue, dwValLen);
        
    if (lStatus != ERROR_SUCCESS) {          
        DEBUGMSG( ZONE_HELPER , (TEXT("ATAConfig: RegSetValueEx(%s) failed -returned %d  Error=%08X\r\n"), szValueName, lStatus, GetLastError()));
        return FALSE;
    }
    
    DEBUGMSG( ZONE_HELPER, (TEXT("ATAPI: AtaSetRegistryString(%s) Value(%s) hKey: %x\r\n"), szValueName, szValue, hKey));
    return TRUE;
}


 BOOL ATAParseIdString(BYTE *str, int len, DWORD *pdwOffset,BYTE **ppDest, DWORD *pcBytesLeft)
{
    BYTE *p;
    DWORD cCopied;
    BYTE *pDest;
    
    // check validity (spec says ASCII, I assumed printable)
    for (p = str; p < &str[len]; ++p)
        if (*p < 0x20 || *p > 0x7F) {
            *pdwOffset = 0;
            return FALSE;
        }

    // find the last non-pad character
    for (p = &str[len]; p > str && p[-1] == ' '; --p)
        ;
    cCopied = p - str;

    // special case - empty string implies not present
    if (cCopied == 0) {
        *pdwOffset = 0;
        return TRUE;
    }
    ++cCopied; // this is a byte count so add a terminating null

    // always increment *ppDest because it counts the bytes that we want,
    // not just the bytes that we've actually written.
    pDest = *ppDest;
    *ppDest += cCopied;

    // if there has already been an error, then we needn't continue further
    if (GetLastError() != ERROR_SUCCESS)
        return TRUE;

    // make sure there's enough space to copy into
    if (cCopied > *pcBytesLeft) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        cCopied = *pcBytesLeft;
    }
    __try {
        if (cCopied) {
            memcpy(pDest, str, cCopied-1);
            pDest[cCopied-1] = '\0';
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
    };

    *pcBytesLeft -= cCopied;
    return TRUE;
}

