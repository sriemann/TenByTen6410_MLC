#include <windows.h>
#define PSII_VERSION				TEXT("[PocketStoreII] v1.5.1 RC1 (31/MAR/2006)\r\n")

// Debugging message
#define PSII_DBGMSG_ENABLE
#if defined(PSII_DBGMSG_ENABLE)
	#define	PSII_RTL_PRINT(x)		RETAILMSG(1, x)
	#define	PSII_DBG_PRINT(x)		RETAILMSG(1, x)
#else
	#define	PSII_RTL_PRINT(x)		RETAILMSG(1, x)
	#define	PSII_DBG_PRINT(x)		RETAILMSG(1, x)
#endif


//OSLess.cpp
#define OSLESS_MALLOC_POOL_SIZE_BY_KB	4000
#undef OSLESS_USE_STDLIB
#undef OAM_32BIT_MEMCPY_OK


//PAM.cpp
#undef USE_VLLD


#undef LLD_STRICT_CHK					// For debugging
#undef LLD_ASYNC_MODE					// We need interrupt signal for using async mode
#ifdef  LLD_ASYNC_MODE
	#define LLD_DEFERRED_CHK			// Should be set defined
#else /* SYNC_MODE */
	#define  LLD_DEFERRED_CHK			// User Can Modify it
#endif
#undef LLD_CACHED_READ					// Deprecated - we can use ONLD_MRead()
#define LLD_ALIGNED_MEMCPY				// For better performance
#undef LLD_SYNC_BURST_READ				// Option
#undef LLD_OTP_UID						// OTP read enable
#define LLD_OTP_PBN						1000 // OTP block nPbn (should be in unlocked region)


//HALWrapper.c
#define HALWP_ERR_MSG_ON		1
#define HALWP_LOG_MSG_ON		0
#define HALWP_INF_MSG_ON		0

//PseudoFTL.c
#define FTLP_ERR_MSG_ON		1
#define FTLP_LOG_MSG_ON		0
#define FTLP_INF_MSG_ON		0

//PseudoVFL.c
#define VFLP_ERR_MSG_ON		1
#define VFLP_LOG_MSG_ON		0
#define VFLP_INF_MSG_ON		0

//ondisk.c
#define STDRV_ERR_MSG_ON		1
#define STDRV_LOG_MSG_ON		0
#define STDRV_INF_MSG_ON		0

