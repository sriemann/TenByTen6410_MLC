//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// wavefile.h
// routines for reading/writing .wav files

MMRESULT ReadWaveFile(LPCTSTR pszFilename, PWAVEFORMATEX * ppWFX, PDWORD pdwBufferSize, PBYTE * ppBufferBits);
MMRESULT WriteWaveFile(LPCTSTR pszFilename, PWAVEFORMATEX pWFX, DWORD dwBufferSize, PBYTE pBufferBits);
