/******************* (c) Marvell Semiconductor, Inc., 2006 ********************
 * File: wlan_thread.h
 * Purpose:
 *      This file contains definitions and data structures for creating a thread
 *
 *
 *****************************************************************************/
#ifndef _WLAN_THREAD_H
#define _WLAN_THREAD_H

#ifdef __cplusplus
extern "C"
{
#endif ///__cplusplus

#define DEFAULT_THREAD_PRIORITY		-1


typedef VOID (*THREADHANDLE)		(PVOID pContent);

HANDLE wlan_create_thread(THREADHANDLE threadHandle, PVOID pContext, int priority);
VOID wlan_destroy_thread(HANDLE);


///========================================================
/*
 Usage:
 
- Declaration:
	HANDLE				hEventThreadSleep;
	VOID 		My_Thread_Handler(IN PVOID pContext);
	
- To create a thread:
	hEventThreadSleep = wlan_create_thread(My_Thread_Handler, (VOID*)pParameter, DEFAULT_THREAD_PRIORITY);

- To Delete a thread:
	wlan_destroy_thread(hEventThreadSleep);

- Tasks of the thread:
	My_Thread_Handler() will be called once the thread is waken and the thread will go back sleeping after leaving
		this function

*/
///========================================================

#ifdef __cplusplus
}
#endif ///__cplusplus
#endif /// _WLAN_THREAD_H

