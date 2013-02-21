//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
//------------------------------------------------------------------------------
//
//  File:  intr.h
//
//  This file contains SMDK6410 board specific interrupt code. 
//
#include <bsp.h>

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrInit
//
BOOL BSPIntrInit()
{
    volatile S3C6410_GPIO_REG * pGPIOReg = (S3C6410_GPIO_REG *)OALPAtoVA(S3C6410_BASE_REG_PA_GPIO, FALSE);
    OALMSG(OAL_INTR&&OAL_FUNC, (L"[OAL] ++BSPIntrInit()\r\n"));

    //---------------------
    // Static SYSINTR mapping
    //---------------------

    // Add static mapping for Built-In OHCI
    OALIntrStaticTranslate(SYSINTR_OHCI, IRQ_UHOST);        // for USB Host 1.1
	// SYSINTR_DM9000 A1: GPN0
	RETAILMSG(1,(TEXT("BSPIntrInit: SYSINTR_DM9000A1 High level triggered \r\n")));
    pGPIOReg->GPNCON &= ~(0x3<<14);
	pGPIOReg->GPNCON |=  (0x2<<14);

	RETAILMSG(1, (TEXT("******DM9000A1 pGPIOReg->GPNCON=%x\r\n "),pGPIOReg->GPNCON));
	
	pGPIOReg->EINT0CON0 &= ~(0x7<<12);
	pGPIOReg->EINT0CON0 |=  (0x1<<12);//High level triggered
    RETAILMSG(1, (TEXT("******DM9000A1 	pGPIOReg->EINT0CON0 =%x\r\n "),	pGPIOReg->EINT0CON0));
	pGPIOReg->EINT0MASK &= ~(0x1<<IRQ_EINT7);
	pGPIOReg->EINT0PEND |=  (0x1<<IRQ_EINT7);
	RETAILMSG(1, (TEXT("******DM9000A1 pGPIOReg->EINT0MASK  =%x\r\n "),	pGPIOReg->EINT0MASK));
    RETAILMSG(1, (TEXT("******DM9000A1 pGPIOReg->EINT0PEND  =%x\r\n "), pGPIOReg->EINT0PEND));
	OALIntrStaticTranslate(SYSINTR_DM9000A1, IRQ_EINT7);
    OALMSG(OAL_INTR&&OAL_FUNC, (L"[OAL] --BSPIntrInit()\r\n"));

    return TRUE;
}

//------------------------------------------------------------------------------

BOOL BSPIntrRequestIrqs(DEVICE_LOCATION *pDevLoc, UINT32 *pCount, UINT32 *pIrqs)
{
    BOOL bRet = FALSE;

    OALMSG(OAL_INTR&&OAL_FUNC, (L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
                            pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
                            pDevLoc->Pin, pCount, pIrqs));
    OALMSG(TRUE, (L"+BSPIntrRequestIrq(0x%08x->%d/%d/0x%08x/%d, 0x%08x, 0x%08x)\r\n",
                            pDevLoc, pDevLoc->IfcType, pDevLoc->BusNumber, pDevLoc->LogicalLoc,
                            pDevLoc->Pin, pCount, pIrqs));
    

    if (pIrqs == NULL
        || pCount == NULL
        || *pCount < 1)
    {
        goto cleanUp;
    }
    else
    {
        switch (pDevLoc->IfcType)
        {
        case Internal:
            switch ((ULONG)pDevLoc->LogicalLoc)
            {
                case BSP_BASE_REG_PA_CS8900A_IOBASE:
                    pIrqs[0] = IRQ_EINT10;
                    *pCount = 1;
                    bRet = TRUE;
                    break;
                    
                case S3C6410_BASE_REG_PA_USBOTG_LINK:
                    pIrqs[0] = IRQ_OTG;
                    *pCount = 1;
                    bRet = TRUE;
                    break;                    
            }
            break;
        }
    }

cleanUp:

    OALMSG(OAL_INTR&&OAL_FUNC, (L"-BSPIntrRequestIrq(rc = %d)\r\n", bRet));
    return bRet;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrEnableIrq
//
//  This function is called from OALIntrEnableIrq to enable interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrEnableIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrEnableIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrEnableIrq(irq = %d)\r\n", irq));
    return irq;
}

//------------------------------------------------------------------------------
//
//  Function:  BSPIntrDisableIrq
//
//  This function is called from OALIntrDisableIrq to disable interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrDisableIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrDisableIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrDisableIrq(irq = %d\r\n", irq));
    return irq;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrDoneIrq
//
//  This function is called from OALIntrDoneIrq to finish interrupt on
//  secondary interrupt controller.
//
UINT32 BSPIntrDoneIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrDoneIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrDoneIrq(irq = %d)\r\n", irq));
    return irq;
}


//------------------------------------------------------------------------------
//
//  Function:  BSPIntrActiveIrq
//
//  This function is called from interrupt handler to give BSP chance to
//  translate IRQ in case of secondary interrupt controller.
//
UINT32 BSPIntrActiveIrq(UINT32 irq)
{
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"+BSPIntrActiveIrq(%d)\r\n", irq));
    OALMSG(OAL_INTR&&OAL_VERBOSE, (L"-BSPIntrActiveIrq(%d)\r\n", irq));
    return irq;
}

//------------------------------------------------------------------------------

