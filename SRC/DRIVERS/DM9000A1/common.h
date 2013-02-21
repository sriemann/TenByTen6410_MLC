/********************************************************************************
 * 
 * $Id: common.h,v 1.1.1.1 2007/04/16 03:45:52 bill Exp $
 *
 * File: Common.h
 *
 * Copyright (c) 2000-2002 Davicom Inc.  All rights reserved.
 *
 ********************************************************************************/


#ifndef	__COMMON_H__
#define	__COMMON_H__


#include	"dm_types.h"


/*******************************************************************************
 *
 * Debug macros
 *
 *******************************************************************************/
#if	DBG
#define	HALT(x)	if(x) PUTS(("FATAL ERROR\n"))
#define	BREAK	__asm int 3
#define	PUTS(x)	NKDbgPrintfW(TEXT x)
#define	MSG(x)	NKDbgPrintfW x
#define	DEBUG_PRINT(msg)	NKDbgPrintfW msg
#else
#define	HALT(x)
#define	BREAK
#define	PUTS(x)
#define	MSG(x)
#define	DEBUG_PRINT(msg)
#endif

/****************************************************************************************
 *
 * simple exception
 *
 ****************************************************************************************/
#define	MAX_EXCEPTIONS	256
#define	ERR_STRING(p)	(PU8)(p)

class C_Exception;
typedef	struct	_EXCEPTION_DATA
{
	int			nIndex;
	jmp_buf		szJumps[MAX_EXCEPTIONS];
	C_Exception		*szpExceptions[MAX_EXCEPTIONS];
}	EXCEPTION_DATA,	*PEXCEPTION_DATA;

extern PEXCEPTION_DATA	_gpExpData;

class C_Exception
{
public:
	C_Exception::C_Exception(void)
		{ Constructor((PU8)"N/A", NDIS_STATUS_FAILURE); }
		
	C_Exception::C_Exception(U32 uErr)
		{ Constructor((PU8)"N/A", uErr); }
		
	C_Exception::C_Exception(PU8 ptrErr, U32 uErr=NDIS_STATUS_FAILURE)
		{ Constructor(ptrErr, uErr); }

	
	C_Exception::~C_Exception(void)
	{
		NdisFreeString(m_ndisErrorMessage);
	};

	void Constructor(PU8 ptrErr, U32 uError)
	{
		m_uErrorCode = uError;
		NdisInitializeString(&m_ndisErrorMessage,ptrErr);
	};
	
	void	PrintErrorMessage(void)
	{
		MSG((
			TEXT("DM9 Exception Level(%d)"),_gpExpData->nIndex+1));
		MSG((m_ndisErrorMessage.Buffer));
	};
	
	U32		GetErrorCode(void) { return m_uErrorCode; };
	
private:
	U32				m_uErrorCode;
	NDIS_STRING		m_ndisErrorMessage;
	
};

#define	INIT_EXCEPTION()	(_gpExpData = (PEXCEPTION_DATA)malloc(sizeof(EXCEPTION_DATA)) \
	, memset((void*)_gpExpData,0,sizeof(EXCEPTION_DATA)),_gpExpData->nIndex = -1)

#define	TERM_EXCEPTION()	free(_gpExpData)

#define	TRY	if(setjmp(_gpExpData->szJumps[++_gpExpData->nIndex]) == 0)

#define	FI --(_gpExpData->nIndex)

#define	CATCH(exp) \
	else if(((exp) = _gpExpData->szpExceptions[_gpExpData->nIndex--]))

#define	THROW(x) \
	( _gpExpData->szpExceptions[_gpExpData->nIndex] = new C_Exception x, \
	longjmp(_gpExpData->szJumps[_gpExpData->nIndex],-1))

#define	CLEAN(x)	delete x

/****************************************************************************************
 *
 * Spinlock definition and implementation
 *
 ****************************************************************************************/
class CSpinlock
{

public:
	CSpinlock::CSpinlock(void)
	{
		memset((void*)&m_SpinLock,0,sizeof(m_SpinLock));
		NdisAllocateSpinLock(&m_SpinLock);
	};

	CSpinlock::~CSpinlock(void)
	{
		NdisFreeSpinLock(&m_SpinLock);
	};

	void	Lock(void)
	{
		NdisAcquireSpinLock(&m_SpinLock);
	};

	void	Release(void)
	{
		NdisReleaseSpinLock(&m_SpinLock);
	};

private:
	NDIS_SPIN_LOCK	m_SpinLock;
};
	
/****************************************************************************************
 *
 * Mutex definition and implementation
 *
 ****************************************************************************************/

#define	WAIT_FOREVER	((U32)-1)

#ifdef	IMPL_NO_CRITICAL_SECTION
#error no critical section
void fn(void)
{
	CRITICAL_SECTION	cs;
	InitializeCriticalSection(&cs);
	TryEnterCriticalSection(&cs);
	LeaveCriticalSection(&cs);
}

class	CMutex
{
public:
	CMutex::CMutex(void)
	{
		memset((void*)&m_SpinLock,0,sizeof(m_SpinLock));
		NdisAllocateSpinLock(&m_SpinLock);
		m_Value = 0;
	};

	CMutex::~CMutex(void)
	{
		NdisFreeSpinLock(&m_SpinLock);
	};

	int	TryLock(void)
	{
		int		ret=1;
		NdisAcquireSpinLock(&m_SpinLock);
		if(m_Value)
			ret = 0;
		else
			m_Value++;
		NdisReleaseSpinLock(&m_SpinLock);
		return ret;
	};
	
	int	Lock(U32	uMicroSeconds=WAIT_FOREVER, U32 uMicroPeriod=1000)
	{
		U32		remains=uMicroSeconds;
		U32		waited;
	
		for(;!TryLock();remains-=waited)
		{
			if(!remains) return 0;
			NdisMSleep((waited=MINI(uMicroPeriod,remains)));
		}
		return 1;
	};
	
	void	Release(void)
	{
		NdisAcquireSpinLock(&m_SpinLock);
		m_Value = 0;
		NdisReleaseSpinLock(&m_SpinLock);
	};

private:
	
	NDIS_SPIN_LOCK	m_SpinLock;
	int				m_Value;
};
#else

class	CMutex
{
public:
	CMutex::CMutex(void)
	{
		memset((void*)&m_Critical,0,sizeof(m_Critical));
		InitializeCriticalSection(&m_Critical);
	};

	CMutex::~CMutex(void)
	{
		DeleteCriticalSection(&m_Critical);
	};

	int	TryLock(void)
	{
		return TryEnterCriticalSection(&m_Critical);
	};
	
	int	Lock(U32 uMicroSeconds=WAIT_FOREVER, U32 uMicroPeriod=1000)
	{
		EnterCriticalSection(&m_Critical);
		return 1;
	};
	
	void	Release(void)
	{
		LeaveCriticalSection(&m_Critical);
	};

private:
	CRITICAL_SECTION	m_Critical;
};

#endif
/****************************************************************************************
 *
 * Queue definition and implementation
 *
 ****************************************************************************************/
typedef	struct	_CQUEUE_GEN_HEADER
{
	struct	_CQUEUE_GEN_HEADER	*pNext;
	U32		uFlags;
	PVOID	pPacket;
	U16		nReserved;
	U16		nLength;
} CQUEUE_GEN_HEADER, *PCQUEUE_GEN_HEADER;

#define	CQueueGetUserPointer(ptr)	\
	((PVOID)((U32)(ptr) + sizeof(CQUEUE_GEN_HEADER)))
	
class CQueue
{
public:
	CQueue::CQueue(void)
	{
		m_pHead = m_pTail = NULL;
	};
	

	void Enqueue(PCQUEUE_GEN_HEADER	pObject)
	{
		HALT(!pObject);

		m_spinLock.Lock();
		pObject->pNext = NULL;
		if(m_pTail == NULL)
		{
			m_pHead = m_pTail = pObject;
		}
		else	
		{
			m_pTail->pNext = pObject;
			m_pTail = pObject;
		}
		m_spinLock.Release();
	};

	PCQUEUE_GEN_HEADER	Dequeue(void)
	{
		PCQUEUE_GEN_HEADER	pcurr = m_pHead;
		m_spinLock.Lock();
		if(m_pHead != NULL) 
		{
			if(m_pHead == m_pTail)
				m_pHead = m_pTail = NULL;
			else
				m_pHead = m_pHead->pNext;
		}
		m_spinLock.Release();
		return pcurr;
	};

	PCQUEUE_GEN_HEADER	GetHead(void) 
	{
		return m_pHead;
	};

	int	IsQueueEmpty(void) { return (m_pHead == NULL); };

	int	Size(void) 
	{
		int	size;
		PCQUEUE_GEN_HEADER	pcurr;
		
		for(size=0,pcurr=m_pHead;
			pcurr;
			pcurr=pcurr->pNext,size++) ;
			
		return size;
	};
	
private:
	CSpinlock			m_spinLock;
	CQUEUE_GEN_HEADER	*m_pHead,*m_pTail;
};
#endif
