//
// Copyright (c) Samsung Electronics. Co. LTD.  All rights reserved.
//
/*++
THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
PARTICULAR PURPOSE.
*/

#include <COMMON/fimg_debug.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include "register.h"
#include "macros.h"
#include "fgl.h"
#include "platform.h"

#include <windows.h>

#include <bsp.h>
#include "DrvLib.h"
#include "s3c6410_base_regs.h"
#include "bsp_cfg.h"
#include "fimgdrv.h"
#include <pmplatform.h>

extern int            g_InitDone;
extern HANDLE            g_hPwrControl;
static HANDLE               g_hMapping;

#define    FIMG_PHY_BASE            S3C6410_BASE_REG_PA_FIMG_3DSE
#define FIMG_PHY_SIZE            0x90000

#define ALLOCATESIZE            0x200000
#define ALLOCATENUM            8

/* Type, Structure & Class Definitions */
struct s3c_3d_mem_alloc
{
    int             size;
    unsigned int vir_addr;
    unsigned int phy_addr;
};


typedef struct ALLOCMEM_POOL {
    void*                   phyAddr;
    void*                   virAddr;
    HANDLE                  handle;
    BOOL                    used;
    void*                   openHandle;
    int                     size;
    struct ALLOCMEM_POOL   *next;
} ALLOCMEM_POOL;

ALLOCMEM_POOL        *pHeadOfMemoryPool = NULL;
ALLOCMEM_POOL       *pDepthBuffer = NULL;

int g_blkSize = 0;
int g_blkNum = 0;
int g_depthBufferSize = 0;

// Pool memory
AddressBase gPoolMem = {
    (void*) 0, NULL
};

// FIMG registers
AddressBase gFimgBase = {
    (void*)FIMG_PHY_BASE, NULL
};

static int fimg_initcount = 0;

#if 0
static volatile S5PC100_DMAC_REG    *g_pPDMAC0Reg = NULL;
static volatile S5PC100_DMAC_REG    *g_pPDMAC1Reg = NULL;
static volatile S5PC100_DMAC_REG    *g_pMDMACReg = NULL;
static volatile S5PC100_SYSCON_CLK_REG    *g_pSysConReg = NULL;
#endif

BOOL ReadMemoryPoolFromRegistry(int *pBlkNum, int *pBlkSize, int *pDepthBufferSize);
void FimgPowerOn(BOOL isOn);
BOOL AllocateMemoryList();
//void FreeMemoryList();


/***********************************************************************************
 Function Name            : InitFimg
 Inputs                             : None
 Outputs                        : None
 Returns                        : None
 Description                : This function initializes the graphics hardware.
************************************************************************************/
extern "C" void InitFimg ()
{
    unsigned int GPU_version = 0;
    
        fglGetVersion(&GPU_version);
        Plat::printf("\n\n\n  *** FIMG VERSION : 0x%x ***   \n\n\n",GPU_version);
    
        fglSoftReset();

    WRITEREG(FGPF_STENCIL_DEPTH_MASK, 0);
    WRITEREG(FGRA_PIXEL_SAMPOS ,FGL_SAMPLE_CENTER);
}


/***********************************************************************************
 Function Name            : InitDevice_wince
 Inputs                             : primary and secondary surface pointers
 Outputs                        : None
 Returns                        : opengl es 2.0 contexts
 Description                : This is the starting function of OpenGL ES 2.0 which maps the FIMG registers,
                         set the framebuffer structure parameters, allocates a pool of memory and 
                         creates a gl context.
************************************************************************************/
extern "C" BOOL GLES2Initdriver ()
{   

    DWORD dwPhys = 0;
#if (_WIN32_WCE >= 600)
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
#endif    
        
    Plat::printf("[FIMGDRV] InitDriver\n");
    //-------------------------------------------------------------------------
    // Map FIMG SFRs
    //-------------------------------------------------------------------------

    g_hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, FIMG_PHY_SIZE, NULL);
    if(g_hMapping == NULL)
    {
        RETAILMSG (1,(TEXT("CreateFileMapping error\r\n")));
        return false;
    }
    
    gFimgBase.vaddr = (void *)MapViewOfFile(g_hMapping, FILE_MAP_WRITE, 0, 0, 0);    
    if (gFimgBase.vaddr < 0)
    {
        RETAILMSG (1,(TEXT("gFimgBase.vaddr not mapped\r\n")));
        return false;
    }

    //if(!VirtualCopy (pVaShared, virtaddr, szBlock, PAGE_NOCACHE|PAGE_READWRITE))
    if(!VirtualCopy (gFimgBase.vaddr, (LPVOID)((UINT32)FIMG_PHY_BASE>>8), FIMG_PHY_SIZE, PAGE_NOCACHE|PAGE_READWRITE|PAGE_PHYSICAL))
    {
        RETAILMSG (1,(TEXT("VirtualCopy error\r\n")));
        return false;    
    }        
    
    if(!ReadMemoryPoolFromRegistry(&g_blkNum, &g_blkSize, &g_depthBufferSize))
    {
        g_blkSize = ALLOCATESIZE;
        g_blkNum = ALLOCATENUM;
    }  
    
    
    AllocateMemoryList();

    return true;
}

extern "C" BOOL GLES2Opendriver ()
{   
//    RETAILMSG(1,(TEXT("WinCE Version = %d\n"), _WIN32_WCE));
    
    EnterCriticalSection(&gles20_open_mutex);
    if(fimg_initcount == 0)
    {
        
        //-------------------------------------------------------------------------
        // Graphics hardware (FIMG) initialization
        //-------------------------------------------------------------------------
    
        FimgPowerOn(TRUE);
        
        InitFimg();
    
        g_InitDone = TRUE;
    }

        
    fimg_initcount++;
    LeaveCriticalSection(&gles20_open_mutex);

    return TRUE;
}


/***********************************************************************************
 Function Name            : CloseDevice
 Inputs                             : None
 Outputs                        : None
 Returns                        : None
 Description                : This fucntion unmap the FIMG registers, pool memory.
************************************************************************************/
extern "C" BOOL GLES2DeInitdriver(void)
{    
        // Unmap any memory areas that we may have mapped.
    if (gFimgBase.vaddr)
    {
        CloseHandle(g_hMapping);
        gFimgBase.vaddr = NULL;
    }
    return true;
}


extern "C" BOOL GLES2Closedriver(void)
{    
    EnterCriticalSection(&gles20_open_mutex);
    fimg_initcount--;

    if(fimg_initcount == 0)
       {    
        
        g_InitDone = FALSE;
        
        FimgPowerOn(FALSE);
    }
    LeaveCriticalSection(&gles20_open_mutex);
    
    return TRUE;
}

extern "C"  void GetPhysicalAddress(BufferAddress* bufAddr)
{
       DWORD dwPhys = 0;       
       //BufferAddress *bufAddr = (BufferAddress *) MapCallerPtr( (LPVOID)unMappedbufAddr, sizeof(BufferAddress));          
    if(TRUE == LockPages((LPVOID)bufAddr->vaddrCP, 1, &dwPhys, LOCKFLAG_QUERY_ONLY))
    {        
        //APR 06Mar07 
        //LockPages was observed to return physical addresses aligned at 4KB.
        //This hack was put here to capture the offset from 4KB aligned physical address if any
        dwPhys |= (DWORD)(((DWORD)bufAddr->vaddrCP) & (0xFFF));
    }
    else
    {
        Plat::printf("LockPage failed..\r\n");
    }    
    bufAddr->paddr = (void*)dwPhys;
    bufAddr->vaddr = bufAddr->vaddrCP;
    
}

void FimgPowerOn(BOOL isOn)
{
    S3C6410_SYSCON_REG *pSysConReg;
#if (_WIN32_WCE>=600)
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};    
#endif    

    // Alloc and Map System Controller SFR
#if (_WIN32_WCE>=600)
    ioPhysicalBase.LowPart = S3C6410_BASE_REG_PA_SYSCON;    
    pSysConReg = (S3C6410_SYSCON_REG *)MmMapIoSpace(ioPhysicalBase, sizeof(S3C6410_SYSCON_REG), FALSE);
#else
    pSysConReg = (S3C6410_SYSCON_REG *)DrvLib_MapIoSpace(S3C6410_BASE_REG_PA_SYSCON, sizeof(S3C6410_SYSCON_REG), FALSE);
#endif    
    if (pSysConReg == NULL)
    {
        RETAILMSG(1,(_T("[FIMG:ERR] FimgPowerOn() : pSysConReg DrvLib_MapIoSpace() Failed\n\r")));
        return;
    }
        
    if(isOn)
    {
        pSysConReg->NORMAL_CFG |= (1<<10);        // DOMAIN_G on
        pSysConReg->HCLK_GATE |= (1<<31);       // Clock On
    }
    else
    {
        pSysConReg->NORMAL_CFG &= ~(1<<10);        // DOMAIN_G off
        pSysConReg->HCLK_GATE &= ~(1<<31);       // Clock Off
    }
    
#if (_WIN32_WCE>=600)
    MmUnmapIoSpace((PVOID)pSysConReg, sizeof(S3C6410_SYSCON_REG));    
#else    
    DrvLib_UnmapIoSpace((PVOID)pSysConReg);
#endif    
}

extern "C"  void GetSFRAddress(int* sfrAddr)
{

#if (_WIN32_WCE >= 600)
    
    *sfrAddr = (int)VirtualAllocCopyEx(
                                (HANDLE)GetCurrentProcessId(), 
                                (HANDLE)GetDirectCallerProcessId(), 
                                gFimgBase.vaddr, 
                                FIMG_PHY_SIZE,
                                PAGE_READWRITE | PAGE_NOCACHE);
    
    Plat::printf("sfr phy address = 0x%x\n", gFimgBase.vaddr);                            
    Plat::printf("sfr address = 0x%x\n", *sfrAddr);
#else
    
    *sfrAddr = (int)gFimgBase.paddr;//(int)MapPtrToProcess(gFimgBase.vaddr, (HANDLE) GetCallerProcess());                                
//    Plat::printf("sfr address = 0x%x\n", *sfrAddr);
    
#endif    
}

extern "C"  void  FreeSFRAddress(int sfrAddr)
{
#if (_WIN32_WCE >= 600)
    if(!VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)sfrAddr, 0,  MEM_RELEASE))
    {
        RETAILMSG(1,(TEXT("%s: VirtualFreeEx is failed\r\n"),_T(__FUNCTION__)));
    }
#else
    //UnMapPtr((LPVOID)sfrAddr);        
#endif    
}

extern "C"  void GetDMASFRAddress(int** sfrAddr)
{
#if 0
#if (_WIN32_WCE >= 600)

    
    sfrAddr[0] = (int *)VirtualAllocCopyEx(
                                (HANDLE)GetCurrentProcessId(), 
                                (HANDLE)GetDirectCallerProcessId(), 
                                (LPVOID)g_pPDMAC0Reg, 
                                sizeof(S5PC100_DMAC_REG),
                                PAGE_READWRITE | PAGE_NOCACHE);
                                
    sfrAddr[1] = (int *)VirtualAllocCopyEx(
                                (HANDLE)GetCurrentProcessId(), 
                                (HANDLE)GetDirectCallerProcessId(), 
                                (LPVOID)g_pPDMAC1Reg, 
                                sizeof(S5PC100_DMAC_REG),
                                PAGE_READWRITE | PAGE_NOCACHE);
                                
    sfrAddr[2] = (int *)VirtualAllocCopyEx(
                                (HANDLE)GetCurrentProcessId(), 
                                (HANDLE)GetDirectCallerProcessId(), 
                                (LPVOID)g_pMDMACReg, 
                                sizeof(S5PC100_DMAC_REG),
                                PAGE_READWRITE | PAGE_NOCACHE);
                                
    sfrAddr[3] = (int *)VirtualAllocCopyEx(
                                (HANDLE)GetCurrentProcessId(), 
                                (HANDLE)GetDirectCallerProcessId(), 
                                (LPVOID)g_pSysConReg, 
                                sizeof(S5PC100_SYSCON_CLK_REG),
                                PAGE_READWRITE | PAGE_NOCACHE);                                                                                                
    

#else
#if 0    
    sfrAddr[0] = (int*)S5PC100_BASE_REG_PA_PDMA0;
    sfrAddr[1] = (int*)S5PC100_BASE_REG_PA_PDMA1;
    sfrAddr[2] = (int*)S5PC100_BASE_REG_PA_MDMA;
    sfrAddr[3] = (int*)S5PC100_BASE_REG_PA_SYSCON_CLK;
#endif
#endif    

#endif
}

extern "C"  void  FreeDMASFRAddress(int** sfrAddr)
{
#if 0    
    
#if (_WIN32_WCE >= 600)
    VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)sfrAddr[0], 0,  MEM_RELEASE);
    VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)sfrAddr[1], 0,  MEM_RELEASE);
    VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)sfrAddr[2], 0,  MEM_RELEASE);
    VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)sfrAddr[3], 0,  MEM_RELEASE);
#else
    //UnMapPtr((LPVOID)sfrAddr);        
#endif    

#endif
}
void* GetEmptyMemBlock()
{
    ALLOCMEM_POOL *pTemp = pHeadOfMemoryPool;
        
    EnterCriticalSection(&gles20_chunkalloc_mutex);        
    while(pTemp != NULL)
    {
        if(pTemp->used == FALSE) 
        {
            pTemp->used = TRUE;
            break;
        }
        pTemp = pTemp->next;
    }
    
    LeaveCriticalSection(&gles20_chunkalloc_mutex);        
    return pTemp;    
}

extern "C" void DoAllocPhysMem(DWORD hOpenContext, void* bufAddr, int size)
{
    FIMG_CONTEXT* openHandle = (FIMG_CONTEXT*)hOpenContext;
    ALLOCMEM_ITEM *pHead = openHandle->allocated_list;    
    ALLOCMEM_ITEM *pItem, *pPrev = NULL;
    s3c_3d_mem_alloc*   pOutputBuf = (s3c_3d_mem_alloc*)bufAddr;

    ALLOCMEM_POOL *pTemp = NULL;
    int i;
        
        
    for(i=0;i<10;i++)
    {    
        Plat::printf("[FIMGDRV:0x%08x] DoAllocPhysMem %dth try \n", hOpenContext, (i+1));
        pTemp = (ALLOCMEM_POOL *)GetEmptyMemBlock();        
        if(pTemp != NULL) break;
        Sleep(100);
    }
        
    if(pTemp == NULL)
    {
        Plat::printf("[FIMGDRV:0x%08x] Buffer Allocator unable to allocate %d bytes\n", hOpenContext, size);
        pTemp = pHeadOfMemoryPool;
        while(pTemp != NULL)
        {
            Plat::printf("[FIMGDRV:0x%08x] status 0x%x 0x%x 0x%x by 0x%08x\n", hOpenContext, pTemp->virAddr, pTemp->phyAddr, pTemp->used, pTemp->openHandle);
            pTemp = pTemp->next;
        }        
    }
    else
    {
        
        pOutputBuf->size = pTemp->size;    
        pOutputBuf->phy_addr = (unsigned int)pTemp->phyAddr;
       
#if (_WIN32_WCE >= 600)      
        void*   pCPAddr;
        pCPAddr = VirtualAllocCopyEx((HANDLE)GetCurrentProcessId(),
                            (HANDLE)GetDirectCallerProcessId(),
                            (LPVOID)pTemp->virAddr,
                            pTemp->size,
                            PAGE_READWRITE | PAGE_NOCACHE );   
        pOutputBuf->vir_addr = (unsigned int)pCPAddr;                                              
#else
        pOutputBuf->vir_addr = (unsigned int)pTemp->virAddr;
#endif

        for(pItem=pHead;pItem != NULL; pItem=pItem->next) 
        {
            pPrev = pItem;
        }
        
            
        pItem = (ALLOCMEM_ITEM*)malloc(sizeof(ALLOCMEM_ITEM));
        pItem->phyAddr = pTemp->phyAddr;
        pItem->virAddr = pTemp->virAddr;
        pItem->virAddrCP = (void*)pOutputBuf->vir_addr;
        pItem->memPool = (void*)pTemp;
        pTemp->used = TRUE;
        pTemp->openHandle = (void*)hOpenContext;
        pItem->next = 0;
        
        if(pPrev != NULL) pPrev->next = pItem;
        else openHandle->allocated_list = pItem;
        
        Plat::printf("[FIMGDRV:0x%08x] DoAllocPhysMem 0x%x 0x%x 0x%x %d\n", hOpenContext, pOutputBuf->phy_addr, pOutputBuf->vir_addr, pTemp->virAddr, pOutputBuf->size);
        Plat::printf("[FIMGDRV:0x%08x] After Alloc, Block list for this context\n", hOpenContext);
        Plat::printf("[FIMGDRV:0x%08x] ========================================\n", hOpenContext);
        for(pItem=openHandle->allocated_list;pItem != NULL; pItem=pItem->next) 
        {
            Plat::printf("[FIMGDRV:0x%08x] 0x%x 0x%x 0x%x is Allocated\n", hOpenContext, pItem->phyAddr, pItem->virAddrCP, pItem->virAddr);
        }    

        Plat::printf("[FIMGDRV:0x%08x] ========================================\n", hOpenContext);        
        
    }        
    
}

extern "C" void DoFreePhysMem(DWORD hOpenContext, void* bufAddr)
{
    FIMG_CONTEXT* openHandle = (FIMG_CONTEXT*)hOpenContext;
    ALLOCMEM_ITEM* pHead = openHandle->allocated_list;        
    ALLOCMEM_ITEM *pItem, *pPrev = NULL;
    ALLOCMEM_POOL *pTemp;
    s3c_3d_mem_alloc*   pOutputBuf = (s3c_3d_mem_alloc*)bufAddr;    
    
    Plat::printf("[FIMGDRV:0x%08x] DoFreePhysMem 0x%x 0x%x %d\n", hOpenContext, pOutputBuf->phy_addr, pOutputBuf->vir_addr, pOutputBuf->size);
    Plat::printf("[FIMGDRV:0x%08x] Before Free, Block list for this context\n", hOpenContext);
    Plat::printf("[FIMGDRV:0x%08x] ========================================\n", hOpenContext);
    for(pItem=pHead;pItem != NULL; pItem=pItem->next) 
    {
        Plat::printf("[FIMGDRV:0x%08x] 0x%x 0x%x 0x%x is Allocated\n", hOpenContext, pItem->phyAddr, pItem->virAddrCP, pItem->virAddr);
    }    
    
    Plat::printf("[FIMGDRV:0x%08x] ========================================\n", hOpenContext);
    
    for(pItem=pHead,pPrev=NULL;pItem != NULL; pItem=pItem->next) 
    {
        if((unsigned int)pItem->phyAddr == pOutputBuf->phy_addr) break;
        pPrev = pItem;
    }
    
    if(pItem != NULL)    
    {
        if(pPrev != NULL) pPrev->next = pItem->next;    
        else if(pItem->next != NULL) openHandle->allocated_list = pItem->next;
        else openHandle->allocated_list = NULL;
        
    #if (_WIN32_WCE >= 600)   
        if(pItem->virAddrCP != NULL)
            VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)pItem->virAddrCP, 0,  MEM_RELEASE);
    #endif        
    
        pTemp = (ALLOCMEM_POOL *)pItem->memPool;
        pTemp->used = FALSE;
        pTemp->openHandle = 0;
        
        
        free(pItem);
    }
        
    
    pItem = 0;
            
}


extern "C" void RequestDepthBuffer(DWORD hOpenContext, void* bufAddr)
{
    FIMG_CONTEXT* openHandle = (FIMG_CONTEXT*)hOpenContext;
    s3c_3d_mem_alloc*   pOutputBuf = (s3c_3d_mem_alloc*)bufAddr;

    if(pDepthBuffer != NULL)
    {            
        pOutputBuf->size = pDepthBuffer->size;    
        pOutputBuf->phy_addr = (unsigned int)pDepthBuffer->phyAddr;

       
#if (_WIN32_WCE >= 600)      
        void*   pCPAddr;
        pCPAddr = VirtualAllocCopyEx((HANDLE)GetCurrentProcessId(),
                            (HANDLE)GetDirectCallerProcessId(),
                            (LPVOID)pDepthBuffer->virAddr,
                            pDepthBuffer->size,
                            PAGE_READWRITE | PAGE_NOCACHE );   
        pOutputBuf->vir_addr = (unsigned int)pCPAddr;                                              
#else
        pOutputBuf->vir_addr = (unsigned int)pDepthBuffer->virAddr;
#endif

        Plat::printf("[FIMGDRV:0x%08x] GetDepthBuffer 0x%x 0x%x 0x%x %d\n", hOpenContext, pOutputBuf->phy_addr, pOutputBuf->vir_addr, pDepthBuffer->virAddr, pOutputBuf->size);
    }
}

extern "C" void ReleaseDepthBuffer(DWORD hOpenContext, void* bufAddr)
{
    FIMG_CONTEXT* openHandle = (FIMG_CONTEXT*)hOpenContext;
    s3c_3d_mem_alloc*   pOutputBuf = (s3c_3d_mem_alloc*)bufAddr;      
    
#if (_WIN32_WCE >= 600)   
    if(pOutputBuf->vir_addr != NULL)
        VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)pOutputBuf->vir_addr, 0,  MEM_RELEASE);
#endif            
}

extern "C" void GetMemoryStatus(DWORD hOpenContext, int *outUsedMemory, int *outTotalMemory)
{
    ALLOCMEM_POOL *pTemp = pHeadOfMemoryPool;
    int usedMemory = 0, totalMemory = 0;            
    while(pTemp != NULL)
    {
        if(pTemp->used == TRUE) 
        {
            usedMemory += pTemp->size;
        }
        totalMemory += pTemp->size;
        
        pTemp = pTemp->next;
    }
     
    *outUsedMemory = usedMemory;
    *outTotalMemory = totalMemory;
}


extern "C"  void GetDMACODEAddress(UINT32 phyaddr, int size, int cacheEnable, BufferAddress* bufAddr)
{
#if 0
    PHYSICAL_ADDRESS    ioPhysicalBase = {0,0};
    ioPhysicalBase.LowPart = phyaddr;
    bufAddr->paddr = (PVOID)phyaddr;
    bufAddr->vaddr = (PVOID)MmMapIoSpace(ioPhysicalBase, size, cacheEnable);
    bufAddr->vaddrCP = (PVOID)VirtualAllocCopyEx((HANDLE)GetCurrentProcessId(),
                        (HANDLE)GetDirectCallerProcessId(),
                        bufAddr->vaddr,
                        size,
                        PAGE_READWRITE | PAGE_NOCACHE);
                        
#endif                       
}

extern "C"  void  FreeDMACODEAddress(BufferAddress* bufAddr, int size)
{
#if 0
    if(bufAddr->vaddrCP != NULL)
    {
        if(!VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)bufAddr->vaddrCP, 0,  MEM_RELEASE))
        {
            RETAILMSG(1,(TEXT("%s: VirtualFreeEx is failed\r\n"),_T(__FUNCTION__)));
        }
    }
    if(bufAddr->vaddr != NULL)
    {
        MmUnmapIoSpace(bufAddr->vaddr, size);
    }
#endif
}

extern "C" void GarbageCollect(DWORD hOpenContext)
{
    FIMG_CONTEXT* openHandle = (FIMG_CONTEXT*)hOpenContext;
    ALLOCMEM_ITEM* pHead = openHandle->allocated_list;        
    ALLOCMEM_ITEM *pItem, *pNext;
    ALLOCMEM_POOL *pTemp;
    
    Plat::printf("[FIMGDRV:0x%08x] Garbage Collect\n", hOpenContext);
        
    for(pItem=pHead;pItem != NULL; ) 
    {
        pNext = pItem->next;
        pTemp = (ALLOCMEM_POOL *)pItem->memPool;
        pTemp->used = FALSE;
        pTemp->openHandle = 0;
#if (_WIN32_WCE >= 600)      
        if(pItem->virAddrCP != NULL)
            VirtualFreeEx((HANDLE)GetDirectCallerProcessId(), (LPVOID)pItem->virAddrCP, 0,  MEM_RELEASE);
#endif

        Plat::printf("[FIMGDRV:0x%08x] Garbage Collect 0x%x 0x%x\n", hOpenContext, pTemp->phyAddr, pTemp->virAddr);

        free(pItem);
        pItem = 0;            
        
        pItem = pNext;
    }
}



BOOL AllocateMemoryList()
{
    void *virAddr, *phyAddr, *pSharedAddr;
    int i;
    ALLOCMEM_POOL *pTemp, *pPrev;
    HANDLE hMapping = NULL;
    BOOL retVal = TRUE; 
                
    MEMORYSTATUS mem_status;

    
    Plat::printf("[FIMGDRV] Alloc Block Num = %d  Alloc Block Size = 0x%x\n", g_blkNum, g_blkSize);
    
    
    GlobalMemoryStatus(&mem_status);
    Plat::printf("[FIMGDRV] alloc physmem = %d / %d\n", mem_status.dwAvailPhys, mem_status.dwTotalPhys );
    Plat::printf("[FIMGDRV] alloc virtmem = %d/%d\n", mem_status.dwAvailVirtual, mem_status.dwTotalVirtual );
  
    if(g_depthBufferSize > 0)
    {
        Plat::printf("[FIMGDRV] Only for Depthbuffer is allocated.\n");
        
        virAddr = AllocPhysMem(g_depthBufferSize, PAGE_READWRITE|PAGE_NOCACHE, 0, 0, (PULONG)&phyAddr);
        if(virAddr == NULL)
        {
            Plat::printf("[FIMGDRV] depth buffer allocated is fail\n");
            retVal = FALSE;
        }    
        else
        {
        
#if (_WIN32_WCE >= 600)   
            pSharedAddr = virAddr;
            {
#else
            hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, g_depthBufferSize, NULL);      
            pSharedAddr = (void *)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);     
        
            if(!VirtualCopy (pSharedAddr, (LPVOID)((UINT32)phyAddr>>8), g_depthBufferSize, PAGE_NOCACHE|PAGE_READWRITE|PAGE_PHYSICAL))
            {
                Plat::printf("[FIMGDRV] depthbuffer alloccopy is fail\n");
            }
            else
            {
                VirtualFree (virAddr, 0, MEM_RELEASE);    // this will release the VM    
#endif        
                pDepthBuffer = (ALLOCMEM_POOL*) malloc(sizeof(ALLOCMEM_POOL));
                pDepthBuffer->phyAddr = phyAddr;
                pDepthBuffer->virAddr = pSharedAddr;
                pDepthBuffer->handle = hMapping;
                pDepthBuffer->used = FALSE;
                pDepthBuffer->next = NULL;
                pDepthBuffer->openHandle = 0;
                pDepthBuffer->size = g_depthBufferSize;
                Plat::printf("[FIMGDRV] depthbuffer is allocated on 0x%x 0x%x, size is %dBytes\n", pSharedAddr, phyAddr, g_depthBufferSize);
            }
        }   
    }
  
#if 0
  ///////////////////// Allocated from Camera.
  phyAddr = (void*)CAM_POOL_BASE;
  virAddr = (void*)VirtualAlloc(NULL, CAM_POOL_SIZE, MEM_RESERVE, PAGE_NOACCESS);
    VirtualCopy (virAddr, (LPVOID)((UINT32)phyAddr>>8), CAM_POOL_SIZE, PAGE_NOCACHE|PAGE_READWRITE|PAGE_PHYSICAL);

  while(((DWORD)phyAddr+ALLOCATESIZE) <= (CAM_POOL_BASE+CAM_POOL_SIZE))
  {
        pTemp = (ALLOCMEM_POOL*) malloc(sizeof(ALLOCMEM_POOL));
        pTemp->phyAddr = phyAddr;
        pTemp->virAddr = virAddr;
        pTemp->handle = NULL;
        pTemp->used = FALSE;
        pTemp->next = NULL;
        pTemp->openHandle = 0;
        pTemp->size = ALLOCATESIZE;
        
        if((DWORD)phyAddr == CAM_POOL_BASE)
        {
            pHeadOfMemoryPool = pTemp;
        }
        else
        {
            pPrev->next = pTemp;
        }
        pPrev = pTemp;
        
      phyAddr = (void*)((DWORD)phyAddr + ALLOCATESIZE);
      virAddr = (void*)((DWORD)virAddr + ALLOCATESIZE);        
  }
#endif  
  
  //////////////////// Allocated From AllocPhysMem.
    for(i=0;i<g_blkNum;i++)
    {
        virAddr = AllocPhysMem(g_blkSize, PAGE_READWRITE|PAGE_NOCACHE, 0, 0, (PULONG)&phyAddr);
        if(virAddr == NULL)
        {
            Plat::printf("[FIMGDRV] %d th allocated is fail\n", (i+1));
            retVal = FALSE;
            break;
        }    
        
#if (_WIN32_WCE >= 600)   
        pSharedAddr = virAddr;
#else
        hMapping = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, g_blkSize, NULL);      
        
        pSharedAddr = (void *)MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, 0);     
        
        if(!VirtualCopy (pSharedAddr, (LPVOID)((UINT32)phyAddr>>8), g_blkSize, PAGE_NOCACHE|PAGE_READWRITE|PAGE_PHYSICAL))
        {
            Plat::printf("[FIMGDRV] %d th alloccopy is fail\n", (i+1));
            break;
        }
        
        VirtualFree (virAddr, 0, MEM_RELEASE);    // this will release the VM    
#endif        
        pTemp = (ALLOCMEM_POOL*) malloc(sizeof(ALLOCMEM_POOL));
        pTemp->phyAddr = phyAddr;
        pTemp->virAddr = pSharedAddr;
        pTemp->handle = hMapping;
        pTemp->used = FALSE;
        pTemp->next = NULL;
        pTemp->openHandle = 0;
        pTemp->size = g_blkSize;
        
        if(i == 0)
        {
            pHeadOfMemoryPool = pTemp;
        }
        else
        {
            pPrev->next = pTemp;
        }
        
        pPrev = pTemp;
    }
    
    GlobalMemoryStatus(&mem_status);
    Plat::printf("[FIMGDRV] alloc physmem = %d / %d\n", mem_status.dwAvailPhys, mem_status.dwTotalPhys );
    Plat::printf("[FIMGDRV] alloc virtmem = %d/%d\n", mem_status.dwAvailVirtual, mem_status.dwTotalVirtual);    
    
    
    pTemp = pHeadOfMemoryPool;
    while(pTemp != NULL)
    {
        Plat::printf("[FIMGDRV] Allocated Pool 0x%x 0x%x 0x%x\n", pTemp->virAddr, pTemp->phyAddr, pTemp->used);
        pTemp = pTemp->next;
    }
    
    
    return retVal;
 
}

extern "C" void FreeMemoryList(DWORD hOpenContext)
{
    HANDLE hMapping;
    ALLOCMEM_POOL *pTemp, *pNext;
    
    MEMORYSTATUS mem_status;
    
    Plat::printf("[FIMGDRV:0x%08x] Release Free Memroy List\n", hOpenContext);
  
    GlobalMemoryStatus(&mem_status);
    Plat::printf("[FIMGDRV] Before Release physmem = %d / %d\n", mem_status.dwAvailPhys, mem_status.dwTotalPhys );
    Plat::printf("[FIMGDRV] Before Release virtmem = %d/%d\n", mem_status.dwAvailVirtual, mem_status.dwTotalVirtual );
      
  
    
    if(pDepthBuffer != NULL)
    {
#if (_WIN32_WCE < 600)    
        hMapping = pDepthBuffer->handle;
    
        if (pDepthBuffer->virAddr != NULL)
        {
            UnmapViewOfFile((LPVOID)pDepthBuffer->virAddr);
        }

        if (hMapping != NULL)
        {
             CloseHandle(hMapping);
        }   
     
        free(pDepthBuffer);         
#endif        
        pDepthBuffer = NULL;
    }
    
    if(pHeadOfMemoryPool != NULL)
    {
#if (_WIN32_WCE < 600)  
        pTemp = pHeadOfMemoryPool;
        while(pTemp != NULL)
        {
            pNext = pTemp->next;
          
            hMapping = pTemp->handle;
    
            if (pTemp->virAddr != NULL)
            {
                UnmapViewOfFile((LPVOID)pTemp->virAddr);
            }

            if (hMapping != NULL)
            {
                 CloseHandle(hMapping);
            }   
     
            free(pTemp);
            pTemp = pNext;
        }         
#endif         
        pHeadOfMemoryPool = NULL;   
    }
    
    GlobalMemoryStatus(&mem_status);
    Plat::printf("[FIMGDRV] After Release physmem = %d / %d\n", mem_status.dwAvailPhys, mem_status.dwTotalPhys );
    Plat::printf("[FIMGDRV] After Release virtmem = %d/%d\n", mem_status.dwAvailVirtual, mem_status.dwTotalVirtual);    
}

extern "C" BOOL ReAllocMemoryList(DWORD hOpenContext)
{
    Plat::printf("[FIMGDRV:0x%08x] Re Allocate Memroy List\n", hOpenContext);
    
    if(pHeadOfMemoryPool != NULL || pDepthBuffer != NULL)
    {
        Plat::printf("[FIMGDRV:0x%08x] ReAllocateList Error! Already allocated\n", hOpenContext);
        return FALSE;
    }
    
    if(!ReadMemoryPoolFromRegistry(&g_blkNum, &g_blkSize, &g_depthBufferSize))
    {
        g_blkSize = ALLOCATESIZE;
        g_blkNum = ALLOCATENUM;
    }
    
    if(!AllocateMemoryList())
    {
        Plat::printf("[FIMGDRV:0x%08x] ReAllocateList Error!\n", hOpenContext);
        FreeMemoryList(hOpenContext);
        return FALSE;
    }
    return TRUE;
}

BOOL ReadMemoryPoolFromRegistry(int *pBlkNum, int *pBlkSize, int *pDepthBufferSize)
{
    HKEY  hKey = 0;
    DWORD dwType  = 0;
    DWORD dwSize  = sizeof ( DWORD );
    DWORD dwValue = -1;

    if( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE, L"Drivers\\BuiltIn\\FIMG", 0, 0, &hKey ))
    {
        //mio
        return FALSE;
    }

    *pDepthBufferSize = 0;
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"DepthBufSize", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(   ( REG_DWORD == dwType )
           && ( sizeof( DWORD ) == dwSize ))
        {
            *pDepthBufferSize = dwValue;
        }
    }

    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"AllocblkNum", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(   ( REG_DWORD == dwType )
           && ( sizeof( DWORD ) == dwSize ))
        {
            *pBlkNum = dwValue;
        }
        else return FALSE;
    }
    else return FALSE;
  
    
    if( ERROR_SUCCESS == RegQueryValueEx( hKey, L"AllocblkSize", 0, &dwType, (BYTE *)&dwValue, &dwSize ) )
    {
        if(   ( REG_DWORD == dwType )
           && ( sizeof( DWORD ) == dwSize ))
        {
            *pBlkSize = dwValue;
        }
        else return FALSE;
    }
    else return FALSE;
    
    return TRUE;
}
