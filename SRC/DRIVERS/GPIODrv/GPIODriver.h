//
// Copyright (c) Hiteg Ltd.  All rights reserved.
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
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

Module Name:    GPIODrv

Abstract:       Device Driver to controll GPIO PIN 22 to PIN 37 on TenByTen6410 GPIO connector

Functions:
				

Notes:

		Verion	1.0			2013		sven.riemann@hiteg.com

--*/
#define IO_CTL_GPE4_PIN22				0x00
#define IO_CTL_GPK0_PIN23				0x01		
#define IO_CTL_GPK1_PIN24				0x02		
#define IO_CTL_GPK2_PIN25				0x03		
#define IO_CTL_GPK3_PIN26				0x04
#define IO_CTL_GPK4_PIN27				0x05		
#define IO_CTL_GPK5_PIN28				0x06		
#define IO_CTL_GPL8_PIN29				0x07		
#define IO_CTL_GPL9_PIN30				0x08
#define IO_CTL_GPL10_PIN31				0x09
#define IO_CTL_GPL11_PIN32				0x0A
#define IO_CTL_GPP8_PIN33				0x0B
#define IO_CTL_GPP9_PIN34				0x0C
#define IO_CTL_GPP12_PIN35				0x0D
#define IO_CTL_GPE0_PIN36				0x0E
#define IO_CTL_GPE1_PIN37				0x0F

// COMMANDS for IO-CTL
#define IO_CTL_GPIO_SELECT				0x00
#define IO_CTL_GPIO_SET_OUTPUT			0x10
#define IO_CTL_GPIO_SET_INPUT			0x20
#define IO_CTL_GPIO_PULL_UP				0x30
#define IO_CTL_GPIO_PULL_DOWN			0x40
#define IO_CTL_GPIO_NO_PUD				0x50
#define IO_CTL_GPIO_EINT_MODE_RAISING	0x60 // not implemented yet
#define IO_CTL_GPIO_EINT_MODE_FALLING	0x70 // not implemented yet
#define IO_CTL_GPIO_EINT_MODE_BOTH		0x80 // not implemented yet
#define IO_CTL_GPIO_RW_MODE_BULK		0x90 // switches to r/w all ports at once
#define IO_CTL_GPIO_RW_MODE_SINGLE		0xA0 // switches to r/w single port
