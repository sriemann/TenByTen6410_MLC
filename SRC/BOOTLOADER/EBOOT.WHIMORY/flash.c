//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of the Microsoft end-user
// license agreement (EULA) under which you licensed this SOFTWARE PRODUCT.
// If you did not accept the terms of the EULA, you are not authorized to use
// this source code. For a copy of the EULA, please see the LICENSE.RTF on your
// install media.
//

#include <windows.h>
#include <bsp.h>
#include "loader.h"

extern DWORD g_dwMinImageStart;
extern DWORD g_ImageType;

/*
    @func   BOOL | OEMIsFlashAddr | Tests whether the address provided resides in the Samsung's flash.
    @rdesc  TRUE = Specified address resides in flash, FALSE = Specified address doesn't reside in flash.
    @comm    
    @xref   
*/
BOOL OEMIsFlashAddr(DWORD dwAddr)
{
    //EdbgOutputDebugString("OEMIsFlashAddr: 0x%x, %d\r\n", dwPhysStart, bRc);
    return(FALSE);
}


/*
    @func   LPBYTE | OEMMapMemAddr | Remaps a specified address to a file cache location.  The file cache is used as a temporary store for flash images before they're written to flash.
    @rdesc  Corresponding address within a file cache area.
    @comm    
    @xref   
*/
LPBYTE OEMMapMemAddr(DWORD dwImageStart, DWORD dwAddr)
{
    if (g_ImageType & IMAGE_TYPE_STEPLDR)
    {
        dwAddr = (FILE_CACHE_START + (dwAddr - STEPLDR_RAM_IMAGE_BASE));
        return (LPBYTE)dwAddr;
    }
    else
    if (g_ImageType & IMAGE_TYPE_LOADER)
    {
        dwAddr = (FILE_CACHE_START + (dwAddr - EBOOT_RAM_IMAGE_BASE));
        return (LPBYTE)dwAddr;
    }
    else
    if (g_ImageType & IMAGE_TYPE_RAWBIN)
    {
        OALMSG(TRUE, (TEXT("OEMMapMemAddr 0x%x  0x%x\r\n"),dwAddr,(FILE_CACHE_START + dwAddr)));
        dwAddr = FILE_CACHE_START + dwAddr;
        return (LPBYTE)dwAddr;
    }
/*    else    // for MultipleXIP
    if (g_ImageType & IMAGE_TYPE_RAMIMAGE)
    {
        // We use the lowest address of all the BIN files being downloaded as the datum for temporarily caching
        // the image in RAM prior to storage in flash...
        if (!g_dwMinImageStart)
        {
            g_dwMinImageStart = dwImageStart;
        }
        // If it's a flash address, translate it by "rebasing" the address into the file cache area.
        if (OEMIsFlashAddr(dwAddr) && (dwImageStart <= dwAddr))
        {
            dwAddr = (FILE_CACHE_START + (dwAddr - g_dwMinImageStart));
               return((LPBYTE)dwAddr);
        }
    }
*/
    return (LPBYTE)dwAddr;
}


/*
    @func   BOOL | OEMStartEraseFlash | Called at the start of image download, this routine begins the flash erase process.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm    
    @xref   
*/
BOOL OEMStartEraseFlash(DWORD dwStartAddr, DWORD dwLength)
{
    // Nothing to do (erase done in OEMWriteFlash)...
    //
    //EdbgOutputDebugString( "OEMStartEraseFlash: Addr:0x%x Len:0x%x\n", dwStartAddr, dwLength);
    return(TRUE);
}


/*
    @func   void | OEMContinueEraseFlash | Called frequenty during image download, this routine continues the flash erase process.
    @rdesc  N/A.
    @comm    
    @xref   
*/
void OEMContinueEraseFlash(void)
{
    // Nothing to do (erase done in OEMWriteFlash)...
    //
    //EdbgOutputDebugString("OEMContinueEraseFlash\r\n");
}


/*
    @func   BOOL | OEMFinishEraseFlash | Called following the image download, this routine completes the flash erase process.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm    
    @xref   
*/
BOOL OEMFinishEraseFlash(void)
{
    // Nothing to do (erase done in OEMWriteFlash)...
    //
    //EdbgOutputDebugString("OEMFinishEraseFlash\r\n");
    return(TRUE);
}


/*
    @func   BOOL | OEMWriteFlash | Writes data to flash (the source location is determined using OEMMapMemAddr).
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm    
    @xref   
*/
BOOL OEMWriteFlash(DWORD dwStartAddr, DWORD dwLength)
{
    //EdbgOutputDebugString("OEMWriteFlash 0x%x 0x%x\r\n", dwStartAddr, dwLength);
    return(TRUE);
}

