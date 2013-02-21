#include <windows.h>
#include <pehdr.h>
#include <romldr.h>
#include "option.h"
#include "s3c6410_addr.h"
#include "nand.h"
#include "utils.h"
//#define BUZZE_ON              (0x0<<15)
//#define BUZZE_OFF             (0x1<<15)
//void Buzze_init(int);

#define MESSAGE_ON		(1)
#define NAND_BYTE_PER_PAGE	(2048)
#ifdef SUPPORTSLC
#define NAND_PAGE_PER_BLOCK	(64)
#endif
#ifdef SUPPORTMLC
#define NAND_PAGE_PER_BLOCK	(128)
#endif
#define NAND_BYTE_PER_BLOCK	(NAND_PAGE_PER_BLOCK*NAND_BYTE_PER_PAGE)

#define LOAD_ADDRESS_PHYSICAL		(0x50000000)									// 2nd loader address
#define LOAD_BYTE_SIZE				(0x00010000)									// 120 KB
#define LOAD_PAGE_SIZE				(LOAD_BYTE_SIZE/NAND_BYTE_PER_PAGE)
#define LOAD_IMAGE_BYTE_OFFSET	(0x00001000)									// 4096 byte for stepldr
#define LOAD_IMAGE_PAGE_OFFSET	(LOAD_IMAGE_BYTE_OFFSET/NAND_BYTE_PER_PAGE)

//#define DEBUGUART

// Function prototypes.
void MMU_EnableICache(void);

// Globals variables.
ROMHDR * volatile const pTOC = (ROMHDR *)-1;

typedef void (*PFN_IMAGE_LAUNCH)();

static BOOLEAN SetupCopySection(ROMHDR *const pTOC)
{
	// This code doesn't make use of global variables so there are no copy sections.  To reduce code size, this is a stub function...
	//
	return(TRUE);
}


void main(void)
{
	register nPage;
	unsigned char *pBuf;
	unsigned char ucDID, ucHID;
	unsigned char nCnt;
	unsigned char uNumOfLoadPage = LOAD_PAGE_SIZE;
	BOOL b4KPage = FALSE;

	// Set up copy section (initialized globals).
	//
	// NOTE: after this call, globals become valid.
	//
//	SetupCopySection(pTOC);

	// Enable the ICache.
	// MMU_EnableICache();

	// Set up all GPIO ports.
	Port_Init();
    Led_Display(0xf);
  //  Buzze_init(BUZZE_ON);

#ifdef DEBUGUART
	// UART initialize
	Uart_Init();
	//Uart_SendString("\r\n\r\nWince 5.0 1st NAND Bootloader (NBL1) for SMDK2443\r\n");

	// Initialize the NAND flash interface.
	Uart_SendString("NAND Initialize\r\n");
#endif

	NAND_Init();

	Read_DeviceID(0, &ucDID, &ucHID);
#ifdef DEBUGUART
	Uart_SendString("Device ID : 0x");
	Uart_SendBYTE(ucDID, 1);
	Uart_SendString("Hidden ID : 0x");
	Uart_SendBYTE(ucHID, 1);
#endif


	if (   (ucDID == 0xd5 && ucHID == 0x14)
	    || (ucDID == 0xd5 && ucHID == 0x94)
	    || (ucDID == 0xd7 && ucHID == 0x55)
	    || (ucDID == 0xd7 && ucHID == 0xD5)  // for MLC
		|| (ucDID == 0xd3 && ucHID == 0x10))  // for SLC
	{
		b4KPage = TRUE;
		uNumOfLoadPage = LOAD_PAGE_SIZE/2;
	}

	// Turn the LEDs off.
	Led_Display(0x0);

	pBuf = (unsigned char *)LOAD_ADDRESS_PHYSICAL;

	// MLC
	// Page 0, 1 : Steploader
	// Page 2 ~ 5 : empty page
	// Page 6 ~ PAGES_PER_BLOCK-3 : effective page
	// read pages with 0, 1 and 6 to PAGES_PER_BLOCK-3
    nPage = 10;
	for (nCnt = 0; nCnt < uNumOfLoadPage; nCnt++)
	{
	    Led_Display(0x2);
		if (nPage >= (NAND_PAGE_PER_BLOCK-2) || (NAND_Read(0, nPage, pBuf, b4KPage) == FALSE))
		{
#ifdef DEBUGUART
			// Uncorrectable ECC Error
			Uart_SendString("ECC Error @ Page 0x");
			Uart_SendBYTE(nPage, 1);
#endif
			Led_Display(0x9);
			while(1);
		}

		nPage++;

		if (b4KPage == TRUE)
			pBuf += NAND_BYTE_PER_PAGE*2;
		else
			pBuf += NAND_BYTE_PER_PAGE;
        Led_Display(0x4);
	}

	//Uart_SendString("Jump to 2nd Bootloader...\r\n");
//	Uart_SendDWORD(LOAD_ADDRESS_PHYSICAL, 1);

	// Turn the LEDs on.
	//
	//Led_Display(0x5);
   //Buzze_init(BUZZE_OFF);	
    //Led_Display(0x6);
    Led_Display(0x0);

#ifdef DEBUGUART
	Uart_SendString("Jump to 2nd Bootloader...\r\n");
#endif

	((PFN_IMAGE_LAUNCH)(LOAD_ADDRESS_PHYSICAL))();
}

