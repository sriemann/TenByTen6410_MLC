#include <windows.h>
#include <oal.h>

#include "image_cfg.h"
#include "bsp_args.h"
#include "bsp_cfg.h"

#include "s3c6410_base_regs.h"

// SoC Components
#include "s3c6410_syscon.h"
//------------------------------------------------------------------------------
//
//  Function:  OALArgsInit
//
//  This function is called by other OAL modules to intialize value of argument
//  structure.
//
VOID OALArgsInit(BSP_ARGS* pArgs)
{
    int i;
    OALMSG(OAL_FUNC, (TEXT("+OALArgsInit()\r\n")));

    // Check the BSP Args area
    //
    if ((pArgs->header.signature  == OAL_ARGS_SIGNATURE)
        || (pArgs->header.oalVersion == OAL_ARGS_VERSION)
        || (pArgs->header.bspVersion == BSP_ARGS_VERSION))
    {
        OALMSG(OAL_INFO, (TEXT("Arguments area has some values. Do not Initialize\r\n")));
        OALMSG(OAL_VERBOSE, (TEXT("pArgs :0x%x\r\n"), pArgs));
        for(i=0;i<sizeof(BSP_ARGS); i++)
        {
            OALMSG(OAL_VERBOSE, (TEXT("0x%02x "),*((UINT8*)pArgs+i)));
        }
    }
    else
    {
        volatile S3C6410_SYSCON_REG *pSysConReg;
        DWORD count, code, j;
        UCHAR d;

        pSysConReg = (S3C6410_SYSCON_REG *)OALPAtoUA(S3C6410_BASE_REG_PA_SYSCON);

        memset(pArgs, 0x0, sizeof(BSP_ARGS));

        // Setup header
        pArgs->header.signature = OAL_ARGS_SIGNATURE;
        pArgs->header.oalVersion = OAL_ARGS_VERSION;
        pArgs->header.bspVersion = BSP_ARGS_VERSION;

        //Set-up device ID for SMDK6410
        count = sizeof(BSP_DEVICE_PREFIX) - 1;            // BSP_DEVICE_PREFIX = "SMDK6410" defined in bsp_cfg.h

        if (count > sizeof(pArgs->deviceId)/2) count = sizeof(pArgs->deviceId)/2;
        memcpy(pArgs->deviceId, BSP_DEVICE_PREFIX, count);

        code = pSysConReg->SYS_ID;
        OALMSG(TRUE, (TEXT("SocID:0x%x\n"),code, sizeof(pArgs->deviceId)));

        // Convert it to hex number
        // 36410101 -> extract 6410101, means 6410 EVT1 
        for (j = 28; j >= 0 && count < sizeof(pArgs->deviceId); j -= 4)
        {
            d = (UCHAR)((code >> j) & 0xF);
            pArgs->deviceId[count++] = ((d < 10) ? ('0' + d) : ('A' + d - 10));
        }

        // End string will be "SMDK6410641010x"
        while (count < sizeof(pArgs->deviceId)) pArgs->deviceId[count++] = '\0';

        count = 0;
        // Set-up dummy uuid for SMDK6410.
        // Actually, S3C6410 does not provide UUID for each chip
        // So on S3C6410 EVT1, uuid will be 3641010136410101
        for(j=60; j> 0 && count < sizeof(pArgs->uuid); j -=4)
        {   
            d = (UCHAR)(((code >> (j%32))) & 0xF);
            pArgs->uuid[count++] = d < 10 ? '0' + d : 'A' + d - 10;
        }
        // Can Add code for cleanboot, hiveclean, formatpartion
        OALMSG(TRUE, (TEXT("Arguments area is initialized\r\n")));
    }
	// as Hiteg values are set when TOC get's initialized....we don't use this init. as it's called from INIT.c and resets everything to defaults.... 
    OALMSG(TRUE, (TEXT("-OALArgsInit()\r\n")));

    return;
}

//------------------------------------------------------------------------------
//
//  Function:  OALArgsQuery
//
//  This function is called from other OAL modules to return boot arguments.
//  Boot arguments are typically placed in fixed memory location and they are
//  filled by boot loader. In case that boot arguments can't be located
//  the function should return NULL. The OAL module then must use default
//  values.
//
VOID* OALArgsQuery(UINT32 type)
{
    VOID *pData = NULL;
    BSP_ARGS *pArgs;

    OALMSG(OAL_ARGS&&OAL_FUNC, (L"+OALArgsQuery(%d)\r\n", type));

    // Get pointer to expected boot args location
    pArgs = (BSP_ARGS*)IMAGE_SHARE_ARGS_UA_START;

    // Check if there is expected signature
    if ((pArgs->header.signature  == OAL_ARGS_SIGNATURE)
        || (pArgs->header.oalVersion == OAL_ARGS_VERSION)
        || (pArgs->header.bspVersion == BSP_ARGS_VERSION))
    {
        // Depending on required args
        switch (type)
        {
        case OAL_ARGS_QUERY_DEVID:
            pData = &pArgs->deviceId;
            break;
        case OAL_ARGS_QUERY_UUID:
            pData = &pArgs->uuid;
            break;
        case OAL_ARGS_QUERY_KITL:
            pData = &pArgs->kitl;
            break;
        case BSP_ARGS_QUERY_HIVECLEAN:
            pData = &pArgs->bHiveCleanFlag;
            break;
        case BSP_ARGS_QUERY_CLEANBOOT:
            pData = &pArgs->bCleanBootFlag;
            break;
        case BSP_ARGS_QUERY_FORMATPART:
            pData = &pArgs->bFormatPartFlag;
            break;
		case BSP_ARGS_QUERY_POWERCTL:
			pData = &pArgs->powerCTL;
			break;
		case BSP_ARGS_QUERY_DISPLAYTYPE:
			pData = &pArgs->displayType;
			break;
		case BSP_ARGS_QUERY_FRAMEBUFFERDEPTH:
			pData = &pArgs->framebufferDepth;
			break;
		case BSP_ARGS_QUERY_BACKGROUNDCOLOR:
			pData = &pArgs->backgroundColor;
			break;
		case BSP_ARGS_QUERY_LOGOHW:
			pData = &pArgs->logoHW;
			break;
		case BSP_ARGS_QUERY_DEBUGUART:
			pData = &pArgs->debugUART;
		break;

        default:
            break;
        }
    }
    else
    {
        OALMSG(TRUE, (TEXT("[OAL:ERR] Arguments area is invalid\r\n")));
    }

    OALMSG(OAL_ARGS&&OAL_FUNC, (L"-OALArgsQuery(pData = 0x%08x)\r\n", pData));

    return pData;
}

//------------------------------------------------------------------------------

