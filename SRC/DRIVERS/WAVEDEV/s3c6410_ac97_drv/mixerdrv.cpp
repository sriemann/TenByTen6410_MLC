//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
// Use of this sample source code is subject to the terms of the 
// Software License Agreement (SLA) under which you licensed this software product.
// If you did not accept the terms of the license agreement, 
// you are not authorized to use this sample source code. 
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//
// -----------------------------------------------------------------------------
//
//      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//      ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//      THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//      PARTICULAR PURPOSE.
//  
// -----------------------------------------------------------------------------
//
//  @doc    WDEV_EXT
//
//  @module mixerdrv.cpp | Implements the WODM_XXX and WIDM_XXX messages that are 
//          passed to the wave audio driver via the <f WAV_IOControl> function.
//          This module contains code that is common or very similar between
//          input and output functions.
//
//  @xref   <t Wave Input Driver Messages> (WIDM_XXX) <nl>
//          <t Wave Output Driver Messages> (WODM_XXX)
//
// -----------------------------------------------------------------------------

#include <wavemain.h>
#include <mmreg.h>


#define LOGICAL_VOLUME_MAX  0xFFFF
#define LOGICAL_VOLUME_MIN  0
#define LOGICAL_VOLUME_STEPS 16

// only support Mic, LineIn as inputs
#define INPUT_SELECT_COUNT 2

#define DEVICE_NAME     L"Audio Mixer"
#define DRIVER_VERSION  0x100

#define NELEMS(x) (sizeof(x)/sizeof((x)[0]))

// mixer handle data types
typedef DWORD (* PFNDRIVERCALL)(DWORD hmx, UINT uMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
typedef struct _tagMCB MIXER_CALLBACK, * PMIXER_CALLBACK;

struct _tagMCB
{
    DWORD           hmx;
    PFNDRIVERCALL   pfnCallback;
    PMIXER_CALLBACK pNext;
};


// mixer line ID are 16-bit values formed by concatenating the source and destination line indices
// 
#define MXLINEID(dst,src)       ((USHORT) ((USHORT)(dst) | (((USHORT) (src)) << 8)))
#define GET_MXLINE_SRC(lineid)  ((lineid) >> 8)
#define GET_MXLINE_DST(lineid)  ((lineid) & 0xff)
#define GET_MXCONTROL_ID(ctl)   ((ctl)-g_controls)

#ifdef DEBUG

// DEBUG-only support for displaying control type codes in readable form


typedef struct 
{
    DWORD   val;
    PWSTR   str;
} MMSYSCODE;


#define MXCTYPE(typ) {typ, TEXT(#typ)}


MMSYSCODE ctype_table[] =
{
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_CUSTOM),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BASS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_EQUALIZER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_FADER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_TREBLE),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_VOLUME),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MIXER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MUX),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SINGLESELECT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BOOLEANMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PEAKMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SIGNEDMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_DECIBELS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PERCENT),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SIGNED),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_UNSIGNED),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_PAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_QSOUNDPAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_SLIDER),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BOOLEAN),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_BUTTON),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_LOUDNESS),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MONO),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MUTE),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_ONOFF),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_STEREOENH),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MICROTIME),
    MXCTYPE(MIXERCONTROL_CONTROLTYPE_MILLITIME),
};


PWSTR
COMPTYPE(DWORD dwValue)
{ int i;

    for (i = 0; i < NELEMS(ctype_table); i++) {
        if (ctype_table[i].val == dwValue) {
            return ctype_table[i].str;
        }
    }
    return TEXT("<unknown>");

}


#else
#define COMPTYPE(n) TEXT("<>")
#endif


// Destinations: 
//    LINE_OUT - line out jack
//      PCM_IN - input to ADC
// Sources
//  MIC - MIC jack



enum {
    LINE_OUT = 0x80,
    PCM_IN,
    MIC,
    NOLINE   = 0xff // doesn't HAVE to be FF, but makes it easier to see
};


const USHORT
g_dst_lines[] = 
    {
    MXLINEID(LINE_OUT,NOLINE),
    MXLINEID(PCM_IN,NOLINE)
};


const USHORT
g_PCM_IN_sources[] = 
{
    MXLINEID(PCM_IN,MIC)
};


// MXLINEDESC corresponds to MIXERLINE, but is designed to conserve space

typedef struct tagMXLINEDESC  const * PMXLINEDESC, MXLINEDESC;
typedef struct tagMXCONTROLDESC  const * PMXCONTROLDESC, MXCONTROLDESC;


struct tagMXLINEDESC { 
    DWORD dwComponentType; 
    PCWSTR szShortName; 
    PCWSTR szName; 
    DWORD ucFlags; 
    USHORT const * pSources;
    USHORT usLineID;
    UINT8 ucChannels; 
    UINT8 ucConnections; 
    UINT8 ucControls; 
    DWORD    dwTargetType;
    UINT8   ucDstIndex;
    UINT8   ucSrcIndex;
} ; 


// MXCONTROLDESC is driver shorthand for Volume and Mute MIXERCONTROL
struct tagMXCONTROLDESC
{
    PWSTR   szName;
    DWORD   dwType;
    USHORT  usLineID;           // line that owns this control
};


// mixerline ID
#define _MXLE(id,dst,src,flags,comptype,nch,ncnx,nctrl,sname,lname,target,sarray)\
    {comptype,\
    TEXT(sname),TEXT(lname),\
    flags, \
    sarray,\
    id,\
    nch,ncnx,nctrl,\
    target,\
    dst,src}


// declare a destination line
#define MXLED(id,dst,flags,comptype,nch,nctrl,sname,lname,target,srcarray,nsrcs)\
    _MXLE(MXLINEID(id,NOLINE),dst,NOLINE,flags,comptype,nch,nsrcs,nctrl,sname,lname,target,srcarray)\


// declare a source line
#define MXSLE(id,dst,src,flags,comptype,nch,nctrl,sname,lname,target)\
    _MXLE(id,dst,src,flags,comptype,nch,0,nctrl,sname,lname,target,NULL)\


MXLINEDESC 
g_mixerline[] = 
{
    // dst line 0 - speaker out
    MXLED(LINE_OUT, 0,
        MIXERLINE_LINEF_ACTIVE, // no flags
        MIXERLINE_COMPONENTTYPE_DST_SPEAKERS,
        1, // mono
        2, // controls: volume + mute
        "Volume Control","Volume Control",
        MIXERLINE_TARGETTYPE_WAVEOUT,
        NULL, 0
        ),
    // dst line 1 - microphone input
    MXLED(PCM_IN, 1,
        MIXERLINE_LINEF_ACTIVE, // flags
        MIXERLINE_COMPONENTTYPE_DST_WAVEIN,
        1, // mono
        1, // 1 control (MUX) - the record gain is virtualized across all mux inputs
        "Recording Contr","Recording Control",
        MIXERLINE_TARGETTYPE_WAVEIN,
        g_PCM_IN_sources, NELEMS(g_PCM_IN_sources)
        ),

    // ----------------------
    // PCM_IN Sources Lines
    // ----------------------

    // src line - Microphone
    MXSLE(MXLINEID(PCM_IN, MIC), 1, 1,
        MIXERLINE_LINEF_ACTIVE | MIXERLINE_LINEF_SOURCE, // flags
        MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE,
        1, // mono
        2, // controls: volume, mute
        "Microphone","Microphone",
        MIXERLINE_TARGETTYPE_WAVEIN
        ),

};
const int nlines = NELEMS(g_mixerline);
#undef _MXLE
#undef MXLED
#undef MXSLE



#define MXCE(dst,src,nme,type) {TEXT(nme),type,MXLINEID(dst,src)}
MXCONTROLDESC
g_controls[] = 
{
    MXCE(LINE_OUT, NOLINE,  "Master Volume",  MIXERCONTROL_CONTROLTYPE_VOLUME),
    MXCE(LINE_OUT, NOLINE,  "Master Mute",    MIXERCONTROL_CONTROLTYPE_MUTE),
    MXCE(PCM_IN,   MIC,     "Mic Volume",     MIXERCONTROL_CONTROLTYPE_VOLUME),
    MXCE(PCM_IN,   MIC,     "Mic Mute",       MIXERCONTROL_CONTROLTYPE_MUTE),
};
const UINT ncontrols = NELEMS(g_controls);
#undef MXCE


PMXLINEDESC
LookupMxLine(USHORT usLineID)
{
    // scan for mixer line
    int i;
    for (i = 0; i < nlines; i++) {
        if (g_mixerline[i].usLineID == usLineID) {
            return &g_mixerline[i];
        }
    }
    return NULL;
}


int
LookupMxControl (USHORT usLineID, DWORD dwControlType)
{
    UINT i;
    for (i = 0; i < ncontrols; i++) {
        PMXCONTROLDESC pSrcControl = &g_controls[i];
        if (    pSrcControl->usLineID == usLineID 
            &&  pSrcControl->dwType == dwControlType) {
            break;
        }
    }
    return i;
}


void
CopyMixerControl(PMIXERCONTROL pDst, PMXCONTROLDESC pSrc, DWORD dwIndex)
{
    // all of our lines have a volume and a mute.
    // in addition, the PCM_IN has a MUX control
    // fill in the volume:
    pDst->cbStruct = sizeof(MIXERCONTROL);
    pDst->dwControlID = dwIndex;

    wcscpy_s(pDst->szName, _countof(pDst->szName), pSrc->szName);
    wcscpy_s(pDst->szShortName, _countof(pDst->szShortName), pSrc->szName);

    pDst->dwControlType = pSrc->dwType;
    pDst->cMultipleItems = 0;

    switch(pSrc->dwType) {
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
        pDst->fdwControl = 0;
        pDst->Metrics.cSteps =  LOGICAL_VOLUME_STEPS;
        pDst->Bounds.lMaximum = LOGICAL_VOLUME_MAX;
        pDst->Bounds.lMinimum = LOGICAL_VOLUME_MIN;
        break;
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
    case MIXERCONTROL_CONTROLTYPE_MUTE:
        pDst->fdwControl = MIXERCONTROL_CONTROLF_UNIFORM;
        pDst->Metrics.cSteps = 0;
        pDst->Bounds.lMaximum = 1;
        pDst->Bounds.lMinimum = 0;
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("Unexpected control type %08x\r\n"), pSrc->dwType));
        ASSERT(0);
    }
}

static PMIXER_CALLBACK g_pHead; // list of open mixer instances


void
PerformMixerCallbacks(DWORD dwMessage, DWORD dwId)
{
    PMIXER_CALLBACK pCurr;
    for (pCurr = g_pHead; pCurr != NULL; pCurr = pCurr->pNext) {
        if (pCurr->pfnCallback != NULL) {
            DEBUGMSG(ZONE_VERBOSE, (TEXT("MixerCallback(%d)\r\n"), dwId));
            pCurr->pfnCallback(pCurr->hmx, dwMessage, 0, dwId, 0);
        }
    }
}


DWORD 
wdev_MXDM_GETDEVCAPS (PMIXERCAPS pCaps, DWORD dwSize)
{
    pCaps->wMid = MM_MICROSOFT;
    pCaps->wPid = MM_MSFT_WSS_MIXER;
    wcscpy(pCaps->szPname, DEVICE_NAME);
    pCaps->vDriverVersion = DRIVER_VERSION;
    pCaps->cDestinations = NELEMS(g_dst_lines);
    pCaps->fdwSupport = 0;

    return MMSYSERR_NOERROR;
}


DWORD 
wdev_MXDM_OPEN (PDWORD phMixer, PMIXEROPENDESC pMOD, DWORD dwFlags)
{
    PMIXER_CALLBACK pNew;
    pNew = (PMIXER_CALLBACK) LocalAlloc(LMEM_FIXED, sizeof(MIXER_CALLBACK));
    if (pNew == NULL) {
        ERRMSG("wdev_MXDM_OPEN: out of memory");
        return MMSYSERR_NOMEM;
    }

    pNew->hmx = (DWORD) pMOD->hmx;
    if (dwFlags & CALLBACK_FUNCTION) {
        pNew->pfnCallback = (PFNDRIVERCALL) pMOD->dwCallback;
    }
    else {
        pNew->pfnCallback = NULL;
    }
    pNew->pNext = g_pHead;
    g_pHead = pNew;
    *phMixer = (DWORD) pNew;

    return MMSYSERR_NOERROR;
}


DWORD 
wdev_MXDM_CLOSE (DWORD dwHandle)
{
    PMIXER_CALLBACK pCurr, pPrev;
    pPrev = NULL;
    for (pCurr = g_pHead; pCurr != NULL; pCurr = pCurr->pNext) {
        if (pCurr == (PMIXER_CALLBACK) dwHandle) {
            if (pPrev == NULL) {
                // we're deleting the first item
                g_pHead = pCurr->pNext;
            }
            else {
                pPrev->pNext = pCurr->pNext;
            }
            LocalFree(pCurr);
            break;
        }
        pPrev = pCurr;
    }

    return MMSYSERR_NOERROR;
}


DWORD 
wdev_MXDM_GETLINEINFO(PMIXERLINE pQuery, DWORD dwFlags)
{ int i;

    // pQuery is validated by API - points to accessible, properly sized MIXERLINE structure

    // result - assume failure
    PMXLINEDESC pFound = NULL;
    MMRESULT mmRet = MIXERR_INVALLINE;
    USHORT usLineID = NOLINE;

    switch (dwFlags & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_DESTINATION:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo DESTINATION %x\r\n"), pQuery->dwDestination));
        {
            if (pQuery->dwDestination >= NELEMS(g_dst_lines)) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid destination line %d\r\n"), pQuery->dwDestination));
                return MIXERR_INVALLINE;
            }
            usLineID = g_dst_lines[pQuery->dwDestination];
        }
        break;
    case MIXER_GETLINEINFOF_LINEID:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo LINEID %x\r\n"), pQuery->dwLineID));
        usLineID = (USHORT) pQuery->dwLineID;
        break;
    case MIXER_GETLINEINFOF_SOURCE:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo SOURCE %x %x\r\n"), pQuery->dwDestination, pQuery->dwSource));
        {
            PMXLINEDESC pLine;
            // look up the destination line, then index into it's source table
            // to find the indexed source.
            if (pQuery->dwDestination >= NELEMS(g_dst_lines)) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid destination line %d\r\n"), pQuery->dwDestination));
                return MIXERR_INVALLINE;
            }
            pLine = LookupMxLine(g_dst_lines[pQuery->dwDestination]);
            if (pLine == NULL) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: inconsistent internal mixer line table\r\n")));
                return MMSYSERR_ERROR;
            }
            if (pQuery->dwSource >= pLine->ucConnections) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid source line %d\r\n"), pQuery->dwSource));
                return MIXERR_INVALLINE;
            }
            usLineID = pLine->pSources[pQuery->dwSource];
        }
        break;
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo COMPONENT\r\n")));
        break;

    case MIXER_GETLINEINFOF_TARGETTYPE:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo TARGET\r\n")));
        // valid query, but we're not going to form usLineID
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid query %08x\r\n"), dwFlags & MIXER_GETLINEINFOF_QUERYMASK));
        return MMSYSERR_INVALPARAM;
    }

    switch (dwFlags & MIXER_GETLINEINFOF_QUERYMASK) {
    case MIXER_GETLINEINFOF_COMPONENTTYPE:
        // scan for line of proper type
        for (i = 0; i < nlines; i++) {
            if (g_mixerline[i].dwComponentType == pQuery->dwComponentType) {
                pFound = &g_mixerline[i];
                break;
            }
        }
#ifdef DEBUG
        if (pFound == NULL) {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: no line of component type %08x\r\n"), pQuery->dwComponentType));
        }
#endif
        break;
    case MIXER_GETLINEINFOF_TARGETTYPE:
        // scan for target type
        for (i = 0; i < nlines; i++) {
            if (g_mixerline[i].dwTargetType == pQuery->Target.dwType) {
                pFound = &g_mixerline[i];
                break;
            }
        }
#ifdef DEBUG
        if (pFound == NULL) {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: no line of target type %08x\r\n"), pQuery->Target.dwType));
        }
#endif
        break;

    case MIXER_GETLINEINFOF_DESTINATION:
    case MIXER_GETLINEINFOF_LINEID:
    case MIXER_GETLINEINFOF_SOURCE:
        pFound = LookupMxLine(usLineID);
        if (pFound == NULL) {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineInfo: invalid line ID %08x\r\n"), usLineID));
            return MMSYSERR_ERROR;
        }
        break;
    default:
        // should never happen - we filter for this in the first switch()
        break;

    }

    if (pFound != NULL) {
        pQuery->cChannels = pFound->ucChannels;
        pQuery->cConnections = pFound->ucConnections;
        pQuery->cControls = pFound->ucControls;
        pQuery->dwComponentType = pFound->dwComponentType;
        pQuery->dwLineID = pFound->usLineID;
        pQuery->dwDestination = pFound->ucDstIndex;
        pQuery->dwSource = pFound->ucSrcIndex;
        pQuery->fdwLine = pFound->ucFlags;
        pQuery->Target.dwDeviceID = 0;
        pQuery->Target.dwType = pFound->dwTargetType;
        pQuery->Target.vDriverVersion = DRIVER_VERSION;
        pQuery->Target.wMid = MM_MICROSOFT;
        pQuery->Target.wPid = MM_MSFT_WSS_MIXER;
        wcscpy(pQuery->szName, pFound->szName);
        wcscpy(pQuery->szShortName, pFound->szShortName);
        wcscpy(pQuery->Target.szPname, DEVICE_NAME);
        mmRet = MMSYSERR_NOERROR;

        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineInfo: \"%s\" %08x\r\n"), pFound->szName, pFound->ucFlags));
    
    }

    return mmRet;
}


DWORD 
wdev_MXDM_GETLINECONTROLS (PMIXERLINECONTROLS pQuery, DWORD dwFlags)
{
 UINT i;

    PMIXERCONTROL pDstControl = pQuery->pamxctrl;
    USHORT usLineID = (USHORT) pQuery->dwLineID;
    DWORD dwCount = pQuery->cControls;

    switch (dwFlags & MIXER_GETLINECONTROLSF_QUERYMASK) {
    case MIXER_GETLINECONTROLSF_ALL:
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineControls: ALL %x\r\n"), usLineID));
        // retrieve all controls for the line pQuery->dwLineID
        {
            PMXLINEDESC pFound = LookupMxLine(usLineID);
            if (pFound == NULL) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid line ID %04x\r\n"), usLineID));
                return MIXERR_INVALLINE;
            }
            if (pFound->ucControls != dwCount) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: incorrect number of controls. Expect %d, found %d.\r\n"),dwCount,pFound->ucControls));
                return MMSYSERR_INVALPARAM;
            }
            for (i = 0; i < ncontrols && dwCount > 0; i++) {
                PMXCONTROLDESC pSrcControl = &g_controls[i];
                if (pSrcControl->usLineID == usLineID) {
                    CopyMixerControl(pDstControl, pSrcControl, i);
                    pDstControl++;
                    dwCount--;
                }
            }
        }
        break;

    case MIXER_GETLINECONTROLSF_ONEBYID: 
        DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineControls: ONEBYID %x\r\n"), pQuery->dwControlID));
        // retrieve the control specified by pQuery->dwControlID
        if (pQuery->dwControlID >= ncontrols) {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid control ID %d (max %d)\r\n"), pQuery->dwControlID, ncontrols));
            return MIXERR_INVALCONTROL;
        }
        if (dwCount < 1) {
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: control count must be nonzero\r\n")));
            return MMSYSERR_INVALPARAM;
        }
        CopyMixerControl(pDstControl, &g_controls[pQuery->dwControlID], pQuery->dwControlID);
        pQuery->dwLineID = g_controls[pQuery->dwControlID].usLineID;
        break;

    case MIXER_GETLINECONTROLSF_ONEBYTYPE: 
        // retrieve the control specified by pQuery->dwLineID and pQuery->dwControlType
        {
            UINT index;
            PMXLINEDESC pFound = LookupMxLine(usLineID);
            if (pFound == NULL) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid line ID %04x\r\n"), usLineID));
                return MIXERR_INVALLINE;
            }
            DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineControls: ONEBYTYPE %x \"%s\"\r\n"), usLineID, pFound->szName));
            if (dwCount < 1) {
                DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: control count must be non zero\r\n")));
                return MMSYSERR_INVALPARAM;
            }
            index = LookupMxControl(usLineID, pQuery->dwControlType);
            if (index >= ncontrols) {
                // not to be alarmed: SndVol32 queries for LOTS of control types we don't have
                DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerLineControls: line %04x (%s) has no control of type %s (%08x)\r\n"), usLineID, pFound->szName, COMPTYPE(pQuery->dwControlType), pQuery->dwControlType));
                return MMSYSERR_INVALPARAM;
            }
            
            CopyMixerControl(pDstControl, &g_controls[index], index);
            return MMSYSERR_NOERROR;
            // if we fall out of the search loop, we return failure
        }
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerLineControls: invalid query %08x\r\n"), dwFlags & MIXER_GETLINECONTROLSF_QUERYMASK));
        break;

    }
    return MMSYSERR_NOERROR;
}


DWORD 
wdev_MXDM_GETCONTROLDETAILS (PMIXERCONTROLDETAILS pQuery, DWORD dwFlags)
{
    PMXCONTROLDESC pSrcControlDesc;
    PMXLINEDESC pLine;
    ULONG ulControlValue = 0;

    // API guarantees that pQuery points to accessible, aligned, properly sized MIXERCONTROLDETAILS structure
    DEBUGMSG(ZONE_VERBOSE, (TEXT("GetMixerControlDetails(%d)\r\n"), pQuery->dwControlID));

    if (pQuery->dwControlID >= ncontrols) {
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerControlDetails: invalid control %d (max %d)\r\n"), pQuery->dwControlID, ncontrols));
        return MIXERR_INVALCONTROL;
    }

    pSrcControlDesc = &g_controls[pQuery->dwControlID];
    pLine = LookupMxLine(pSrcControlDesc->usLineID);
    if (pLine == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerControlDetails: inconsistent internal mixer line table\r\n")));
        return MMSYSERR_ERROR;
    }
 

    switch (dwFlags & MIXER_GETCONTROLDETAILSF_QUERYMASK) {
    case MIXER_GETCONTROLDETAILSF_VALUE:
        switch(pSrcControlDesc->dwType) {
            case MIXERCONTROL_CONTROLTYPE_VOLUME:
                {
                    switch (GET_MXLINE_DST(pSrcControlDesc->usLineID)) {
                        case LINE_OUT:
                            ulControlValue = g_pHWContext->GetOutputGain();
                            break;
                        case PCM_IN:
                            ulControlValue = g_pHWContext->GetInputGain();
                            break;
                        default:
                            DEBUGCHK(0);
                            break;
                    }


                    MIXERCONTROLDETAILS_UNSIGNED * pValue = (MIXERCONTROLDETAILS_UNSIGNED * ) pQuery->paDetails;
                    ULONG ulVolR, ulVolL;
                    ulVolR = ulControlValue & 0xffff;
                    if (pLine->ucChannels == 2) {
                        ulVolL = (ulControlValue >> 16) & 0xffff;
                    }
                    else {
                        ulVolL = ulVolR;
                    }

                    if (pQuery->cChannels == 1) {
                        pValue[0].dwValue = (ulVolR + ulVolL)/2;
                    }
                    else {
                        pValue[0].dwValue = ulVolL;
                        pValue[1].dwValue = ulVolR;
                    }
                }
                break;
            case MIXERCONTROL_CONTROLTYPE_ONOFF:
            case MIXERCONTROL_CONTROLTYPE_MUTE:
                {
                    switch (GET_MXLINE_DST(pSrcControlDesc->usLineID)) {
                        case LINE_OUT:
                            ulControlValue = g_pHWContext->GetOutputMute();
                            break;
                        case PCM_IN:
                            ulControlValue = g_pHWContext->GetInputMute();
                            break;
                        default:
                            DEBUGCHK(0);
                            break;
                    }

                    MIXERCONTROLDETAILS_BOOLEAN * pValue = (MIXERCONTROLDETAILS_BOOLEAN *) pQuery->paDetails;
                    pValue[0].fValue = ulControlValue;
                }
                break;
            default:
                DEBUGCHK(0);
                break;
        }
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerControlDetails: invalid query %08x\r\n"), dwFlags & MIXER_GETCONTROLDETAILSF_QUERYMASK));
        break;
    }
    return MMSYSERR_NOERROR;
}


DWORD 
wdev_MXDM_SETCONTROLDETAILS (PMIXERCONTROLDETAILS pDetail, DWORD dwFlags)
{
    PMXCONTROLDESC pSrcControlDesc;
    PMXLINEDESC pLine;
    MIXERCONTROLDETAILS_UNSIGNED * pValue = (MIXERCONTROLDETAILS_UNSIGNED * ) pDetail->paDetails;
    DWORD dwSetting;

    // API guarantees that pDetail points to accessible, aligned, properly siezd MIXERCONTROLDETAILS structure
    DEBUGMSG(ZONE_VERBOSE, (TEXT("SetMixerControlDetails(%d)\r\n"), pDetail->dwControlID));

    if (pDetail->dwControlID >= ncontrols) {
        DEBUGMSG(ZONE_ERROR, (TEXT("SetMixerControlDetails: invalid control %d (max %d)\r\n"), pDetail->dwControlID, ncontrols));
        return MIXERR_INVALCONTROL;
    }

    pSrcControlDesc = &g_controls[pDetail->dwControlID];
    pLine = LookupMxLine(pSrcControlDesc->usLineID);
    if (pLine == NULL) {
        DEBUGMSG(ZONE_ERROR, (TEXT("SetMixerControlDetails: inconsistent internal mixer line table\r\n")));
        return MMSYSERR_ERROR;
    }

    switch(pSrcControlDesc->dwType) {
    case MIXERCONTROL_CONTROLTYPE_VOLUME:
        {
            DWORD dwSettingL, dwSettingR;
            dwSettingL = pValue[0].dwValue & 0xffff;
            // setting might be mono or stereo. For mono, apply same volume to both channels
            if (pDetail->cChannels == 2) {
                dwSettingR = pValue[1].dwValue & 0xffff;
            }
            else {
                dwSettingR = dwSettingL; 
            }

            if (pLine->ucChannels == 1) {
                dwSetting = (dwSettingL + dwSettingR) / 2;
            }
            else {
                dwSetting = (dwSettingL << 16) | dwSettingR;
            }
            // now apply the setting to the hardware
            switch (GET_MXLINE_DST(pSrcControlDesc->usLineID)) {
            case LINE_OUT:
                g_pHWContext->SetOutputGain(dwSetting);
                break;
            case PCM_IN:
                g_pHWContext->SetInputGain(dwSetting);
                break;
            default:
                DEBUGCHK(0);
                break;

            }
            DEBUGMSG(ZONE_VERBOSE, (TEXT("wdev_MXDM_SETCONTROLDETAILS: %08x %08x %08x\r\n"), dwSettingL, dwSettingR, dwSetting));
        }
        break;
    case MIXERCONTROL_CONTROLTYPE_ONOFF:
    case MIXERCONTROL_CONTROLTYPE_MUTE:
        {
            // now apply the setting to the hardware
            switch (GET_MXLINE_DST(pSrcControlDesc->usLineID)) {
            case LINE_OUT:
                g_pHWContext->SetOutputMute(pValue[0].dwValue);
                break;
            case PCM_IN:
                g_pHWContext->SetInputMute(pValue[0].dwValue);
                break;
            default:
                DEBUGCHK(0);
                break;

            }
        }
        break;
    default:
        DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerControlDetails: unexpected control type %08x\r\n"), pSrcControlDesc->dwType));
        ASSERT(0);
        return MMSYSERR_ERROR;
    }


    PerformMixerCallbacks (MM_MIXM_CONTROL_CHANGE, GET_MXCONTROL_ID(pSrcControlDesc));

    return MMSYSERR_NOERROR;
}


BOOL HandleMixerMessage(PMMDRV_MESSAGE_PARAMS pParams, DWORD *pdwResult)
{
    MMRESULT dwRet;

    switch (pParams->uMsg) {
    case MXDM_GETNUMDEVS:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETNUMDEVS\r\n")));
        dwRet = 1;
        break;

    case MXDM_GETDEVCAPS:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETDEVCAPS\r\n")));
        dwRet = wdev_MXDM_GETDEVCAPS((PMIXERCAPS) pParams->dwParam1, pParams->dwParam2);
        break;

    case MXDM_OPEN:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_OPEN\r\n")));
        dwRet = wdev_MXDM_OPEN((PDWORD) pParams->dwUser, (PMIXEROPENDESC) pParams->dwParam1, pParams->dwParam2);
        break;

    case MXDM_CLOSE:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_CLOSE\r\n")));
        dwRet = wdev_MXDM_CLOSE(pParams->dwUser);
        break;

    case MXDM_GETLINEINFO:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETLINEINFO\r\n")));
        dwRet = wdev_MXDM_GETLINEINFO((PMIXERLINE) pParams->dwParam1, pParams->dwParam2); 
        break;

    case MXDM_GETLINECONTROLS:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETLINECONTROLS\r\n")));
        dwRet = wdev_MXDM_GETLINECONTROLS((PMIXERLINECONTROLS) pParams->dwParam1, pParams->dwParam2);
        break;

    case MXDM_GETCONTROLDETAILS:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_GETCONTROLDETAILS\r\n")));
        dwRet = wdev_MXDM_GETCONTROLDETAILS((PMIXERCONTROLDETAILS) pParams->dwParam1, pParams->dwParam2);
        break;

    case MXDM_SETCONTROLDETAILS:
        PRINTMSG(ZONE_WODM, (TEXT("MXDM_SETCONTROLDETAILS\r\n")));
        dwRet = wdev_MXDM_SETCONTROLDETAILS((PMIXERCONTROLDETAILS) pParams->dwParam1, pParams->dwParam2);
        break;

    default:
        ERRMSG("Unsupported mixer message");
        dwRet = MMSYSERR_NOTSUPPORTED;
        break;
    }      // switch (pParams->uMsg)


    *pdwResult = dwRet;
    return TRUE;
}


struct _global_volume
{
    ULONG   dwMasterVolume;
    BOOL    fMasterMute;
    ULONG   dwMicVolume;
    BOOL    fMicMute;
} g_VolumeSettings;


MMRESULT GetMixerValue(DWORD dwControl, PDWORD pdwSetting)
{

    switch (dwControl) {
        case WPDMX_MASTER_VOL:
            *pdwSetting = g_VolumeSettings.dwMasterVolume;
            break;
        case WPDMX_MASTER_MUTE:
            *pdwSetting = g_VolumeSettings.fMasterMute;
            break;
        case WPDMX_MIC_VOL:
            *pdwSetting = g_VolumeSettings.dwMicVolume;
            break;
        case WPDMX_MIC_MUTE:
            *pdwSetting = g_VolumeSettings.fMicMute;
            break;
        default:
            DEBUGMSG(ZONE_ERROR, (TEXT("GetMixerValue: unrecognized control")));
            return MMSYSERR_NOTSUPPORTED;
    }

    return MMSYSERR_NOERROR;
}


