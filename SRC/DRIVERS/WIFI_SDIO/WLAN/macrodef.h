/******************* (c) Marvell Semiconductor, Inc., 2004 ********************
 *
 *
 *  Purpose:    This file contains the macro and symbol definitions
 *
 *  Notes:
 *
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/08/23 $
 *
 *	$Revision: #2 $
 *
 *****************************************************************************/

#ifndef _MACRODEF_H_
#define _MACRODEF_H_

/*
===============================================================================
            PUBLIC DEFINITIONS
===============================================================================
*/

//
//          Bit definition
//
#define DW_BIT_0        0x00000001
#define DW_BIT_1        0x00000002
#define DW_BIT_2        0x00000004
#define DW_BIT_3        0x00000008
#define DW_BIT_4        0x00000010
#define DW_BIT_5        0x00000020
#define DW_BIT_6        0x00000040
#define DW_BIT_7        0x00000080
#define DW_BIT_8        0x00000100
#define DW_BIT_9        0x00000200
#define DW_BIT_10       0x00000400
#define DW_BIT_11       0x00000800
#define DW_BIT_12       0x00001000
#define DW_BIT_13       0x00002000
#define DW_BIT_14       0x00004000
#define DW_BIT_15       0x00008000
#define DW_BIT_16       0x00010000
#define DW_BIT_17       0x00020000
#define DW_BIT_18       0x00040000
#define DW_BIT_19       0x00080000
#define DW_BIT_20       0x00100000
#define DW_BIT_21       0x00200000
#define DW_BIT_22       0x00400000
#define DW_BIT_23       0x00800000
#define DW_BIT_24       0x01000000
#define DW_BIT_25       0x02000000
#define DW_BIT_26       0x04000000
#define DW_BIT_27       0x08000000
#define DW_BIT_28       0x10000000
#define DW_BIT_29       0x20000000
#define DW_BIT_30       0x30000000
#define DW_BIT_31       0x80000000

#define W_BIT_0         0x0001
#define W_BIT_1         0x0002
#define W_BIT_2         0x0004
#define W_BIT_3         0x0008
#define W_BIT_4         0x0010
#define W_BIT_5         0x0020
#define W_BIT_6         0x0040
#define W_BIT_7         0x0080
#define W_BIT_8         0x0100
#define W_BIT_9         0x0200
#define W_BIT_10        0x0400
#define W_BIT_11        0x0800
#define W_BIT_12        0x1000
#define W_BIT_13        0x2000
#define W_BIT_14        0x4000
#define W_BIT_15        0x8000

#define B_BIT_0         0x01
#define B_BIT_1         0x02
#define B_BIT_2         0x04
#define B_BIT_3         0x08
#define B_BIT_4         0x10
#define B_BIT_5         0x20
#define B_BIT_6         0x40
#define B_BIT_7         0x80
#define B_BIT_8		0x100
#define B_BIT_9		0X200
#define B_BIT_10		0x400


/*
===============================================================================
            MACRO DEFINITIONS
===============================================================================
*/

//          Check if the FW has WPA enabled
#define FW_IS_WPA_ENABLED(_adapter) (_adapter->FWCapInfo & MRVDRV_FW_CAPINFO_WPA)

//
//          Allocate and release tagged memory
//
#define MRVDRV_ALLOC_MEM(_pbuffer, _length) NdisAllocateMemoryWithTag( \
		(PVOID *)(_pbuffer),(_length), 'LVRM')

#define MRVDRV_FREE_MEM(_buffer,_length) NdisFreeMemory((_buffer), (_length), 0)
#define MRVDRV_ZERO_MEM(_buffer,_length) NdisZeroMemory((PVOID *)(_buffer),(_length));

//
//          PCI memory mapped I/O access routines
//
#define MEM_WRITE_UCHAR(_base, _offset, _value)  NdisWriteRegisterUchar((PUCHAR)(((ULONG)_base)+_offset), (UCHAR)_value)
#define MEM_WRITE_USHORT(_base, _offset, _value) NdisWriteRegisterUshort((PUSHORT)(((ULONG)_base)+_offset), (USHORT)_value)
#define MEM_WRITE_ULONG(_base, _offset, _value)  NdisWriteRegisterUlong((PULONG)(((ULONG)_base)+_offset), (ULONG)_value)

#define MEM_READ_UCHAR(_base, _offset, _pBuf)  NdisReadRegisterUchar((PUCHAR)(((ULONG)_base)+_offset), _pBuf)
#define MEM_READ_USHORT(_base, _offset, _pBuf) NdisReadRegisterUshort((PUSHORT)(((ULONG)_base)+_offset), _pBuf)
#define MEM_READ_ULONG(_base, _offset, _pBuf)  NdisReadRegisterUlong((PULONG)(((ULONG)_base)+_offset), _pBuf)


//
//          Q data structures and useful Q operation macros. Right now Tx and command
//          routines are using these macros to queue and retrieve NDIS_PACKETs and 
//          commands.

//
//          NDIS miniport driver cast queued item to Q_NODE type and link it to the
//          queue maintained by Q_KEEPER
//
typedef struct _Q_NODE
{
    struct _Q_NODE *Next;
} Q_NODE, *PQ_NODE;

//
//          Q_KEEPER maintains the first and the last item on the queue
//
typedef struct _Q_KEEPER
{
    PQ_NODE First;
    PQ_NODE Last;
} Q_KEEPER, *PQ_KEEPER;

//
//          Q_KEEPER initialization macro
//
#define InitializeQKeeper(QKeeper)						\
	{                                                   \
        (QKeeper)->First = (QKeeper)->Last = NULL;		\
    }

//
//          Q_NODE push macro
//
#define InsertQNodeFromHead(QKeeper, QNode)				\
    {                                                   \
        ((PQ_NODE)QNode)->Next = (QKeeper)->First;		\
        (QKeeper)->First = (PQ_NODE)(QNode);			\
        if( (QKeeper)->Last == NULL )					\
            (QKeeper)->Last = (PQ_NODE)(QNode);			\
    }

//
//          Q_NODE queue macro
//
#define InsertQNodeAtTail(QKeeper, QNode)				\
    {                                                   \
        ((PQ_NODE)QNode)->Next = NULL;					\
        if( (QKeeper)->Last )                           \
            (QKeeper)->Last->Next = (PQ_NODE)(QNode);	\
        else                                            \
            (QKeeper)->First = (PQ_NODE)(QNode);        \
        (QKeeper)->Last = (PQ_NODE)(QNode);				\
    }

//
//          Q_NODE pop macro
//
#define PopFirstQNode(QKeeper)                          \
	(QKeeper)->First;                                   \
	{													\
        if( (QKeeper)->First )                          \
            (QKeeper)->First = (QKeeper)->First->Next;  \
        if( (QKeeper)->First == NULL )                  \
            (QKeeper)->Last = NULL;                     \
    }

#define PreviewFirstQNode(QKeeper)                      \
	(QKeeper)->First;

//
//          Q_KEEPER empty macro
//
#define IsQEmpty(QKeeper) ((QKeeper)->First == NULL)

//
//          S_SWAP : To swap 2 unsigned char
//
#define S_SWAP(a,b) do { unsigned char  t = SArr[a]; SArr[a] = SArr[b]; SArr[b] = t; } while(0)


typedef struct _TXQ_NODE
{
   PNDIS_PACKET        pPacket;
   SDIO_TX_PKT          DnldPacket;
   struct _TXQ_NODE    *next;
} TXQ_NODE,*PTXQ_NODE, **PPTXQ_NODE; 

typedef struct _TXQ_KEEPER
{
   PTXQ_NODE head;
   PTXQ_NODE tail;
}TXQ_KEEPER,*PTXQ_KEEPER,**PPTXQ_KEEPER; 

#define InitializeTxQKeeper(QKeeper)									\
{                                                     \
   (QKeeper)->head = (QKeeper)->tail = NULL;					\
} 


#define InsertTxQNodeFromHead(QKeeper, QNode)				\
    {                                                   \
        ((PTXQ_NODE)QNode)->next = (QKeeper)->head;		\
        (QKeeper)->head = (PTXQ_NODE)(QNode);			\
        if( (QKeeper)->tail == NULL )					\
            (QKeeper)->tail = (PTXQ_NODE)(QNode);			\
    }


#define IsTxQEmpty(QKeeper) ((QKeeper)->head == NULL)



#define InsertTxQNodeAtTail(QKeeper, QNode)						\
{                                  				            \
   ((PTXQ_NODE)QNode)->next = NULL;								    \
   if( (QKeeper)->tail )                        			\
       (QKeeper)->tail->next = (PTXQ_NODE)(QNode);		\
   else                                         			\
       (QKeeper)->head = (PTXQ_NODE)(QNode);    			\
   (QKeeper)->tail = (PTXQ_NODE)(QNode);							\
}


#define PopFirstTxQNode(QKeeper)                     \
	(QKeeper)->head;                                   \
	{													 												 \
        if( (QKeeper)->head )                        \
            (QKeeper)->head = (QKeeper)->head->next; \
        if( (QKeeper)->head == NULL )                \
            (QKeeper)->tail = NULL;                  \
  }


#define PopFreeTxQNode(QNode)				\
  (PTXQ_NODE)QNode;                 \
  {                                 \
    QNode = QNode->next;            \
  }
                    

#define PushFreeTxQNode(HeadNode, FreeNode)			             \
 {                                                           \
  ((PTXQ_NODE)FreeNode)->next = (PTXQ_NODE)HeadNode;         \
  ((PTXQ_NODE)FreeNode)->pPacket = NULL;                     \
  (PTXQ_NODE)HeadNode = (PTXQ_NODE)FreeNode;                 \
 }                                                           \

#endif /* _MACRODEF_H_ */


