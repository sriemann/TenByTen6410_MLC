#include <windows.h>
#include <pehdr.h>
#include <romldr.h> 
#include <bsp_cfg.h>
#include "image_cfg.h"
#include "utils.h"

#define EBOOT_NB0_SIZE (0x80000/0x200)
#define LOAD_ADDRESS_PHYSICAL (IMAGE_EBOOT_PA_START)
// SD/MMC Card Block Size.
#define globalBlockSizeHide		(*((volatile unsigned int*)( 0x0C003FFC)))

#define CopyMovitoMem(z,a,b,c,e)	(((int(*)(int, unsigned int, unsigned short, unsigned int*, int))(*((unsigned int *)0x0C004008)))(z,a,b,c,e))

/**
  * This Function copy MMC(MoviNAND/iNand) Card Data to memory.
  * Always use EPLL source clock.
  * This function works at 25Mhz.
  * @param UINT32 StartBlkAddress : Source card(MoviNAND/iNand MMC)) Address.(It must block address.)
  * @param UINT16 blockSize : Number of blocks to copy.
  * @param UINT32* memoryPtr : Buffer to copy from.
* @param UINT32 extClockSpeed : External clock speed(per HZ)
* @param bool with_init : determined card initialization.
  * @return bool(u8) - Success or failure.
  */
typedef BOOL (*CopyMovitoMem)( int, UINT32 , UINT16 , UINT32* , BOOL);  
 


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
	//UINT32 *pBuf;
	
	//MMU_EnableICache();

	Uart_Init();

//	Uart_SendString("\nREAD EBOOT from SDMMC\n");
	Uart_SendString("\nReading EBOOT from SDMMC\n");
	

	//*pBuf = globalBlockSizeHide-18-EBOOT_NB0_SIZE;
	
	//((CopyMovitoMem)(*((UINT32 *)(READ_MOVI_FUNCTION))))( globalBlockSizeHide - 2 - 16 - EBOOT_NB0_SIZE, (UINT16)EBOOT_NB0_SIZE, pBuf , 1 );	
	CopyMovitoMem(1,globalBlockSizeHide-18-EBOOT_NB0_SIZE, EBOOT_NB0_SIZE, (unsigned int *)LOAD_ADDRESS_PHYSICAL, 1);

	((PFN_IMAGE_LAUNCH)(LOAD_ADDRESS_PHYSICAL))();
}

