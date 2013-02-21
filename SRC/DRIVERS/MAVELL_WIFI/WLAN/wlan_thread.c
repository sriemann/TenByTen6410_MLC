/******************* (c) Marvell Semiconductor, Inc., 2006 ********************
 * File: wlan_thread.c
 * Purpose:
 *      This file contains implementation for creating/destroy a thread
 *
 *
 *****************************************************************************/
#include "precomp.h"
#include "wlan_thread.h"

typedef enum {
	THREAD_NOT_INITED,
	THREAD_RUNNING,
	THREAD_ENDED
} THREAD_STATUS;

typedef struct _thread_param
{
	BOOLEAN						EndOfThread;		///End of thread
	HANDLE						hThread;			///handle of the roaming thread 
	HANDLE						hEvent;				///event of the roaming thread (to wakeup the thread)
	HANDLE						syncthread;			///Synchronized with the thread
	int							priority;				///Thread priority
	BOOLEAN						isThreadEnded;		///Is the thread ended

	THREADHANDLE				threadHandleBody;
	PVOID						pContext;
} THREAD_PARAM, *PTHREAD_PARAM;

#define			MAX_THREADLIST		10
static THREAD_PARAM		threadlist[MAX_THREADLIST];
static int					theadlistsize = 0;

static VOID threadHandle(IN PVOID pContext);

HANDLE wlan_create_thread(THREADHANDLE threadHandleBody, PVOID pContext, int priority)
{
	HANDLE				retval = NULL;
	DWORD				threadID;
	PTHREAD_PARAM		pparam = NULL;

	
	if (theadlistsize >= MAX_THREADLIST) {
		DBGPRINT(DBG_THREAD|DBG_ERROR,(L"->Thread: Exceed the thread capacity (%d)\n", theadlistsize));
		goto funcFinal;
	}
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Creating a thread\n", (theadlistsize)));	
	pparam = &threadlist[theadlistsize ++];

	pparam->EndOfThread = FALSE;
	pparam->hEvent = CreateEvent(NULL, FALSE, FALSE,NULL);
	pparam->syncthread = CreateEvent(NULL, FALSE, FALSE,NULL);
	pparam->threadHandleBody = threadHandleBody;
	pparam->pContext = pContext;
	pparam->priority = priority;
	pparam->hThread =  CreateThread(NULL,
                                        0,
                                        (LPTHREAD_START_ROUTINE)threadHandle,
                                        pparam,
                                        0,
                                        &threadID);
	pparam->isThreadEnded = FALSE;
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Waiting for thread to be initialized\n"));	
	WaitForSingleObject(pparam->syncthread, INFINITE);
	retval = pparam->hEvent;
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Thread (%d) is created successfully\n", (theadlistsize-1)));
funcFinal:
	return retval;
}

VOID wlan_destroy_thread(HANDLE event)
{
	int	i;
	PTHREAD_PARAM	pparam = NULL;
	
	for (i=0 ; i<theadlistsize ; i++) {
		if (threadlist[i].hEvent == event) {
			pparam = &threadlist[i];
			break;
		}
	}
	if (i == MAX_THREADLIST) {
		DBGPRINT(DBG_THREAD|DBG_WARNING,(L"->Thread: wlan_destroy_thread: No available thread to destroy\n"));
		goto funcFinal;
	}

	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: wlan_destroy_thread: destroying (%d)th thread in %d\n", i, theadlistsize));
	///Destory the thread
	if (pparam->EndOfThread == FALSE) {
		///Terminate the thread
		pparam->EndOfThread = TRUE;
		SetEvent(pparam->hEvent);

		//wait until the thread is terminated
		DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Waiting for thread to be terminated\n"));
		while (pparam->isThreadEnded == FALSE) {
			NdisMSleep(1);
		}
		NdisMSleep(1);
		DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Thread is ended. Close the handler now\n"));

		CloseHandle(pparam->hEvent);
                CloseHandle(pparam->hThread);
		CloseHandle(pparam->syncthread);
	} else {
		DBGPRINT(DBG_THREAD|DBG_WARNING,(L"->Thread: <Warning> wlan_destroy_thread: Thread had been ended \n"));
	}


	///Reorganize the list;
	if (theadlistsize > 1) {
		NdisMoveMemory(pparam, &threadlist[theadlistsize-1], sizeof(THREAD_PARAM));
		NdisZeroMemory(&threadlist[theadlistsize-1], sizeof(THREAD_PARAM));
	}
	theadlistsize --;
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: wlan_destroy_thread: Thread is destroyed successfully\n"));
funcFinal:
	return;
}


static VOID threadHandle(IN PVOID pContext)
{
	PTHREAD_PARAM		pparam = (PTHREAD_PARAM)pContext;

	pparam->isThreadEnded = FALSE;
	SetEvent(pparam->syncthread);
	///pparam->ThreadStatus = THREAD_RUNNING;
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Thread is running\n"));
	if (pparam->priority != DEFAULT_THREAD_PRIORITY) {
		CeSetThreadPriority(GetCurrentThread(), pparam->priority); 
	}
	
	while (pparam->EndOfThread == FALSE ) {
		WaitForSingleObject(pparam->hEvent, INFINITE);
		if (pparam->EndOfThread == TRUE) {
			DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Thread is ending. Leave the loop now\n"));
			break;
		}
		///Call the real body of the thread handle
		if (pparam->threadHandleBody) {
			pparam->threadHandleBody(pparam->pContext);
		}
	}

	///SetEvent(pparam->syncthread);
	pparam->isThreadEnded = TRUE;
	DBGPRINT(DBG_THREAD|DBG_HELP,(L"->Thread: Thread is terminiated\n"));
	return;
}


