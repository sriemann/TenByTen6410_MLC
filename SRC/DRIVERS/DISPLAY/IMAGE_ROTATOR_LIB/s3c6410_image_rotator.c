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
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    s3c6410_image_rotator.c

Abstract:       Implementation of Image Rotator Control Library
                This module implements Low Level HW control 

Functions:


Notes:


--*/

#include <windows.h>
#include <bsp_cfg.h>
#include <s3c6410.h>
#include "s3c6410_image_rotator.h"
#include "s3c6410_image_rotator_macro.h"

#define ROT_MSG(x)
#define ROT_INF(x)
#define ROT_ERR(x)    RETAILMSG(TRUE, x)

static volatile S3C6410_ROTATOR_REG *g_pRotatorReg = NULL;

ROT_ERROR Rotator_initialize_register_address(void *pRotatorReg)
{
    ROT_ERROR error = ROT_SUCCESS;

    ROT_MSG((_T("[ROT] ++Rotator_initialize_register_address(0x%08x)\n\r"), pRotatorReg));

    if (pRotatorReg == NULL)
    {
        ROT_ERR((_T("[ROT:ERR] Rotator_initialize_register_address() : NULL pointer parameter\n\r")));
        error = ROT_ERROR_NULL_PARAMETER;
    }
    else
    {
        g_pRotatorReg = (S3C6410_ROTATOR_REG *)pRotatorReg;
        ROT_INF((_T("[ROT:INF] g_pRotatorReg = 0x%08x\n\r"), g_pRotatorReg));
    }

    ROT_MSG((_T("[ROT] --Rotator_initialize_register_address() : %d\n\r"), error));

    return error;
}

ROT_ERROR Rotator_initialize(ROTATOR_IMAGE_FORMAT eFormat, ROTATOR_OPERATION_TYPE eOperation, unsigned int uiSrcWidth, unsigned int uiSrcHeight)
{
    ROT_ERROR error = ROT_SUCCESS;

    ROT_MSG((_T("[ROT] ++Rotator_initialize(%d, %d, %d, %d)\n\r"), eFormat, eOperation, uiSrcWidth, uiSrcHeight));

    g_pRotatorReg->CTRLCFG = g_pRotatorReg->CTRLCFG & INT_ENABLE;        // Keep Interrupt Enable Bit and Clear Others

    switch(eFormat)
    {
    case ROT_FORMAT_YUV420:
        g_pRotatorReg->CTRLCFG |= IMG_FORMAT_YUV420;
        break;
    case ROT_FORMAT_YUV422:
        g_pRotatorReg->CTRLCFG |= IMG_FORMAT_YUV422;
        break;
    case ROT_FORMAT_RGB565:
        g_pRotatorReg->CTRLCFG |= IMG_FORMAT_RGB565;
        break;
    case ROT_FORMAT_RGB888:
        g_pRotatorReg->CTRLCFG |= IMG_FORMAT_RGB888;
        break;
    default:
        ROT_ERR((_T("[ROT:ERR] Rotator_initialize() : Unknown Image Foramt [%d]\n\r"), eFormat));
        error = ROT_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    switch(eOperation)
    {
    case ROT_OP_ROTATE_90:
        g_pRotatorReg->CTRLCFG |= ROTATE_DEGREE_90;
        break;
    case ROT_OP_ROTATE_180:
        g_pRotatorReg->CTRLCFG |= ROTATE_DEGREE_180;
        break;
    case ROT_OP_ROTATE_270:
        g_pRotatorReg->CTRLCFG |= ROTATE_DEGREE_270;
        break;
    case ROT_OP_FLIP_VERTICAL:
        g_pRotatorReg->CTRLCFG |= FLIP_VERTICAL;
        break;
    case ROT_OP_FLIP_HORIZONTAL:
        g_pRotatorReg->CTRLCFG |= FLIP_HORIZONTAL;
        break;
    default:
        ROT_ERR((_T("[ROT:ERR] Rotator_initialize() : Unknown Operation Type [%d]\n\r"), eOperation));
        error = ROT_ERROR_ILLEGAL_PARAMETER;
        break;
    }

    g_pRotatorReg->SrcSizeReg = VERTICAL_SIZE(uiSrcHeight) | HORIZONTAL_SIZE(uiSrcWidth);

    ROT_MSG((_T("[ROT] --Rotator_initialize() : %d\n\r"), error));

    return error;
}

ROT_ERROR Rotator_set_source_buffer(unsigned int AddrRGBY, unsigned int AddrCb, unsigned int AddrCr)
{
    ROT_ERROR error = ROT_SUCCESS;

    ROT_MSG((_T("[ROT] ++Rotator_set_source_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrRGBY, AddrCb, AddrCr));

    if ((AddrRGBY % 0x4)
        || (AddrCb % 0x4)
        || (AddrCr % 0x4))
    {
        ROT_ERR((_T("[ROT:ERR] Rotator_set_source_buffer() : Source Address is Not Aligned [0x%08x, 0x%08x, 0x%08x]\n\r"), AddrRGBY, AddrCb, AddrCr));
        error = ROT_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        g_pRotatorReg->SrcAddrReg0 = ADDRESS_RGB_Y(AddrRGBY);
        g_pRotatorReg->SrcAddrReg1 = ADDRESS_RGB_Y(AddrCb);
        g_pRotatorReg->SrcAddrReg2 = ADDRESS_RGB_Y(AddrCr);
    }

    ROT_MSG((_T("[ROT] --Rotator_set_source_buffer() : %d\n\r"), error));

    return error;
}

ROT_ERROR Rotator_set_destination_buffer(unsigned int AddrRGBY, unsigned int AddrCb, unsigned int AddrCr)
{
    ROT_ERROR error = ROT_SUCCESS;

    ROT_MSG((_T("[ROT] ++Rotator_set_destination_buffer(0x%08x, 0x%08x, 0x%08x)\n\r"), AddrRGBY, AddrCb, AddrCr));

    if ((AddrRGBY % 0x4)
        || (AddrCb % 0x4)
        || (AddrCr % 0x4))
    {
        ROT_ERR((_T("[ROT:ERR] Rotator_set_destination_buffer() : Destination Address is Not Aligned [0x%08x, 0x%08x, 0x%08x]\n\r"), AddrRGBY, AddrCb, AddrCr));
        error = ROT_ERROR_ILLEGAL_PARAMETER;
    }
    else
    {
        g_pRotatorReg->DstAddrReg0 = ADDRESS_RGB_Y(AddrRGBY);
        g_pRotatorReg->DstAddrReg1 = ADDRESS_RGB_Y(AddrCb);
        g_pRotatorReg->DstAddrReg2 = ADDRESS_RGB_Y(AddrCr);
    }

    ROT_MSG((_T("[ROT] --Rotator_set_destination_buffer() : %d\n\r"), error));

    return error;
}

void Rotator_start(void)
{
    ROT_MSG((_T("[ROT] Rotator_start()\n\r")));

    g_pRotatorReg->CTRLCFG |= ROTATOR_START;
}

void Rotator_stop(void)
{
    ROT_MSG((_T("[ROT] Rotator_stop()\n\r")));

    g_pRotatorReg->CTRLCFG &= ~ROTATOR_START;
}

void Rotator_enable_interrupt(void)
{
    ROT_MSG((_T("[ROT] Rotator_enable_interrupt()\n\r")));

    g_pRotatorReg->CTRLCFG |= INT_ENABLE;
}

void Rotator_disable_interrupt(void)
{
    ROT_MSG((_T("[ROT] Rotator_disable_interrupt()\n\r")));

    g_pRotatorReg->CTRLCFG &= ~INT_ENABLE;
}

BOOL Rotator_clear_interrupt_pending(void)
{
    unsigned int uiStatusValue;

    ROT_MSG((_T("[ROT] Rotator_clear_interrupt_pending()\n\r")));

    uiStatusValue = g_pRotatorReg->STATCFG;

    if (uiStatusValue & INT_PENDING)
    {
        return TRUE;        // Interrupt Pended and Cleared
    }
    else
    {
        return FALSE;    // Interrupt Not Pended
    }
}

ROT_ERROR Rotator_get_status(tRotatorStatus *pStatus)
{
    unsigned int uiStatusValue;

    ROT_ERROR error = ROT_SUCCESS;

    ROT_MSG((_T("[ROT] ++Rotator_get_status(0x%08x)\n\r"), pStatus));

    if (pStatus == NULL)
    {
        ROT_ERR((_T("[ROT:ERR] Rotator_get_status() : Null Parameter for pStatus\n\r")));
        error = ROT_ERROR_NULL_PARAMETER;
    }
    else
    {
        uiStatusValue = g_pRotatorReg->STATCFG;

        pStatus->uiCurLineNumber = CUR_LINE_MASK(uiStatusValue);

        if (uiStatusValue & INT_PENDING)
        {
            pStatus->bIntPending = TRUE;
        }
        else
        {
            pStatus->bIntPending = FALSE;
        }

        pStatus->eOpStatus = uiStatusValue & (ROTATOR_BUSY | ROTATOR_BUSY2);
    }

    ROT_MSG((_T("[ROT] --Rotator_get_status() : %d\n\r"), error));

    return error;
}

