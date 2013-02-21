//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.

*/

#ifndef    __DEVICE_DRIVER_LIB_H__
#define __DEVICE_DRIVER_LIB_H__

#if __cplusplus
extern "C"
{
#endif
void *DrvLib_MapIoSpace(UINT32 PhysicalAddress, UINT32 NumberOfBytes, BOOL CacheEnable);
void DrvLib_UnmapIoSpace(void *MappedAddress);

void CPUStall_us(unsigned int us);
void CPUStall_ms(unsigned int ms);

HANDLE InitMutex(LPCTSTR name);
DWORD GetMutex(HANDLE handle);
BOOL BootDeviceInit();
#if __cplusplus
}
#endif

#endif    // __DEVICE_DRIVER_LIB_H__

