/******************************************************************************
* DM9000A.c
* 	This driver is eboot driver for DM9000/DM9000A/DM9010.
*	Created by Spenser, DAVICOM, www.davicom.com.tw.
* V1.00 - 30/05/2005
* V1.01 - 29/06/2005
* V1.02 - 12/07/2005
*			- Add revision detection.
*			- DM9000 = 0 or 1, DM9000A E4=18h, E5 = 19H, DM9010 E4 = 10h, E5 = 11h.
*******************************************************************************/    

#include <windows.h>
#include <ceddk.h>
#include <ethdbg.h>
#include <oal.h>
#include <halether.h>


#define DM9000_ID		0x90000A46

// Hash creation constants.
//
#define CRC_PRIME               0xFFFFFFFF;
#define CRC_POLYNOMIAL          0x04C11DB6;


#define IOREAD(o)					((UCHAR)*((volatile USHORT *)(o)))
#define IOWRITE(o, d)				*((volatile USHORT *)(o)) = (UCHAR)(d)

#define IOREAD16(o)					((USHORT)*((volatile USHORT *)(o)))
#define IOWRITE16(o, d)				*((volatile USHORT *)(o)) = (USHORT)(d)

#define MEMREAD(o)					((USHORT)*((volatile USHORT *)(dwEthernetMemBase + (o))))
#define MEMWRITE(o, d)				*((volatile USHORT *)(dwEthernetMemBase + (o))) = (USHORT)(d)

static DWORD dwEthernetIOBase;
static DWORD dwEthernetDataPort;

static UCHAR DM9000_iomode;
static USHORT hash_table[4];

static DWORD dwEthernetMemBase;

//V1.02
static UCHAR DM9000_rev;	

//#define DM9000_MEM_MODE

#ifdef	DM9000_MEM_MODE

#define READ_REG1					ReadReg
#define READ_REG2					MEMREAD

#define WRITE_REG1					WriteReg
#define WRITE_REG2					MEMWRITE

#else

#define READ_REG1					ReadReg
#define READ_REG2					ReadReg

#define WRITE_REG1					WriteReg
#define WRITE_REG2					WriteReg

#endif

static	BOOL bIsPacket;


static UCHAR
ReadReg(USHORT offset)
{
	IOWRITE(dwEthernetIOBase, offset);
	return IOREAD(dwEthernetDataPort);
}

static void 
WriteReg(USHORT offset, USHORT data)
{
	IOWRITE(dwEthernetIOBase, offset);
	IOWRITE(dwEthernetDataPort, data);
}


/*
    @func   BYTE | CalculateHashIndex | Computes the logical addres filter hash index value.  This used when there are multiple
	                                    destination addresses to be filtered.
    @rdesc  Hash index value.
    @comm    
    @xref   
*/
BYTE CalculateHashIndex( BYTE  *pMulticastAddr )
{
   DWORD CRC;
   BYTE  HashIndex;
   BYTE  AddrByte;
   DWORD HighBit;
   int   Byte;
   int   Bit;

   // Prime the CRC.
   CRC = CRC_PRIME;

   // For each of the six bytes of the multicast address.
   for ( Byte=0; Byte<6; Byte++ )
   {
      AddrByte = *pMulticastAddr++;

      // For each bit of the byte.
      for ( Bit=8; Bit>0; Bit-- )
      {
         HighBit = CRC >> 31;
         CRC <<= 1;

         if ( HighBit ^ (AddrByte & 1) )
         {
            CRC ^= CRC_POLYNOMIAL;
            CRC |= 1;
         }

         AddrByte >>= 1;
      }
   }

   // Take the least significant six bits of the CRC and copy them
   // to the HashIndex in reverse order.
   for( Bit=0,HashIndex=0; Bit<6; Bit++ )
   {
      HashIndex <<= 1;
      HashIndex |= (BYTE)(CRC & 1);
      CRC >>= 1;
   }

   return(HashIndex);
}

void DM9000_Delay(DWORD dwCounter)
{
	//	Simply loop...
	while (dwCounter--);
}	

void dm9000_hash_table(USHORT *mac)
{
	USHORT i, oft;

	/* Set Node address */
	WRITE_REG1(0x10, (UINT8)(mac[0] & 0xFF));
	WRITE_REG1(0x11, (UINT8)(mac[0] >> 8));
	WRITE_REG1(0x12, (UINT8)(mac[1] & 0xFF));
	WRITE_REG1(0x13, (UINT8)(mac[1] >> 8));
	WRITE_REG1(0x14, (UINT8)(mac[2] & 0xFF));
	WRITE_REG1(0x15, (UINT8)(mac[2] >> 8));	

	/* Clear Hash Table */
	for (i = 0; i < 4; i++)
		hash_table[i] = 0x0;

	/* broadcast address */
	hash_table[3] = 0x8000;
	/* Write the hash table to MAC MD table */
	for (i = 0, oft = 0x16; i < 4; i++) {
		WRITE_REG1(oft++, hash_table[i] & 0xff);
		WRITE_REG1(oft++, (hash_table[i] >> 8) & 0xff);
	}
		

}


static BOOL Probe(void)
{
	BOOL r = FALSE;
	DWORD id_val;
	
	id_val = READ_REG1(0x28);
	id_val |= READ_REG1(0x29) << 8;
	id_val |= READ_REG1(0x2a) << 16;
	id_val |= READ_REG1(0x2b) << 24;
	
	//gao1210
	RETAILMSG(1, (TEXT("DM9000 ID is 0x%x\r\n"), id_val));
	
	if (id_val == DM9000_ID) {
		RETAILMSG(1, (TEXT("INFO: Probe: DM9000 is detected.\r\n")));
		DM9000_rev = READ_REG1(0x2c);
		
		r = TRUE;
	}
	else {
		RETAILMSG(1, (TEXT("ERROR: Probe: Can not find DM9000.\r\n")));
	}

	return r;
}



void DM9000AEnableInts(void)
{
	WRITE_REG1(0xff, 0x83);
}


void DM9000ADisableInts(void)
{
	WRITE_REG1(0xff, 0x80);
}

/* Send a data block via Ethernet. */

static USHORT dm9000_send (BYTE *pbData, USHORT length)
{	
	int i;
	int tmplen;


	IOWRITE(dwEthernetIOBase, 0xf8);	/* data copy ready set */
	/* copy data */
	tmplen = (length + 1) / 2;
	for (i = 0; i < tmplen; i++)
		IOWRITE16(dwEthernetDataPort, ((USHORT *)pbData)[i]);

	WRITE_REG1(0xfd, (length >> 8) & 0xff);  /*set transmit leng */
	WRITE_REG1(0xfc, length & 0xff);
	/* start transmit */
	WRITE_REG1(0x02, 1);
	
	if (DM9000_rev < 0x10) {	//DM9000E
		while(1) {
			if (READ_REG1(0xfe) & 2) {	//TX completed
				WRITE_REG1(0xfe, 2);
				break;
			}
			DM9000_Delay(1000);
		}
	}
	return 0;
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/

BOOL DM9000AInit(BYTE *iobase, ULONG membase, USHORT MacAddr[3])
{
	BOOL r = FALSE;

	bIsPacket         = FALSE;
	dwEthernetIOBase  = (DWORD)iobase;
	dwEthernetDataPort = dwEthernetIOBase + 4;
	dwEthernetMemBase = membase;
    
    r = Probe();	//Detect DM9000
    
	RETAILMSG(1, (TEXT("DM9000: MAC Address: %x:%x:%x:%x:%x:%x\r\n"),
				MacAddr[0] & 0x00FF, MacAddr[0] >> 8,
				MacAddr[1] & 0x00FF, MacAddr[1] >> 8,
				MacAddr[2] & 0x00FF, MacAddr[2] >> 8));

	/* set the internal PHY power-on, GPIOs normal, and wait 2ms */
	WRITE_REG1(0x1f, 0);	/* GPR (reg_1Fh)bit GPIO0=0 pre-activate PHY */	
	DM9000_Delay(200000000);		/* wait 20us for PHY power-on ready */

	/* do a software reset and wait 20us */
	WRITE_REG1(0x0, 3);
	DM9000_Delay(200000000);		/* wait 20us at least for software reset ok */
	WRITE_REG1(0x0, 3);	/* NCR (reg_00h) bit[0] RST=1 & Loopback=1, reset on. Added by SPenser */
	DM9000_Delay(200000000);		/* wait 20us at least for software reset ok */

	/* I/O mode */
	DM9000_iomode = READ_REG1(0xfe) >> 6; /* ISR bit7:6 keeps I/O mode */

	/* Set PHY */
//	db->op_mode = media_mode;
//	set_PHY_mode(db);

	/* Program operating register */
	WRITE_REG1(0x0, 0);
	WRITE_REG1(0x02, 0);		/* TX Polling clear */
//	WRITE_REG1(0x08, 0x3f);	/* Less 3kb, 600us */
	WRITE_REG1(0x2f, 0);		/* Special Mode */
	WRITE_REG1(0x01, 0x2c);	/* clear TX status */
	WRITE_REG1(0xfe, 0x0f); 	/* Clear interrupt status */


 
	/* Set address filter table */
	dm9000_hash_table(MacAddr);

	/* Activate DM9000A/DM9010 */
	WRITE_REG1(0x05, 0x30 | 1);	/* RX enable */
	WRITE_REG1(0xff, 0x83); 	// Enable TX/RX interrupt mask
				
	return r;
}

//----------------------------------------------------------------------

DWORD
DM9000A_GetPendingInts(void)
{
	UCHAR RxRead;
	
	RxRead = READ_REG1(0xf0);
	RxRead = IOREAD(dwEthernetDataPort);
	RxRead = IOREAD(dwEthernetDataPort);

	if ((RxRead & 3) != 1)  /* no data */ 
		return(0);
	return(1);

}


/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
UINT16
DM9000AGetFrame(BYTE *pbData, UINT16 *pwLength)
{
	
	int i;
	unsigned short rxlen, tmplen;
	unsigned short status;
	UCHAR RxRead;
//RETAILMSG(1, (TEXT("[DM9000A]: DM9000A_GetFrame()..........\r\n")));
//V1.01
	READ_REG1(0x3); READ_REG1(0x4);		//Try & work run.
	
	RxRead = READ_REG1(0xf0);
	RxRead = IOREAD(dwEthernetDataPort);
	RxRead = IOREAD(dwEthernetDataPort);


	if ((RxRead & 3) != 1)  /* no data */ 
		return 0;

	IOWRITE(dwEthernetIOBase, 0xf2);	/* set read ptr ++ */
	status = IOREAD16(dwEthernetDataPort);		/* get stat */
	rxlen = IOREAD16(dwEthernetDataPort);		/* get len */

//    EdbgOutputDebugString("DM9000: RX length byte = 0x%x.\r\n", rxlen);

//
	tmplen = (rxlen + 1) / 2;
	for (i = 0; i < tmplen; i++)
		((USHORT *)pbData)[i] = IOREAD16(dwEthernetDataPort);

	*pwLength = rxlen;
	
	return rxlen;	
}

/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
/*::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::*/
UINT16 
DM9000ASendFrame( BYTE *pbData, DWORD dwLength ) 
{
//	RETAILMSG(1, (TEXT("[DM9000A]: DM9000A_SendFrame()..........\r\n")));
	return 
		dm9000_send(pbData, (UINT16)dwLength);
}


/*
    @func   void | DM9000A_CurrentPacketFilter | Sets a receive packet h/w filter.
    @rdesc  N/A.
    @comm    
    @xref   
*/
void DM9000ACurrentPacketFilter(DWORD dwFilter)
{
	UCHAR uTemp;
	USHORT i, oft;

RETAILMSG(1, (TEXT("[DM9000A]: DM9000A_CurrentPacketFilter()..........\r\n")));		
	// What kind of filtering do we want to apply?
	//
	// NOTE: the filter provided might be 0, but since this EDBG driver is used for KITL, we don't want
	// to stifle the KITL connection, so broadcast and directed packets should always be accepted.
	//
	if (dwFilter & PACKET_TYPE_ALL_MULTICAST)
	{	// Accept *all* multicast packets.
		uTemp = READ_REG1(0x05);
		WRITE_REG1(0x05, uTemp | 0x08);		//Enable pass all multicast
	}

#if 0 	//Always can receive multicast address according to hash table.
	if (dwFilter & PACKET_TYPE_MULTICAST)
	{	// Accept multicast packets.
		
	}
#endif 

	if (dwFilter & PACKET_TYPE_BROADCAST)
	{
		/* broadcast address */
		hash_table[3] = 0x8000;
		/* Write the hash table to MAC MD table */
		for (i = 0, oft = 0x16; i < 4; i++) {
			WRITE_REG1(oft++, hash_table[i] & 0xff);
			WRITE_REG1(oft++, (hash_table[i] >> 8) & 0xff);
		}
		
	}
	
	// Promiscuous mode is causing random hangs - it's not strictly needed.
	if (dwFilter & PACKET_TYPE_PROMISCUOUS)
	{	// Accept anything.
		uTemp = READ_REG1(0x05);
		WRITE_REG1(0x05, uTemp | 0x02);		//Enable pass all multicast
	}

    EdbgOutputDebugString("DM9000: Set receive packet filter [Filter=0x%x].\r\n", dwFilter);


}	// DM9000A_CurrentPacketFilter().


/*
    @func   BOOL | DM9000A_MulticastList | Sets a multicast address filter list.
    @rdesc  TRUE = Success, FALSE = Failure.
    @comm    
    @xref   
*/
BOOL DM9000AMulticastList(PUCHAR pucMulticastAddresses, DWORD dwNumAddresses)
{
	BYTE nCount;
	BYTE nIndex;
	BYTE i, oft;
	BYTE Reg5;

	//Stop RX
	Reg5 = READ_REG1(0x05);
	WRITE_REG1(0x05, Reg5 & 0xfe);

	// Compute the logical address filter value.
	//
	for (nCount = 0 ; nCount < dwNumAddresses ; nCount++)
	{
        EdbgOutputDebugString("DM9000: Multicast[%d of %d]  = %x-%x-%x-%x-%x-%x\r\n",
                             (nCount + 1),
							 dwNumAddresses,
                             pucMulticastAddresses[6*nCount + 0],
                             pucMulticastAddresses[6*nCount + 1],
                             pucMulticastAddresses[6*nCount + 2],
                             pucMulticastAddresses[6*nCount + 3],
                             pucMulticastAddresses[6*nCount + 4],
                             pucMulticastAddresses[6*nCount + 5]);

		nIndex = CalculateHashIndex(&pucMulticastAddresses[6*nCount]);
        hash_table[nIndex/16]  |=  1 << (nIndex%16);
	}

	EdbgOutputDebugString("DM9000: Logical Address Filter = %x.%x.%x.%x.\r\n", hash_table[3], hash_table[2], hash_table[1], hash_table[0]);
	
	/* Write the hash table to MAC MD table */
	for (i = 0, oft = 0x16; i < 4; i++) {
		WRITE_REG1(oft++, hash_table[i] & 0xff);
		WRITE_REG1(oft++, (hash_table[i] >> 8) & 0xff);
	}

	//Start RX
	WRITE_REG1(0x05, Reg5);
	
    return(TRUE);

}	// DM9000A_MulticastList().

