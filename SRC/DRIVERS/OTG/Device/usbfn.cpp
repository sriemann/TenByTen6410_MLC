#include <windows.h>
#include <stdio.h>
#include <usbfnioctl.h>
#include <svsutil.hxx>

#define ZONE_TRACE  DEBUGZONE(9)
#define ZONE_ERROR  DEBUGZONE(0)

HANDLE GetUSBfnHandle();
VOID   CloseUfnController(HANDLE hUSBfn);

BOOL USBCurrentDriver(PUFN_CLIENT_INFO pDriverList) 
{
    HANDLE          hUSBfn;
    DWORD           bytesReturned;
    BOOL            bRet = FALSE;


    hUSBfn = GetUSBfnHandle();
    if (hUSBfn == INVALID_HANDLE_VALUE) {
        return bRet;
    }
    if (bRet = DeviceIoControl(hUSBfn, 
                               IOCTL_UFN_GET_CURRENT_CLIENT, 
                               NULL, 
                               0, 
                               pDriverList, 
                               sizeof(*pDriverList), 
                               &bytesReturned, 
                               NULL)) {
        ASSERT(bytesReturned == sizeof(*pDriverList));
        DEBUGMSG(ZONE_TRACE, (_T("Current Client \"%s\" \n"), pDriverList->szName));
    } else {
        DEBUGMSG(ZONE_ERROR, (_T("IOCTL_UFN_GET_CURRENT_CLIENT failed, error %d\n"), GetLastError()));

    }
    CloseUfnController(hUSBfn);
    return bRet;
}



VOID CloseUfnController(HANDLE hUSBfn)
{
    CloseHandle(hUSBfn);
}

HANDLE GetUSBfnHandle()
{
    HANDLE                      hUSBfn = INVALID_HANDLE_VALUE;
    BYTE                        guidBuffer[sizeof(GUID) + 4]; // +4 since scanf writes longs
    LPGUID                      pGuid = (LPGUID) guidBuffer;
    LPCTSTR                     pUSBFnGuid = _T("E2BDC372-598F-4619-BC50-54B3F7848D35");
    DEVMGR_DEVICE_INFORMATION   devInfo;
    HANDLE                      hTemp;
    DWORD                       error;


    error = _stscanf(pUSBFnGuid, SVSUTIL_GUID_FORMAT, SVSUTIL_PGUID_ELEMENTS(&pGuid));
    
    if ((error == 0) || (error == EOF)) {
        DEBUGMSG(ZONE_ERROR, (_T("Can not find USB FN GUID!\r\n")));
        return hUSBfn;
    }
    ASSERT(error != 0 && error != EOF);  


    // Get a handle to the bus driver
    memset(&devInfo, 0, sizeof(devInfo));
    devInfo.dwSize = sizeof(devInfo);
    hTemp = FindFirstDevice(DeviceSearchByGuid, pGuid, &devInfo);

    if (hTemp != INVALID_HANDLE_VALUE) {
        hUSBfn = CreateFile(devInfo.szBusName, 
                            GENERIC_READ, 
                            FILE_SHARE_READ, 
                            NULL, 
                            OPEN_EXISTING, 
                            0, 
                            NULL);
    } else {
//        RETAILMSG(ZONE_ERROR, (_T("No available UsbFn controller!\r\n")));
    }
    return hUSBfn;
}
