/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    msmddoal.cpp

Abstract:

    Handles OAL requirements for Microsoft Mobile Devices.

--*/

#include <windows.h>
#include <bldver.h>

//#if ( (CE_MAJOR_VER == 0x0003)  && (CE_MINOR_VER == 0x0000) )
#include "pehdr.h" //by digibuff 09-Jul-2003
//#endif

#include "romxip.h"
#include <PSIIMDDOAL.h>
#include "romldr.h"
#include "pkfuncs.h"

//#include <PSII.h>
#include <PSIIROMUpdate.h>	// added by SHYoon. 10-MAR-2003

#ifndef IOCTL_HAL_GET_BIN_CHAIN

#define IOCTL_HAL_GET_BIN_CHAIN  CTL_CODE(FILE_DEVICE_HAL, 65, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif

//#include "guts.h"
//#include "ehm.h"

#define NOT_FIXEDUP		   (DWORD*)-1
#undef	PS_DEBUG

extern "C" {

    //
    // FIXUP variables that are set in config.bib
    // You MUST declare these in the OAL
    //

    extern DWORD * pdwXIPLoc;
    extern DWORD   dwDontUseChain;
    
    // kernel declares these:
    extern ROMChain_t *OEMRomChain;

};

/*****************************************************************************/
/* Debug Definitions                                                         */
/*****************************************************************************/
#define MSMDD_ERR_MSG_ON             1
#define MSMDD_LOG_MSG_ON             1
#define MSMDD_INF_MSG_ON             1

#define MSMDD_RTL_PRINT(x)           RETAILMSG(1, x)

#if MSMDD_ERR_MSG_ON
#define MSMDD_ERR_PRINT(x)           RETAILMSG(1, x)
#else
#define MSMDD_ERR_PRINT(x)        
#endif /* #if MSMDD_ERR_MSG_ON */

#if MSMDD_LOG_MSG_ON
#define MSMDD_LOG_PRINT(x)           RETAILMSG(1, x)
#else
#define MSMDD_LOG_PRINT(x)        
#endif  /* #if MSMDD_LOG_MSG_ON */

#if MSMDD_INF_MSG_ON
#define MSMDD_INF_PRINT(x)           RETAILMSG(1, x)
#else
#define MSMDD_INF_PRINT(x)        
#endif  /* #if MSMDD_INF_MSG_ON */

#if ( (CE_MAJOR_VER == 0x0004)  && (CE_MINOR_VER >= 0x0014) && (CE_BUILD_VER >= 13100) )	// 2004-0309 lek

HRESULT
DoGetBinExtensions(XIPCHAIN_SUMMARY *pChain,
                   LPDWORD           pdwSize);

///////////////////////////////////////////////////////////////////////////////

BOOL 
IsMSMDDIoControl(DWORD   dwIoControlCode, 
                 LPVOID  lpInBuf,         DWORD nInBufSize, 
                 LPVOID  lpOutBuf,        DWORD nOutBufSize, 
                 LPDWORD lpBytesReturned, BOOL *pRetVal) 
{
    HRESULT hr = NOERROR;
    BOOL    bRet = TRUE;

    switch (dwIoControlCode) 
    {
    case IOCTL_HAL_GET_BIN_CHAIN:
        if ( (pdwXIPLoc != NOT_FIXEDUP) )
        {
            hr = DoGetBinExtensions((XIPCHAIN_SUMMARY *)lpInBuf,
                                    (lpInBuf) ? &nInBufSize : (LPDWORD)lpOutBuf);
            bRet = SUCCEEDED(hr);
        }
        
        break;
    
    default:
            //
            // DON'T change pRetVal, just bail
            //
            return FALSE;
	}

    if ( !SUCCEEDED(hr) )
    {
        SetLastError(hr);
    }

    if ( pRetVal )
    {
        //
        // Note: only set pRetVal when we have handled the IOCTL
        //
        *pRetVal = bRet;
    }

    return bRet;
} 
 

HRESULT
GetOSChain(XIPCHAIN_SUMMARY **pOSChain, LPDWORD pdwNumEntries)
{
    const CHAR szChainDesc[] = "chain information";

    ROMHDR              *pToc = (ROMHDR *)UserKInfo[KINX_PTOC]; 
    ROMPID              *pRomPid;
    DWORD               dwChainDescLen = strlen(szChainDesc);

    if ( pToc->pExtensions ) 
    {
        //
        // grab the kernel chain from the extensions
        //

        pRomPid = (ROMPID *)pToc->pExtensions;
        ROMPID *pItem = (ROMPID *)pRomPid->pNextExt;
        while ( pItem )
        {
            if ( (memcmp( pItem->name, szChainDesc, dwChainDescLen) == 0) && pItem->length )
            {
                *pOSChain = (XIPCHAIN_SUMMARY *) pItem->pdata;
                *pdwNumEntries = pItem->length / sizeof(XIPCHAIN_SUMMARY);
                return S_OK;
            }
            pItem = (ROMPID *)pItem->pNextExt;
        }
    }

    return E_FAIL;
}

// ***************************************************************************
//
//  Function Name: VerifyChain
// 
//  Purpose: verifies MDD chain against kernel chain
//
//  Arguments:
//
//  Return Values:
//
//      FALSE if verification fails
//
//  Description:  
//
//      The MDD chain and the kernel chain should be identical in order
//      and regions.  They are different in the following:
//      1. The first region of the kernel chain is the MDD chain.
//      2. The kernel chain has entries consisting of a subset of the MDD chain fields
//          
//      This function verifies that the ordering and contents of the two
//      chains are in sync.
//
//

BOOL
VerifyChain(XIPCHAIN_SUMMARY *pExtChain, DWORD dwNumExtEntries)
{
    XIPCHAIN_ENTRY      *pMDDChain = (XIPCHAIN_ENTRY *)(pdwXIPLoc+1);

    if ( dwNumExtEntries != ((*pdwXIPLoc)+1) )
    {
        return FALSE;
    }

    // walk through chain to verify

    for ( UINT i = 1; i < dwNumExtEntries; ++i )
    {
        if ( pExtChain[i].pvAddr != pMDDChain[i-1].pvAddr )
        {
            return FALSE;
        }

    }

    return TRUE;
}

static DWORD*
AllocXIPLoc(void)
{
	DWORD *pMem;
	DWORD dwBlockStart;
	DWORD dwBlockLen;
	
	FlashInit(TRUE);
	FlashGetBlockInfo((LPVOID)pdwXIPLoc, &dwBlockStart, &dwBlockLen);

	pMem = (DWORD *) malloc(dwBlockLen * sizeof(BYTE));
	if (NULL == pMem)
	{
		return 0;
	}

	FlashRead((LPVOID)pdwXIPLoc, (LPVOID)pMem, dwBlockLen);

	return pMem;
}

static BOOL
FreeXIPLoc(DWORD *pMem)
{
	free(pMem);

	return TRUE;
}

#if defined(PS_DEBUG)
static void
ViewXIPChain(DWORD	dwNumXIPs, PXIPCHAIN_ENTRY pEntry)
{
	UINT  i, j;
	wchar_t szXIPName[XIP_NAMELEN];

	NKDbgPrintfW(TEXT("dwNumXIPs       = %d\r\n"), dwNumXIPs);
	for (i = 0; i < dwNumXIPs; i ++)
	{
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  REGION[%d]\r\n"), i);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pvAddr[%d].pvAddr      = 0x%X\r\n"), i, (DWORD) pEntry[i].pvAddr);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].dwLength    = 0x%X\r\n"), i, (DWORD) pEntry[i].dwLength);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].dwMaxLength = 0x%X\r\n"), i, (DWORD) pEntry[i].dwMaxLength);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].usOrder     = 0x%X\r\n"), i, (USHORT) pEntry[i].usOrder);
    	MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].usFlags     = 0x%X\r\n"), i, (USHORT) pEntry[i].usFlags);
    	MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].dwVersion   = 0x%X\r\n"), i, (DWORD) pEntry[i].dwVersion);

		for (j = 0; j < XIP_NAMELEN; j ++)
			szXIPName[j] = (TCHAR) (pEntry[i].szName[j]);

    	MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].szName      = %s\r\n"),   i, szXIPName);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].dwAlgoFlags = 0x%x\r\n"), i, (DWORD) pEntry[i].dwAlgoFlags);
		MSMDD_RTL_PRINT(TEXT("[MSMDD:   ]  \t pEntry[%d].dwKeyLen    = 0x%x\r\n"), i, (DWORD) pEntry[i].dwKeyLen);
	}
}
#endif	//(PS_DEBUG)

BOOL
VerifyChainInPocketStore(XIPCHAIN_SUMMARY *pExtChain, DWORD dwNumExtEntries)
{
    XIPCHAIN_ENTRY      *pMDDChain;
    DWORD				dwNumOfXIPLoc;
    DWORD				*pNANDXIPLoc;


    MSMDD_LOG_PRINT((TEXT("[MSMDD: IN] ++VerifyChainInPocketStore()\r\n")));

	pNANDXIPLoc = AllocXIPLoc();
	if (0 == pNANDXIPLoc)
	{
		MSMDD_ERR_PRINT((TEXT("[MSMDD:ERR]  FATAL ERROR: AllocXIPLoc fail!!!!\r\n")));
        MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --VerifyChainInPocketStore()\r\n")));
		return FALSE;
	}

    pMDDChain = (XIPCHAIN_ENTRY *)(pNANDXIPLoc + 1);
    dwNumOfXIPLoc = (*pNANDXIPLoc) + 1;

#if defined(PS_DEBUG)
	ViewXIPChain((DWORD) *pNANDXIPLoc, pMDDChain);
#endif	//(PS_DEBUG)	

    if ( dwNumExtEntries != (dwNumOfXIPLoc) )
    {
    	FreeXIPLoc(pNANDXIPLoc);
    	
    	MSMDD_ERR_PRINT((TEXT("[MSMDD:ERR]  VerifyChain(dwNumExtEntries=%d,dwNumOfXIPLoc=%d)\r\n"),
    		             dwNumExtEntries, dwNumOfXIPLoc));
        MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --VerifyChainInPocketStore()\r\n")));
        return FALSE;
    }

    // walk through chain to verify

    for ( UINT i = 1; i < dwNumExtEntries; ++i )
    {
        if ( pExtChain[i].pvAddr != pMDDChain[i-1].pvAddr )
        {
        	FreeXIPLoc(pNANDXIPLoc);
        	
	    	MSMDD_ERR_PRINT((TEXT("[MSMDD:OUT]  pExtChain[%d].pvAddr=(%X),pMDDChain[%d].pvAddr=(%X)\r\n"),
    			             i, pExtChain[i].pvAddr, i-1, pMDDChain[i-1].pvAddr));
    	    MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --VerifyChainInPocketStore()\r\n")));
            return FALSE;
        }
    }

	FreeXIPLoc(pNANDXIPLoc);
    
    MSMDD_INF_PRINT((TEXT("[MSMDD:MSG]  VerifyChainInPocketStore OK\r\n")));
    MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --VerifyChainInPocketStore()\r\n")));
    return TRUE;
}


// ***************************************************************************
//
//  Function Name: DoGetBinExtensions
// 
//  Purpose: tells BINFS what regions to load
//
//  Arguments:
//      IN        ppRomPid     XIPSUMMARY chain
//
//  Return Values:
//
//      E_OUTOFMEMORY   
//
//
//  Description:  
//
//      BINFS calls this at init to get the XIP chain.  
//      We walk the MDD chain and put regions into the BINFS chain
//      accordingly to the flags settings.
//
//      One pain is that the BINFS chain structure is different than the MDD chain.
//      BINFS chain is an array of XIPCHAIN_SUMMARY structures
//      MDD chain is an array of XIPCHAIN_ENTRY structures
//
//

HRESULT
DoGetBinExtensions(XIPCHAIN_SUMMARY   *pExtChain,
                   LPDWORD             pdwSize)
{
    DEBUGCHK(pdwXIPLoc != NOT_FIXEDUP);
    DEBUGCHK(pdwSize);

    PXIPCHAIN_ENTRY     pMDDChain;
    XIPCHAIN_SUMMARY *  pOSChain;
    DWORD               dwNumOSEntries, dwCurEntry;
    HRESULT             hr;
    DWORD				*pNANDXIPLoc;

    MSMDD_LOG_PRINT((TEXT("[MSMDD: IN] ++DoGetBinExtensions()\r\n")));
    
    hr = GetOSChain(&pOSChain, &dwNumOSEntries);
    if ( SUCCEEDED(hr) )
    {
#if	1
    	{
    		UINT i;

            MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  ====== pOSChain Info Print ======\r\n")));
        
    		for (i = 0; i < dwNumOSEntries; i ++)
    		{
    			MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  \tpOSChain[%d].pvAddr       =0x%X\r\n"),   i, pOSChain[i].pvAddr));
    			MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  \tpOSChain[%d].dwMaxLength  =0x%X\r\n"),   i, pOSChain[i].dwMaxLength));
    			MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  \tpOSChain[%d].usOrder      =0x%X\r\n"),   i, pOSChain[i].usOrder));
    			MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  \tpOSChain[%d].usFlags      =0x%X\r\n"),   i, pOSChain[i].usFlags));
    		}
    		MSMDD_INF_PRINT((TEXT("[MSMDD:   ]  =================================\r\n")));
    	}
#endif
        //
        // NOTE: we always put a copy of the MDD chain into the BINFS chain
        //  that's just the way BINFS expects it, it always skips the first
        //  entry assuming it's our chain bin
        //

        if ( pExtChain )
        {
            memcpy(pExtChain, pOSChain, sizeof(pExtChain[0]));
        }
        dwCurEntry = 1;
    }
    else
    {
        MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --DoGetBinExtensions()\r\n")));
        return hr;
    }

#if defined(_ORG_CODE_)
	if ( !VerifyChain(pOSChain, dwNumOSEntries) )
#else	//_ORG_CODE_
    if ( !VerifyChainInPocketStore(pOSChain, dwNumOSEntries) )
#endif	//_ORG_CODE_
    {
        //
        // the chains are inconsistent-- the kernel chain that's 
        // being used is not in sync with the MDD chain
        // we don't want the system to boot with any regions
        //
        DEBUGCHK(0);

        MSMDD_ERR_PRINT((TEXT("[MSMDD:ERR]  MDD Chain and OS chain are not matched!!!!\r\n")));
        MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --DoGetBinExtensions()\r\n")));
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }

	pNANDXIPLoc = AllocXIPLoc();

	if (0 == pNANDXIPLoc)
	{
		MSMDD_ERR_PRINT((TEXT("[MSMDD:ERR]  AllocXIPLoc fail\r\n")));
		MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --DoGetBinExtensions()\r\n")));
		return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
	}
#if defined(_ORG_CODE_)
	pMDDChain = (PXIPCHAIN_ENTRY)(pdwXIPLoc + 1);
#else	//_ORG_CODE_
    pMDDChain = (PXIPCHAIN_ENTRY)(pNANDXIPLoc + 1);
#endif	//_ORG_CODE_


    for ( UINT i = 1; i < dwNumOSEntries; ++i )
    {
        if ( pMDDChain[i-1].usFlags & ROMXIP_OK_TO_LOAD )
        {
            if ( pExtChain )
            {
                memcpy(&(pExtChain[dwCurEntry]), &(pOSChain[i]), sizeof(pExtChain[dwCurEntry]));
				pExtChain[dwCurEntry].usOrder = (USHORT)(dwCurEntry-1); // BINFS counts on this field
            }

            ++dwCurEntry;
        }
    }

    if ( !pExtChain )
    {
        *pdwSize = dwCurEntry * sizeof(XIPCHAIN_SUMMARY);
    }

    FreeXIPLoc(pNANDXIPLoc);

    MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --DoGetBinExtensions()\r\n")));
    return S_OK;
}

#endif // (CE_MAJOR_VER == 0x0004)  && (CE_MINOR_VER == 0x0014) && (CE_BUILD_VER >= 13100)

// ***************************************************************************
//
//  Function Name: MSMDDInit
// 
//  Purpose: called at OEMInit for MSMDD platforms.  Does OEMInit type stuff for MDD.
//
//  Description:  
//
//      List of things that this function will do per MDD requirements:
//      1. Writes OEMRomChain for multixip images except when BLOCKROM/BINFS is used
//      2. initializes paging pool if specified in config.bib
//
//

BOOL
MSMDDInit()
{
    static		ROMChain_t	s_pNextRom[MAX_ROM];
    DWORD		dwRomCount = 0;
    DWORD       dwChainCount = 0;
    DWORD *     pdwCurXIP;
    DWORD       dwNumXIPs;

    PXIPCHAIN_ENTRY pChainEntry = NULL;
    
	MSMDD_LOG_PRINT((TEXT("[MSMDD: IN] ++MSMDDInit()\r\n")));

	// Added for MultiXIP stuff
    if ( pdwXIPLoc != NOT_FIXEDUP && dwDontUseChain == (DWORD)NOT_FIXEDUP )
    {
        MSMDD_INF_PRINT((TEXT("pdwXIPLoc fixed up correctly\n")));
    
        MSMDD_INF_PRINT((TEXT("pdwXIPLoc = 0x%x\n"), pdwXIPLoc));
		
        // Set the top bit to mark it a virtual address
        pdwCurXIP = (DWORD*)(((DWORD)pdwXIPLoc) | 0x80000000 );
    
        MSMDD_INF_PRINT((TEXT("pdwCurXIP = 0x%x\n"), pdwCurXIP));
		
        //
        // first DWORD is the number of XIPs
        //
        dwNumXIPs = *pdwCurXIP;
		
        MSMDD_INF_PRINT((TEXT("dwNumXIPs = 0x%x\n"), dwNumXIPs));
		
        if ( dwNumXIPs > MAX_ROM )
        {
            MSMDD_INF_PRINT((TEXT("ERROR: Number of XIPs exceeds MAX\n")));
        }
        else
        {
            pChainEntry = (PXIPCHAIN_ENTRY)(pdwCurXIP + 1);
            while (dwChainCount < dwNumXIPs)
            {
                if ((pChainEntry->usFlags & ROMXIP_OK_TO_LOAD) &&  // flags indicates valid XIP
                    *(LPDWORD)(((DWORD)(pChainEntry->pvAddr)) + ROM_SIGNATURE_OFFSET) == ROM_SIGNATURE)
                {

                    s_pNextRom[dwRomCount].pTOC = *(ROMHDR **)(((DWORD)(pChainEntry->pvAddr)) + ROM_SIGNATURE_OFFSET + 4);
                    s_pNextRom[dwRomCount].pNext = NULL;
    
                    MSMDD_INF_PRINT((TEXT("[MSMDD:INF]  XIP region found: %a\r\n"), pChainEntry->szName));
    
                    if (dwRomCount != 0)
                    {
                        s_pNextRom[dwRomCount-1].pNext = &s_pNextRom[dwRomCount];
                    }
                    else
                    {
                        OEMRomChain = s_pNextRom;
                    }
                    dwRomCount++;
                }
                else
                {
                    MSMDD_ERR_PRINT((TEXT("[MSMDD:ERR]  Invalid XIP found\n")));
                }
    
                ++pChainEntry;
                dwChainCount++;
            }
        }
    }
    else
    {
        MSMDD_INF_PRINT((TEXT("[MSMDD:INF]  pdwXIPLoc=0x%x, dwDontUseChain=0x%x\r\n"), pdwXIPLoc, dwDontUseChain));
        MSMDD_INF_PRINT((TEXT("[MSMDD:INF]  XIP chain location not fixed up\r\n")));
    }

	// end MultiXIP stuff

	MSMDD_LOG_PRINT((TEXT("[MSMDD:OUT] --MSMDDInit()\r\n")));

    return TRUE;
}
