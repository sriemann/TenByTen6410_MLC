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

Module Name:    s3c6410_pm.h

Abstract:       Common Definition of PM module

Functions:


Notes:


--*/

#ifndef _S3C6410_PM__H_
#define _S3C6410_PM__H_

#define SETVOLTAGE_ARM            (1)
#define SETVOLTAGE_INTERNAL    (2)
#define SETVOLTAGE_BOTH        (SETVOLTAGE_ARM|SETVOLTAGE_INTERNAL)

extern void PMIC_Init();
extern void PMIC_VoltageSet(UINT32 uPwr, UINT32 uVolate, UINT32 uDelay);

#endif _S3C6410_PM__H_
