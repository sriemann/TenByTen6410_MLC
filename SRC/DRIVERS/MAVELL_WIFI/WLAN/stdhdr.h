/******************* © Marvell Semiconductor, Inc., 2003 ********************
 *
 *  Purpose:    Defines commonly used headers
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *
 *****************************************************************************/

#define ETH_ADDR_LENGTH 6

typedef char ETH_ADDR[ETH_ADDR_LENGTH];

typedef struct _ETH_HEADER {

	ETH_ADDR DestAddr;
	ETH_ADDR SrcAddr;

	USHORT Type;	// type or length
} ETH_HEADER, *PETH_HEADER;

