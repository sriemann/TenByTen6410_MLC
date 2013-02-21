//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/
/*
***************************************************************************//*!
*
* \file        pixel.h
* \brief    pixel format definitions and conversion function declarations
*
*//*---------------------------------------------------------------------------
* NOTES:
*
*/

#ifndef __PIXELFMTS_H__
#define __PIXELFMTS_H__

#ifdef __cplusplus
extern "C" {
#endif

//! Pixel Format definitions
typedef enum {
    /*FB color pixel formats*/
    
    //formats with alpha
    E_ARGB8,
    E_ARGB4,
    E_ARGB1555,

    E_LUMINANCE_ALPHA88,
    E_ALPHA_LUMINANCE88,
    E_LUMINANCE_ALPHA08,
    E_RGBA4,     
    E_RGBA5551,    
    
    E_ABGR8, 
    E_ALPHA8, //THIS must be the last alpha format hasAlpha depends on it
    
    
    E_ARGB0888,
    E_RGB8,
    
    E_RGB565,
    E_RGB5,
    
    E_LUMINANCE8,
    
    E_LUMINANCE_ALPHA80,
    
    /*FB depth/stencil formats*/
    E_Stencil8,
    E_Depth24,
    E_Stencil8Depth24,
    
    // added shariq for texture format  
    // first one is at higher memory address 
    
    
    // input as well as output texture data 
    
    //E_ARGB8,  // after swapping R & B already present above 
    //input texture data           
    E_BGR8,     
    
    //COMPRESSED texture formats
    E_PALETTE4_RGB8_OES,        
    E_PALETTE4_RGBA8_OES,             
    E_PALETTE4_R5_G6_B5_OES,          
    E_PALETTE4_RGBA4_OES,             
    E_PALETTE4_RGB5_A1_OES,           
    E_PALETTE8_RGB8_OES,              
    E_PALETTE8_RGBA8_OES,             
    E_PALETTE8_R5_G6_B5_OES,          
    E_PALETTE8_RGBA4_OES,             
    E_PALETTE8_RGB5_A1_OES, 
    E_RGB_S3TC_OES ,                  
     E_RGBA_S3TC_OES,                 


    //Note before adding texture formats, verify that the texture formats match 
    // with the FB or not
    // And define new ones only if they do not match in component arrangement
    E_NUM_PIXEL_FMTS,
    E_INVALID_PIXEL_FORMAT,
} PxFmt;

#ifdef __cplusplus
}
#endif
#endif //__PIXELFMTS_H__
