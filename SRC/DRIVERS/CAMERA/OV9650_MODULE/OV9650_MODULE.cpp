
// --------------------------------------------------------------------------------------
// Head files
#include <windows.h>
#include <nkintr.h>
#include <pm.h>
#include "pmplatform.h"
#include <ceddk.h>
#include <S3c6410.h>
#include <bsp.h>
#include "iic.h"
#include "setting.h"
#include "Module.h"

// --------------------------------------------------------------------------------------
// Macros Definitions
#define OV9650_ERROR     1
#define OV9650_DEBUG	 0

#define OV9650_PID       0x96
#define OV9650_VER       0x52

#define OV9650_PID_REG	 0x0A
#define OV9650_VER_REG   0x0B

#define OV9650_SLAVE_ID  0x60
#define CAMERA_WRITE    (OV9650_SLAVE_ID | 0x0)
#define CAMERA_READ     (OV9650_SLAVE_ID | 0x1)

#define DEFAULT_MODULE_ITUXXX        CAM_ITU601
#define DEFAULT_MODULE_YUVORDER      CAM_ORDER_YCBYCR
#define DEFAULT_MODULE_HSIZE         640
#define DEFAULT_MODULE_VSIZE         480
#define DEFAULT_MODULE_HOFFSET       0
#define DEFAULT_MODULE_VOFFSET       0
#define DEFAULT_MODULE_UVOFFSET      CAM_UVOFFSET_0
#define DEFAULT_MODULE_CLOCK         24000000
#define DEFAULT_MODULE_CODEC         CAM_CODEC_422
#define DEFAULT_MODULE_HIGHRST       1
#define DEFAULT_MODULE_INVPCLK       1
#define DEFAULT_MODULE_INVVSYNC      0
#define DEFAULT_MODULE_INVHREF       0

// --------------------------------------------------------------------------------------
// Variables
static MODULE_DESCRIPTOR             gModuleDesc;
static HANDLE                        hI2C;   // I2C Bus Driver

// --------------------------------------------------------------------------------------
// Functions 
int  ModuleWriteBlock();
DWORD HW_WriteRegisters(PUCHAR pBuff, DWORD nRegs);
DWORD HW_ReadRegisters(PUCHAR pBuff, UCHAR StartReg, DWORD nRegs);

int ModuleInit()
{
    DWORD   dwErr = ERROR_SUCCESS, bytes;
    UINT32  IICClock = 100000;
    UINT32  uiIICDelay;
	
    RETAILMSG(OV9650_DEBUG,(TEXT("ModuleInit++\r\n")));
	
    // Initialize I2C Driver
    hI2C = CreateFile( L"IIC0:",
	                   GENERIC_READ|GENERIC_WRITE,
	                   FILE_SHARE_READ|FILE_SHARE_WRITE,
	                   NULL, OPEN_EXISTING, 0, 0);
                   
    if ( INVALID_HANDLE_VALUE == hI2C ) 
	{
        dwErr = GetLastError();
        RETAILMSG(OV9650_ERROR, (TEXT("Error %d opening device '%s' \r\n"), dwErr, L"I2C0:" ));
        return FALSE;
    }
	else
	{
		RETAILMSG(OV9650_DEBUG, (TEXT("I2C0 Device is opened successfully! \r\n")));
	}
    
    gModuleDesc.ITUXXX = DEFAULT_MODULE_ITUXXX;
    gModuleDesc.UVOffset = DEFAULT_MODULE_UVOFFSET;
    gModuleDesc.SourceHSize = DEFAULT_MODULE_HSIZE;
    gModuleDesc.Order422 = DEFAULT_MODULE_YUVORDER;
    gModuleDesc.SourceVSize = DEFAULT_MODULE_VSIZE;
    gModuleDesc.Clock = DEFAULT_MODULE_CLOCK;
    gModuleDesc.Codec = DEFAULT_MODULE_CODEC;
    gModuleDesc.HighRst = DEFAULT_MODULE_HIGHRST;
    gModuleDesc.SourceHOffset = DEFAULT_MODULE_HOFFSET;
    gModuleDesc.SourceVOffset = DEFAULT_MODULE_VOFFSET;
    gModuleDesc.InvPCLK = DEFAULT_MODULE_INVPCLK;
    gModuleDesc.InvVSYNC = DEFAULT_MODULE_INVVSYNC;
    gModuleDesc.InvHREF = DEFAULT_MODULE_INVHREF;

	RETAILMSG(OV9650_DEBUG,(TEXT("ModuleInit: gModuleDesc.Clock = %d\n"),gModuleDesc.Clock));
    
    // use iocontrol to write
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_SET_CLOCK, 
                          &IICClock, sizeof(UINT32), 
                          NULL, 0,
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(OV9650_ERROR,(TEXT("IOCTL_IIC_SET_CLOCK ERROR: %u \r\n"), dwErr));
        return FALSE;
    }       
    
    uiIICDelay = Clk_0;
    
    if ( !DeviceIoControl(hI2C,
                      IOCTL_IIC_SET_DELAY, 
                      &uiIICDelay, sizeof(UINT32), 
                      NULL, 0,
                      &bytes, NULL) )
    {
        dwErr = GetLastError();
        RETAILMSG(OV9650_ERROR,(TEXT("IOCTL_IIC_SET_DELAY ERROR: %u \r\n"), dwErr));
        return FALSE;
    }

    RETAILMSG(OV9650_DEBUG,(TEXT("ModuleInit--\r\n")));
	
    return TRUE;
}

void ModuleDeinit()
{    
    CloseHandle(hI2C);
}

int  ModuleWriteBlock()
{
    int i;
    UCHAR BUF=0;
	
    RETAILMSG(OV9650_DEBUG,(TEXT("ModuleWriteBlock++ \r\n")));
	
	// Read OV9650 PID
    HW_ReadRegisters(&BUF, OV9650_PID_REG, 1);
	if (OV9650_PID == BUF)
		RETAILMSG(OV9650_DEBUG,(TEXT("Read OV9650_PID: 0x%x successfully! \r\n"), BUF));
	else
		RETAILMSG(OV9650_ERROR,(TEXT("Read OV9650_PID: 0x%x failed! \r\n"), BUF));

	// Read OV9650 VID
	HW_ReadRegisters(&BUF, OV9650_VER_REG, 1);
	if (OV9650_VER == BUF)
		RETAILMSG(OV9650_DEBUG,(TEXT("Read OV9650_VER_REG: 0x%x successfully! \r\n"), BUF));
	else
		RETAILMSG(OV9650_ERROR,(TEXT("Read OV9650_VER_REG: 0x%x failed! \r\n"), BUF));

	RETAILMSG(OV9650_DEBUG, (TEXT("sizeof(ov9650_reg)= %d, sizeof(ov9650_reg[0]) = %d, OV9650_REGS = %d.\r\n"), sizeof(ov9650_reg), sizeof(ov9650_reg[0]), OV9650_REGS));    	
    for(i=0; i<OV9650_REGS; i++)
    {
        HW_WriteRegisters(&ov9650_reg[i][0], 2);
		RETAILMSG(OV9650_DEBUG,(TEXT("HW_WriteRegisters: Reg[0x%x] = 0x%x, i = %d.\r\n"), ov9650_reg[i][0], ov9650_reg[i][1], i));
    }
    
    ModuleSetImageSize(VGA);
    
#if OV9650_DEBUG   
    for(i=0; i<OV9650_REGS; i++)
    {
        HW_ReadRegisters(&BUF, ov9650_reg[i][0], 1);
		RETAILMSG(OV9650_DEBUG,(TEXT("HW_ReadRegisters: Reg[0x%x] = 0x%x, i = %d.\r\n"), ov9650_reg[i][0], BUF ,i));
    }
#endif  
    RETAILMSG(OV9650_DEBUG,(TEXT("ModuleWriteBloc-- \r\n")));
    return TRUE;
}

DWORD
HW_WriteRegisters(
    PUCHAR pBuff,   // Optional buffer
    DWORD nRegs     // number of registers
    )
{
    DWORD dwErr=0;
    DWORD bytes;
    IIC_IO_DESC IIC_Data;
    
    RETAILMSG(OV9650_DEBUG,(TEXT("HW_WriteRegisters++ \r\n")));    
    
    IIC_Data.SlaveAddress = CAMERA_WRITE;
    IIC_Data.Count    = nRegs;
    IIC_Data.Data = pBuff;
    
    // use iocontrol to write
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_WRITE, 
                          &IIC_Data, sizeof(IIC_IO_DESC), 
                          NULL, 0,
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(OV9650_ERROR,(TEXT("IOCTL_IIC_WRITE ERROR: %u \r\n"), dwErr));
    }   

    if ( dwErr ) {
        RETAILMSG(OV9650_ERROR, (TEXT("I2CWrite ERROR: %u \r\n"), dwErr));
    }            
    RETAILMSG(OV9650_DEBUG,(TEXT("HW_WriteRegisters-- \r\n")));    

    return dwErr;
}

DWORD
HW_ReadRegisters(
    PUCHAR pBuff,       // Optional buffer
    UCHAR StartReg,     // Start Register
    DWORD nRegs         // Number of Registers
    )
{
    DWORD dwErr=0;
    DWORD bytes;
    IIC_IO_DESC IIC_AddressData, IIC_Data;

	RETAILMSG(OV9650_DEBUG,(TEXT("HW_ReadRegisters++ \r\n"))); 
	
    IIC_AddressData.SlaveAddress = CAMERA_WRITE;
    IIC_AddressData.Data = &StartReg;
    IIC_AddressData.Count = 1;
    
    IIC_Data.SlaveAddress = CAMERA_READ;
    IIC_Data.Data = pBuff;
    IIC_Data.Count = 1;
    
    // use iocontrol to read    
    if ( !DeviceIoControl(hI2C,
                          IOCTL_IIC_READ, 
                          &IIC_AddressData, sizeof(IIC_IO_DESC), 
                          &IIC_Data, sizeof(IIC_IO_DESC),
                          &bytes, NULL) ) 
    {
        dwErr = GetLastError();
        RETAILMSG(OV9650_ERROR,(TEXT("IOCTL_IIC_WRITE ERROR: %u \r\n"), dwErr));
    }   

    
    if ( !dwErr ) {


    } else {        
        RETAILMSG(OV9650_ERROR,(TEXT("I2CRead ERROR: %u \r\n"), dwErr));
    }            

	RETAILMSG(OV9650_DEBUG,(TEXT("HW_ReadRegisters-- \r\n"))); 
	
    return dwErr;
}


// copy module data to output buffer
void ModuleGetFormat(MODULE_DESCRIPTOR &outModuleDesc)
{
    memcpy(&outModuleDesc, &gModuleDesc, sizeof(MODULE_DESCRIPTOR));
}

int     ModuleSetImageSize(int imageSize)
{
    BYTE sizeValue[2];
    
    sizeValue[0] = 0x12;
    switch(imageSize)
    {
        case VGA:
            sizeValue[1] = 0x40;
            break;
        case CIF:
            sizeValue[1] = 0x20;
            break;
        case QVGA:
            sizeValue[1] = 0x10;
            break;
        case QCIF:
            sizeValue[1] = 0x08;
            break;
		case RAW_RGB:
			sizeValue[1] = 0x04;
			break;
    }
    
    HW_WriteRegisters(sizeValue, 2);
    return TRUE;
}
