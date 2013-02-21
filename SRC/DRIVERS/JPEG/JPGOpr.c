//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/


#include "JPGMem.h"
#include "JPGMisc.h"
#include "JPGOpr.h"
#include "JPGConf.h"


enum
{
    UNKNOWN,
    BASELINE = 0xC0,
    EXTENDED_SEQ = 0xC1,
    PROGRESSIVE = 0xC2
} JPG_SOF_MARKER;


/*----------------------------------------------------------------------------
*Function: decodeJPG

*Parameters:    jCTX:
                input_buff:
                input_size:
                output_buff:
                output_size
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
JPG_RETURN_STATUS decodeJPG(S3C6410_JPG_CTX *jCTX,  JPG_DEC_PROC_PARAM *decParam)
{
    int        ret;
    SAMPLE_MODE_T sampleMode;
    UINT32    width, height, orgwidth, orgheight;
    BOOL    headerFixed = FALSE;

    printD("DD::decodeJPG\n");

    if (jCTX)
        resetJPG(jCTX);
    else
    {
        RETAILMSG(1, (TEXT("DD::JPG CTX is NULL\r\n")));
        return JPG_FAIL;
    }

    __try
    {
        ////////////////////////////////////////
        // Header Parsing                     //
        ////////////////////////////////////////

        decodeHeader(jCTX);

        ret = waitForIRQ(jCTX);
        if(ret != OK_HD_PARSING)
        {
            RETAILMSG(1, (TEXT("DD::JPG Header Parsing Error(%d)\r\n"), ret));
            return JPG_FAIL;
        }

        sampleMode = getSampleType(jCTX);
        printD("DD::sampleMode : %d\n", sampleMode);
        if(sampleMode == JPG_SAMPLE_UNKNOWN)
        {
            RETAILMSG(1, (TEXT("DD::JPG has invalid sampleMode\r\n")));
            return JPG_FAIL;
        }
        decParam->sampleMode = sampleMode;

        getXY(jCTX, &width, &height);
        printD("DD:: width : 0x%x height : 0x%x\n", width, height);
        if(width <= 0 || width > MAX_JPG_WIDTH || height <= 0 || height > MAX_JPG_HEIGHT)
        {
            RETAILMSG(1, (TEXT("DD::JPG has invalid width/height\n")));
            return JPG_FAIL;
        }

        ////////////////////////////////////////
        // Header Correction                  //
        ////////////////////////////////////////

        orgwidth = width;
        orgheight = height;

        if(!isCorrectHeader(sampleMode, &width, &height))
        {
            rewriteHeader(jCTX, decParam->fileSize, width, height);
            headerFixed = TRUE;
        }

        ////////////////////////////////////////
        // Body Decoding                      //
        ////////////////////////////////////////

        if(headerFixed)
        {
            resetJPG(jCTX);
            decodeHeader(jCTX);

            ret = waitForIRQ(jCTX);
            if(ret != OK_HD_PARSING)
            {
                RETAILMSG(1, (TEXT("DD::JPG Header Parsing Error(%d)\r\n"), ret));
                return JPG_FAIL;
            }

            decodeBody(jCTX);

            ret = waitForIRQ(jCTX);
            if(ret != OK_ENC_OR_DEC)
            {
                RETAILMSG(1, (TEXT("DD::JPG Body Decoding Error(%d)\n"), ret));
                return JPG_FAIL;
            }

            // for post processor, discard pixel
            if(orgwidth % 4 != 0)
                orgwidth = (orgwidth/4)*4;

            if(orgheight % 2 != 0)
                orgheight = (orgheight/2)*2;

            printD("DD:: orgwidth : %d orgheight : %d\n", orgwidth, orgheight);
            rewriteYUV(jCTX, width, orgwidth, height, orgheight);

            // JPEG H/W IP always return YUV422
            decParam->dataSize = getYUVSize(JPG_422, orgwidth, orgheight);
            decParam->width = orgwidth;
            decParam->height = orgheight;
        }
        else
        {
            decodeBody(jCTX);

            ret = waitForIRQ(jCTX);
            if(ret != OK_ENC_OR_DEC)
            {
                RETAILMSG(1, (TEXT("DD::JPG Body Decoding Error(%d)\n"), ret));
                return JPG_FAIL;
            }

            // JPEG H/W IP always return YUV422
            decParam->dataSize = getYUVSize(JPG_422, width, height);
            decParam->width = width;
            decParam->height = height;
        }
    }
     __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        RETAILMSG(1, (TEXT("decodeJPG::JPG exception\n")));
        return JPG_FAIL;
    }
    
    return JPG_SUCCESS;
}

/*----------------------------------------------------------------------------
*Function: isCorrectHeader

*Parameters:    sampleMode:
                width:
                height:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
BOOL isCorrectHeader(SAMPLE_MODE_T sampleMode, UINT32 *width, UINT32 *height)
{
    BOOL result = FALSE;

    switch(sampleMode)
    {
        case JPG_400 :
        case JPG_444 :
            if((*width % 8 == 0) && (*height % 8 == 0))
                result = TRUE;
            if(*width % 8 != 0)
                *width += 8 - (*width % 8);
            if(*height % 8 != 0)
                *height += 8 - (*height % 8);
            break;
            
        case JPG_422 :
            if((*width % 16 == 0) && (*height % 8 == 0))
                result = TRUE;
            if(*width % 16 != 0)
                *width += 16 - (*width % 16);
            if(*height % 8 != 0)
                *height += 8 - (*height % 8);
            break;
            
        case JPG_420 :
        case JPG_411 :
            if((*width % 16 == 0) && (*height % 16 == 0))
                result = TRUE;
            if(*width % 16 != 0)
                *width += 16 - (*width % 16);
            if(*height % 16 != 0)
                *height += 16 - (*height % 16);
            break;
    }

    printD("DD::after error correction : width(%x) height(%x)\n", *width, *height);
    return(result);
}

/*----------------------------------------------------------------------------
*Function: rewriteHeader

*Parameters:    jCTX:
                file_size:
                width:
                height:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
void rewriteHeader(S3C6410_JPG_CTX *jCTX, UINT32 file_size, UINT32 width, UINT32 height)
{
    UINT32    i;
    UINT8    *ptr = (UINT8 *)jCTX->v_pJPGData_Buff + file_size;
    UINT8    *ptr2; 
    UINT8    *SOF1 = NULL, *SOF2 = NULL;
    UINT8    *header = NULL;

    printD("DD::Header is not multiple of MCU\n");
    
    for(i=0; i < file_size; i++)
    {
        ptr--;
        if(*ptr == 0xFF)
        {
            ptr2 = ptr+1;
            if((*ptr2 == BASELINE) || (*ptr2 == EXTENDED_SEQ) || (*ptr2 == PROGRESSIVE))
            {
                printD("DD::match FFC0(i : %d)\n", i);
                SOF1 = ptr2+1;
                break;
            }
        }
    }

    printD("DD::start header correction\n");
    
    if(i <= file_size)
    {
        header = SOF1;
        
        header += 3; //length(2) + sampling bit(1)
        *header = (height>>8) & 0xFF;
        *header++;
        *header = height & 0xFF;
        *header++;
        *header = (width>>8) & 0xFF;
        *header++;
        *header = width & 0xFF;
    }
}

/*----------------------------------------------------------------------------
*Function: resetJPG

*Parameters:    jCTX:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
void resetJPG(S3C6410_JPG_CTX *jCTX)
{
    printD("DD::resetJPG\n");
    
    jCTX->v_pJPG_REG->JPGSoftReset = 0; //ENABLE
}

/*----------------------------------------------------------------------------
*Function: decodeHeader

*Parameters:    jCTX:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
void decodeHeader(S3C6410_JPG_CTX *jCTX)
{
    printD("DD::decodeHeader\n");
    
    jCTX->v_pJPG_REG->JPGFileAddr0 = JPG_DATA_BASE_ADDR;
    jCTX->v_pJPG_REG->JPGFileAddr1 = JPG_DATA_BASE_ADDR;
    
    jCTX->v_pJPG_REG->JPGMod = JPG_DECODE; //decoding mode
    jCTX->v_pJPG_REG->JPGIRQ = ENABLE_IRQ;
    jCTX->v_pJPG_REG->JPGCntl = DISABLE_HW_DEC;
    jCTX->v_pJPG_REG->JPGMISC = (NORMAL_DEC | YCBCR_MEMORY);
    jCTX->v_pJPG_REG->JPGStart = 1;
}

/*----------------------------------------------------------------------------
*Function: decodeBody

*Parameters:    jCTX:
*Return Value:
*Implementation Notes: 
-----------------------------------------------------------------------------*/
void decodeBody(S3C6410_JPG_CTX *jCTX)
{
    printD("DD::decodeBody\n");
    
    jCTX->v_pJPG_REG->JPGYUVAddr0 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;
    jCTX->v_pJPG_REG->JPGYUVAddr1 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE;

    jCTX->v_pJPG_REG->JPGCntl = 0;
    jCTX->v_pJPG_REG->JPGMISC = 0;
    jCTX->v_pJPG_REG->JPGReStart = 1;
}

/*----------------------------------------------------------------------------
*Function: rewriteYUV

*Parameters:    jCTX:
                width:
                orgwidth:
                height:
                orgheight:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
void rewriteYUV(S3C6410_JPG_CTX *jCTX, UINT32 width, UINT32 orgwidth, UINT32 height, UINT32 orgheight)
{
    UINT32    src, dst;
    UINT32    i;
    UINT8    *streamPtr;

    printD("DD::rewriteYUV\n");

    streamPtr = (UINT8 *)(jCTX->v_pJPGData_Buff + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE);
    src = 2*width;
    dst = 2*orgwidth;
    
    for(i = 1; i < orgheight; i++)
    {
        memmove(&streamPtr[dst], &streamPtr[src], 2*orgwidth);
        src += 2*width;
        dst += 2*orgwidth;
    }
}

/*----------------------------------------------------------------------------
*Function: waitForIRQ

*Parameters:    jCTX:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
JPG_RETURN_STATUS waitForIRQ(S3C6410_JPG_CTX *jCTX)
{
    ULONG IRQstatus, OPRstatus;
    char    result;
    BOOL    status = FALSE;
    int        i;

    printD("DD::waitForIRQ\n");

    for(i=0;;i++)
    {
        IRQstatus = jCTX->v_pJPG_REG->JPGIRQStatus;
        if(IRQstatus != 0 )
        {
            status = TRUE;
            break;
        }
        
        if(i > 10000)
        {
            status = FALSE;
            break;
        }
        Sleep(1);
    }

    if(status)
    {
        IRQstatus &= ((1<<6)|(1<<4)|(1<<3));

        switch(IRQstatus)
        {
            case 0x08 : result = OK_HD_PARSING; break;
            case 0x00 : result = ERR_HD_PARSING; break;
            case 0x40 : result = OK_ENC_OR_DEC; break;
            case 0x10 : result = ERR_ENC_OR_DEC; break;
            default : result = ERR_UNKNOWN;
        }
    }
    else
        result = ERR_UNKNOWN;

    OPRstatus = jCTX->v_pJPG_REG->JPGStatus;
    printD("DD::i: %d result : %d OPRstatus : %d\n", i, result, OPRstatus);
    return(result);
}

/*----------------------------------------------------------------------------
*Function:    getSampleType

*Parameters:    jCTX:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
SAMPLE_MODE_T getSampleType(S3C6410_JPG_CTX *jCTX)
{
    ULONG    jpgMode;
    SAMPLE_MODE_T    sampleMode = JPG_SAMPLE_UNKNOWN;

    jpgMode = jCTX->v_pJPG_REG->JPGMod;

    sampleMode =
        ((jpgMode & JPG_SMPL_MODE_MASK) == 0) ? JPG_444 :
        ((jpgMode & JPG_SMPL_MODE_MASK) == 1) ? JPG_422 :
        ((jpgMode & JPG_SMPL_MODE_MASK) == 2) ? JPG_420 :
        ((jpgMode & JPG_SMPL_MODE_MASK) == 3) ? JPG_400 :
        ((jpgMode & JPG_SMPL_MODE_MASK) == 6) ? JPG_411 : JPG_SAMPLE_UNKNOWN;

    return(sampleMode);
}

/*----------------------------------------------------------------------------
*Function:    getXY

*Parameters:    jCTX:
                x:
                y:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
void getXY(S3C6410_JPG_CTX *jCTX, UINT32 *x, UINT32 *y)
{
    *x = jCTX->v_pJPG_REG->JPGX;
    *y = jCTX->v_pJPG_REG->JPGY;
}

/*----------------------------------------------------------------------------
*Function: getYUVSize

*Parameters:    sampleMode:
                width:
                height:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
UINT32 getYUVSize(SAMPLE_MODE_T sampleMode, UINT32 width, UINT32 height)
{
    switch(sampleMode)
    {
        case JPG_444 : return(width*height*3);
        case JPG_422 : return(width*height*2);
        case JPG_420 :
        case JPG_411 : return((width*height) + (width*height>>1));
        case JPG_400 : return(width*height);
        default : return(0);
    }
}

/*----------------------------------------------------------------------------
*Function: resetJPG

*Parameters:    jCTX:
*Return Value:
*Implementation Notes:
-----------------------------------------------------------------------------*/
JPG_RETURN_STATUS encodeJPG(S3C6410_JPG_CTX *jCTX,
                            JPG_ENC_PROC_PARAM    *EncParam)
{
    UINT    i, ret;

    __try
    {
        if(    EncParam->width <= 0 || EncParam->width > MAX_JPG_WIDTH
            || EncParam->height <=0 || EncParam->height > MAX_JPG_HEIGHT)
        {
            RETAILMSG(1, (TEXT("DD::Encoder : Invalid width/height\r\n")));
            return JPG_FAIL;
        }

        resetJPG(jCTX);

        jCTX->v_pJPG_REG->JPGMod = (EncParam->sampleMode == JPG_422) ? (JPG_422 << JPG_SMPL_MODE_BIT) : (JPG_420 << JPG_SMPL_MODE_BIT);
        jCTX->v_pJPG_REG->JPGRSTPos = JPG_RESTART_INTRAVEL; // MCU inserts RST marker
        jCTX->v_pJPG_REG->JPGQTblNo = (JPG_1BIT_MASK << JPG_QUANT_TABLE3_BIT) | (JPG_1BIT_MASK << 14);
        jCTX->v_pJPG_REG->JPGX = EncParam->width;
        jCTX->v_pJPG_REG->JPGY = EncParam->height;

        if(EncParam->encType == JPG_MAIN)
        {
            jCTX->v_pJPG_REG->JPGYUVAddr0 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE; // Address of input image
            jCTX->v_pJPG_REG->JPGYUVAddr1 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE; // Address of input image
            jCTX->v_pJPG_REG->JPGFileAddr0 = JPG_DATA_BASE_ADDR; // Address of JPEG stream
            jCTX->v_pJPG_REG->JPGFileAddr1 = JPG_DATA_BASE_ADDR; // next address of motion JPEG stream
        }
        else
        { // thumbnail encoding
            jCTX->v_pJPG_REG->JPGYUVAddr0 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE; // Address of input image
            jCTX->v_pJPG_REG->JPGYUVAddr1 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE + JPG_STREAM_THUMB_BUF_SIZE + JPG_FRAME_BUF_SIZE; // Address of input image
            jCTX->v_pJPG_REG->JPGFileAddr0 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE; // Address of JPEG stream
            jCTX->v_pJPG_REG->JPGFileAddr1 = JPG_DATA_BASE_ADDR + JPG_STREAM_BUF_SIZE; // next address of motion JPEG stream
        }

        jCTX->v_pJPG_REG->JPGCOEF1 = COEF1_RGB_2_YUV; // Coefficient value 1 for RGB to YCbCr
        jCTX->v_pJPG_REG->JPGCOEF2 = COEF2_RGB_2_YUV; // Coefficient value 2 for RGB to YCbCr
        jCTX->v_pJPG_REG->JPGCOEF3 = COEF3_RGB_2_YUV; // Coefficient value 3 for RGB to YCbCr

        jCTX->v_pJPG_REG->JPGMISC = (JPG_MODESEL_YCBCR << JPG_MODE_SEL_BIT) | (0 << 2);
        jCTX->v_pJPG_REG->JPGCntl = DISABLE_MOTION_ENC;


        // Quantiazation and Huffman Table setting
        for (i=0; i<64; i++)
            jCTX->v_pJPG_REG->JQTBL0[i] = (UINT32)QTBL_Luminance[EncParam->quality][i];

        for (i=0; i<64; i++)
            jCTX->v_pJPG_REG->JQTBL1[i] = (UINT32)QTBL_Chrominance[EncParam->quality][i];

        for (i=0; i<16; i++)
            jCTX->v_pJPG_REG->JHDCTBL0[i] = (UINT32)HDCTBL0[i];

        for (i=0; i<12; i++)
            jCTX->v_pJPG_REG->JHDCTBLG0[i] = (UINT32)HDCTBLG0[i];

        for (i=0; i<16; i++)
            jCTX->v_pJPG_REG->JHACTBL0[i] = (UINT32)HACTBL0[i];

        for (i=0; i<162; i++)
            jCTX->v_pJPG_REG->JHACTBLG0[i] = (UINT32)HACTBLG0[i];

        jCTX->v_pJPG_REG->JPGStart = 0;

        ret = waitForIRQ(jCTX);
        if(ret != OK_ENC_OR_DEC)
        {
            RETAILMSG(1, (TEXT("DD::JPG Encoding Error(%d)\n"), ret));
            return JPG_FAIL;
        }

        EncParam->fileSize = jCTX->v_pJPG_REG->JPGDataSize;
    }
     __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        RETAILMSG(1, (TEXT("encodeJPG::JPG exception\n")));
        return JPG_FAIL;
    }
    return JPG_SUCCESS;
}
