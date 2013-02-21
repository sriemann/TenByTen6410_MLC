//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
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

*/


#ifndef __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__
#define __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__

#include <image_cfg.h>

// Physical Base Address for the MFC Host I/F Registers
#define S3C6410_BASEADDR_MFC_SFR                (0x7e002000)

// Physical Base Address for the MFC Shared Buffer
//   Shared Buffer = {CODE_BUF, WORK_BUF, PARA_BUF}
#define S3C6410_BASEADDR_MFC_BITPROC_BUF    (IMAGE_MFC_BUFFER_PA_START)    //0x56000000    // 0x57800000

// Physical Base Address for the MFC Data Buffer
//   Data Buffer = {STRM_BUF, FRME_BUF}
#define S3C6410_BASEADDR_MFC_DATA_BUF        (IMAGE_MFC_BUFFER_PA_START+0x116000)    // 0x56116000    // 0x57916000

// Physical Base Address for the MFC Host I/F Registers
#define S3C6410_BASEADDR_POST_SFR            (0x77000000)


//////////////////////////////////
/////                        /////
/////    MFC BITPROC_BUF     /////
/////                        /////
//////////////////////////////////
//the following three buffers have fixed size
//firware buffer is to download boot code and firmware code
#define MFC_CODE_BUF_SIZE    81920    // It is fixed depending on the MFC'S FIRMWARE CODE (refer to 'Prism_S_V133.h' file)

//working buffer is uded for video codec operations by MFC
#define MFC_WORK_BUF_SIZE    1048576 // 1024KB = 1024 * 1024


//Parameter buffer is allocated to store yuv frame address of output frame buffer.
#define MFC_PARA_BUF_SIZE    8192  //Stores the base address of Y , Cb , Cr for each decoded frame

#define MFC_BITPROC_BUF_SIZE        \
                            (    MFC_CODE_BUF_SIZE + \
                                MFC_PARA_BUF_SIZE    + \
                                MFC_WORK_BUF_SIZE)


///////////////////////////////////
/////                         /////
/////      MFC DATA_BUF       /////
/////                         /////
///////////////////////////////////
#define MFC_NUM_INSTANCES_MAX    2    // MFC Driver supports 4 instances MAX.

// Determine if 'Post Rotate Mode' is enabled.
// If it is enabled, the memory size of SD YUV420(720x576x1.5 bytes) is required more.
#define MFC_ROTATE_ENABLE        1



/*
 * stream buffer size must be a multiple of 512bytes 
 * becasue minimun data transfer unit between stream buffer and internal bitstream handling block 
 * in MFC core is 512bytes
 */
#define MFC_LINE_BUF_SIZE_PER_INSTANCE        (614400)//(211968)
#if (MFC_LINE_BUF_SIZE_PER_INSTANCE & 0xFFF)
#error "In WinCE6.0, MFC_LINE_BUF_SIZE_PER_INSTANCE value must be 4K byte aligned."
#endif

#define MFC_LINE_BUF_SIZE        (MFC_NUM_INSTANCES_MAX * MFC_LINE_BUF_SIZE_PER_INSTANCE)
#define MFC_FRAM_BUF_SIZE        (720*480*3*4 + 2048)  // page aligned


#define MFC_STRM_BUF_SIZE        (MFC_LINE_BUF_SIZE)
#define MFC_DATA_BUF_SIZE        (MFC_STRM_BUF_SIZE + MFC_FRAM_BUF_SIZE)

/*
 * MFC Interrupt Enable Macro Definition
 */
#define MFC_INTR_ENABLE_ALL    0xCCFF
#define MFC_INTR_ENABLE_RESET    0xC00E


#endif /* __SAMSUNG_SYSLSI_APDEV_MFC_CONFIG_H__ */
