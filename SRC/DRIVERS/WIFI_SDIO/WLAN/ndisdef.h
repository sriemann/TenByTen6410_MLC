/*****************************************************************************
 *
 *  Purpose: 
 *
 *      This file contains Microsoft NDIS 5.1 802.11 macros and data 
 *      structures. Borrowed from Windows XP DDK
 *
 *  $Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $   	    
 *
 *****************************************************************************/

#ifndef _NDIS_DEF_H_
#define _NDIS_DEF_H_


#define ETH_ADDR_LENGTH 6

typedef char ETH_ADDR[ETH_ADDR_LENGTH];

typedef struct _ETH_HEADER {

	ETH_ADDR DestAddr;
	ETH_ADDR SrcAddr;

	USHORT Type;	// type or length
} ETH_HEADER, *PETH_HEADER;

#define B_SUPPORTED_RATES		8
#define BASIC_SUPPORTED_RATES	8
#define G_SUPPORTED_RATES		14
#define NDIS_SUPPORTED_RATES	G_SUPPORTED_RATES


#endif

