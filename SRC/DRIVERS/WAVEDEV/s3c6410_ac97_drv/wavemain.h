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
#pragma once
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//      Copyright (c) 1995-2000 Microsoft Corporation.  All rights reserved.
//
// -----------------------------------------------------------------------------

#include <windows.h>
#include <types.h>
#include <memory.h>
#include <string.h>
#include <nkintr.h>
#include <excpt.h>
#include <wavedbg.h>
#include <wavedev.h>
#include <waveddsi.h>
#include <mmddk.h>
#include <cardserv.h>
#include <devload.h>

#include <linklist.h>
#include <audiosys.h>
#include <wfmtmidi.h>

class StreamContext;
class WaveStreamContext;
class InputStreamContext;
class OutputStreamContext;
class DeviceContext;
class MixerDeviceContext;
class InputDeviceContext;
class OutputDeviceContext;
class HardwareContext;

#include "wavepdd.h"
#include "devctxt.h"
#include "hwctxt.h"
#include "strmctxt.h"
#include "midistrm.h"
#include "mixerdrv.h"

