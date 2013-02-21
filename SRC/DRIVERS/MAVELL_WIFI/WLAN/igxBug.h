/******************* (c) Marvell Semiconductor, Inc., *************************
 *
 *
 *  Purpose:    Debugging macros
 *
 *
 *	$Author: schiu $
 *
 *	$Date: 2004/10/27 $
 *
 *	$Revision: #3 $
 *
 *****************************************************************************/

#ifndef _IXBUG_H_
#define _IXBUG_H_

 //Added by ice.zhang for debug message

 
#if 0
#define DBG_MSG_TO_RETAILMSG  
#define MRVL_PRINT_DBG_MSG 1
#endif /**/

#ifndef DEFINE_DEBUG_VARS
extern ULONG DebugLevel;                    // this must be defined somewhere in drvr.
extern unsigned long long DebugModule;
#endif



#define DBBASE          0x378               // base address for debug strobe output for analyzer snooping
#define DBLINE1         0x01                // bit 0 is parallel port pin 1
#define DBLINE2         0x02                // bit 1 is parallel port pin 14
#define DBLINE3         0x04                // bit 2 is parallel port pin 16
#define DBLINE4         0x08                // bit 3 is parallel port pin 17

//Define debug Level
#define DBG_NONE        0x0                 // 
#define DBG_ERROR       (1<<0)              // Debug for ERRORS
#define DBG_WARNING     (1<<1)              // Debug for Warnings.
#define DBG_HELP        (1<<2)              // Debug for driver load   

//Define debug module
#define DBG_LOAD        (1<<3)
#define DBG_UNLOAD      DBG_LOAD            // Debug for driver unload.
#define DBG_ISR         (1<<4)              // Debug for ISR messages.
#define DBG_WMM         (1<<5)
#define DBG_TX          (1<<6)              // Transmit data path debug
#define DBG_RX          (1<<7)              // Receive data path debug.
#define DBG_MACEVT      (1<<8)              // Debug MAC events.
#define DBG_CMD         (1<<9)              // Debug fw command/responses.
#define DBG_PS          (1<<10)             // Debug for power save option.
#define DBG_ADHOC       (1<<11)             // for ad hoc mode
#define DBG_TMR         (1<<12)             // Debug timers.
#define DBG_SCAN        (1<<13)             // Debug scan operation.
#define DBG_CCX         (1<<14)             // Debug CCX
#define DBG_WEP         (1<<15)             // security related debug  
#define DBG_HOSTSLEEP   (1<<16)             // debug hostslelep, deepsleep, D0<->D3
#define DBG_DEEPSLEEP   DBG_HOSTSLEEP
#define DBG_BUF         (1<<17)             // debug buffer, memory, resource management
#define DBG_ASSO        (1<<18)             // debug association,reconnect.
#define DBG_OID         (1<<19)             // Debug OIDS
#define DBG_RSSI        (1<<20)             // Debug RSSI
#define DBG_GPIOINT     (1<<21)             // Debug GPIO interrupt
#define DBG_V9          (1<<22)             // Debug porting to V9 FW
#define DBG_RECONNECT   (1<<23)             // Debug Reassociate
#define DBG_WPA         (1<<24) 
#define DBG_THREAD      (1<<25)             // Debug thread module
#define DBG_CCX_CCKM    (1<<26)
#define DBG_CCX_V4      (1<<27)
#define DBG_CUSTOM      (1<<28)             // Debug custom message
#define DBG_STHRD       (1<<29)             // Debug Status indication thread
#define DBG_MEF_CFG     (1<<30)
#define DBG_FW          (1<<31)             // For SMALL_DEBUG

#define DBG_11D         (1LL<<33)
#define DBG_PNP         (1LL<<34)           // Plug-n-play related debug.
#define DBG_DATARATE    (1LL<<35)           // Link speed/data rate debug.
#define DBG_POWER       (1LL<<36)           // debug power management.
#define DBG_NEWCMD      (1LL<<37)           // debug new command structure
#define DBG_ALLEN       (1LL<<38)       
#define DBG_QOS         (1LL<<39)           // 
#define DBG_CRLF        (1LL<<40)           // add some cr lfs before this message.
#define DBG_IOCTL       (1LL<<41)
#define DBG_OID_OPT     (1LL<<42)           // For Wireless Optimizer OID
#define DBG_GPM         (1LL<<43)           // For Wireless Optimizer OID
#define DBG_ROAM        (1LL<<44)             // debug roaming issue 
#define DBG_TMP         (1LL<<45)           //tmp message

#define DBG_USER0       (1LL<<63)

#define DBG_ALL         0xffffffffffffffffLL         // All debug enabled.


#define DBG_LVLDEFAULT (DBG_ERROR \
| DBG_WARNING \
| DBG_HELP)

#define DBG_MDLDEFAULT (DBG_ROAM)


NTHALAPI
LARGE_INTEGER
KeQueryPerformanceCounter (
   OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
   );

#ifdef MRVL_PRINT_DBG_MSG


#define DBGSTROBE_LINE_ON( line )
#define DBGSTROBE_LINE_OFF( line )
#define INITDEBUG()



#ifdef DBG_MSG_TO_RETAILMSG 
       void MrvRETAILMSG(const wchar_t *fmt, ...);   
#define DBGPRINT(lvl, Str)\
	{\
		unsigned long long __lvl = lvl;\
		if ( (__lvl & DBG_ERROR) ||(__lvl &(DBG_CMD|DBG_RX|DBG_TX|DBG_OID|DBG_ISR|DBG_SCAN|DBG_ASSO))  )\
		{\
			MrvRETAILMSG Str;\
		}\
	}
#else //DBG_MSG_TO_RETAILMSG
	void MrvPrintFile(const unsigned short *fmt, ...);
#define DBGPRINT(lvl, Str)\
        {\
		unsigned long long __lvl = lvl;\
		if ( 1/*(__lvl & DBG_ERROR) || ((DebugLevel & __lvl) && (DebugModule & __lvl))|| (( (__lvl & (DBG_ERROR|DBG_WARNING|DBG_HELP)) == DBG_NONE) && (DebugModule & __lvl)) */)\
		{\
			MrvPrintFile Str;\
		}\
	}
   

#endif //DBG_MSG_TO_RETAILMSG
 
#define MRV_DRV_LINE_LENGTH 16
    static __inline
	void HexDump ( unsigned long long dbgLevel, char *msg, PUCHAR buf, int len)
	{
	  int ii = 0, jj;
	  UCHAR lastBuf[MRV_DRV_LINE_LENGTH];

	  if ( msg ) DBGPRINT(dbgLevel,(L"%S\r\n",msg));

	  // new version that prints out correctly in dbgview
	  jj = len / MRV_DRV_LINE_LENGTH;
	  for (ii = 0; ii < jj; ii ++ )
	  {
	      // note: if MRV_DRV_LINE_LENGTH changes, the following format
	      //       needs to change as well
	      DBGPRINT(dbgLevel, (L"%02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
	                          (ULONG)(LONG)(UCHAR)*buf,
	                          (ULONG)(LONG)(UCHAR)*(buf+1),
	                          (ULONG)(LONG)(UCHAR)*(buf+2),
	                          (ULONG)(LONG)(UCHAR)*(buf+3),
	                          (ULONG)(LONG)(UCHAR)*(buf+4),
	                          (ULONG)(LONG)(UCHAR)*(buf+5),
	                          (ULONG)(LONG)(UCHAR)*(buf+6),
	                          (ULONG)(LONG)(UCHAR)*(buf+7),
	                          (ULONG)(LONG)(UCHAR)*(buf+8),
	                          (ULONG)(LONG)(UCHAR)*(buf+9),
	                          (ULONG)(LONG)(UCHAR)*(buf+10),
	                          (ULONG)(LONG)(UCHAR)*(buf+11),
	                          (ULONG)(LONG)(UCHAR)*(buf+12),
	                          (ULONG)(LONG)(UCHAR)*(buf+13),
	                          (ULONG)(LONG)(UCHAR)*(buf+14),
	                          (ULONG)(LONG)(UCHAR)*(buf+15)));
	      buf+= MRV_DRV_LINE_LENGTH;
	  }

	  jj = len % MRV_DRV_LINE_LENGTH;

	  if ( jj != 0 )
	  {
	      for ( ii = 0; ii < jj; ii++ )
	      {
	          lastBuf[ii] = *(buf+ii);
	      }

	      for ( ii=jj; ii < MRV_DRV_LINE_LENGTH; ii++ )
	      {
	          lastBuf[ii] = 0;
	      }

	      DBGPRINT(dbgLevel, (L"%02x %02x %02x %02x %02x %02x %02x %02x - %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
	                              (ULONG)(LONG)(UCHAR)lastBuf[0],
	                              (ULONG)(LONG)(UCHAR)lastBuf[1],
	                              (ULONG)(LONG)(UCHAR)lastBuf[2],
	                              (ULONG)(LONG)(UCHAR)lastBuf[3],
	                              (ULONG)(LONG)(UCHAR)lastBuf[4],
	                              (ULONG)(LONG)(UCHAR)lastBuf[5],
	                              (ULONG)(LONG)(UCHAR)lastBuf[6],
	                              (ULONG)(LONG)(UCHAR)lastBuf[7],
	                              (ULONG)(LONG)(UCHAR)lastBuf[8],
	                              (ULONG)(LONG)(UCHAR)lastBuf[9],
	                              (ULONG)(LONG)(UCHAR)lastBuf[10],
	                              (ULONG)(LONG)(UCHAR)lastBuf[11],
	                              (ULONG)(LONG)(UCHAR)lastBuf[12],
	                              (ULONG)(LONG)(UCHAR)lastBuf[13],
	                              (ULONG)(LONG)(UCHAR)lastBuf[14],
	                              (ULONG)(LONG)(UCHAR)lastBuf[15]));
	  }      
	}
 
#else  //MRVL_PRINT_DBG_MSG
#define DBGPRINT(lvl, Str)
#define DEBUGCHKZONE(x)
#define HexDump(dbglevel, msg, buf, size)
#endif  //MRVL_PRINT_DBG_MSG


#endif  



