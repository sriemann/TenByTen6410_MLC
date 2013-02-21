/***********************************************************
    ehm.h
    Error handling macros

Author(s): KKennedy (derived from many sources)

    {x,}{CVD}{H,P,B,W}R{A,}{Ex,}
    C=check, V=verify, D=dump
    H=hresult, P=ptr, B=bool, W=windows
    R=result
    A=assert
    Ex=extended version
    x=wart, suppresses assert due to practical pblm (obsolete!)

    C checks, and bails on failure after setting hr
        A adds an ASSERT before the bail
    V checks, and ASSERTs on failure
    D checks, and SPEWs on failure
    (yes, the 'A' is inconsistent)
    Ex adds overriding failure-case 'E_*' of user's choice
    x is obsolete, you shouldn't use it

    n.b. we only have some of the above implemented (the ones we've needed
    so far...)

    if you're an unlucky soul and run into a label collision, use the
    _ehmErrorLab mechanism (see below).

    see also the document: <todo: ptr to in kk's ehm .doc>

NOTES
    we use the "if (0,fResult), while (0,0)" style in order to suppress W4 warnings.  the PF_EXPR wraps that to keep prefix happy.

*/
#ifndef _EHM_H_
#define _EHM_H_

// 99% of the time ehm.h "just works".  however 1% of the time the "Error"
// label is already in use (e.g. for some other macro package).  if you hit
// that, do this:
//      #define _ehmErrorLab    EhmError
//      #include <ehm.h>
// ... and then use EhmError as your label.  it may seem a bit silly to add
// this extra indirection vs. just use EhmError always, but given that it's
// a 99%/1%, it seems worthwhile.
//
// our suggestion is that custom clients standardize on "EhmError", but if
// need be, they can choose whatever they want.
#ifndef _ehmErrorLab
#define _ehmErrorLab   Error
#endif

typedef enum
    {
    eHRESULT,
    eBOOL,
    ePOINTER,
    eWINDOWS
    } eCodeType;

#ifdef DEBUG
BOOL OnAssertionFail(eCodeType eType, DWORD dwCode, const TCHAR* pszFile, unsigned long ulLine, const TCHAR* pszMessage);
#endif

// Check HRESULT
#define _CHREx0(hResult)\
    do {hr = (hResult); if(FAILED(hr)) goto _ehmErrorLab;} while (0,0)
#define _CHR(hResult, hrFail)\
    do {hr = (hResult); if(FAILED(hr)) { hr = (hrFail); goto _ehmErrorLab;} } while (0,0)

// Check pointer result
#define _CPR(p, hrFail)\
    do {if (PF_EXPR(!(p))) {hr = (hrFail); goto _ehmErrorLab;} } while (0,0)

// Check boolean result
#define _CBR(fResult, hrFail)\
    do {if (PF_EXPR(!(fResult))) {hr = (hrFail); goto _ehmErrorLab;}} while(0,0)

// Check windows result.  Exactly like CBR for the non-Asserting case
#define _CWR _CBR

// The above macros with Asserts when the condition fails
#ifdef DEBUG

#define _CHRAEx0(hResult)\
    do\
        {\
        hr = (hResult);\
        if(FAILED(hr))\
            {\
            if(OnAssertionFail(eHRESULT, hr, TEXT(__FILE__), __LINE__, TEXT("CHRA(") TEXT( # hResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            goto _ehmErrorLab;\
            }\
        }\
    while (0,0)

#define _CHRA(hResult, hrFail)\
    do\
        {\
        hr = (hResult);\
        if(FAILED(hr))\
            {\
            hr = (hrFail);\
            if(OnAssertionFail(eHRESULT, hr, TEXT(__FILE__), __LINE__, TEXT("CHRA(") TEXT( # hResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            goto _ehmErrorLab;\
            }\
        }\
    while (0,0)

#define _CPRA(p, hrFail)\
    do\
        {\
        if (PF_EXPR(!(p)))\
            {\
            hr = (hrFail);\
            if(OnAssertionFail(ePOINTER, 0, TEXT(__FILE__), __LINE__, TEXT("CPRA(") TEXT( # p ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            goto _ehmErrorLab;\
            }\
        }\
    while (0,0)

#define _CBRA(fResult, hrFail)\
    do\
        {\
        if (PF_EXPR(!(fResult)))\
            {\
            hr = (hrFail);\
            if(OnAssertionFail(eBOOL, 0, TEXT(__FILE__), __LINE__, TEXT("CBRA(") TEXT( # fResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            goto _ehmErrorLab;\
            }\
        }\
    while (0,0)

#define _CWRA(fResult, hrFail)\
    do\
        {\
        if (PF_EXPR(!(fResult)))\
            {\
            hr = (hrFail);\
            if(OnAssertionFail(eWINDOWS, 0, TEXT(__FILE__), __LINE__, TEXT("CWRA(") TEXT( # fResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            goto _ehmErrorLab;\
            }\
        }\
    while (0,0)


#define VBR(fResult)\
    do\
        {\
        if (PF_EXPR(!(fResult)))\
            {\
            if(OnAssertionFail(eBOOL, 0, TEXT(__FILE__), __LINE__, TEXT("VBR(") TEXT( # fResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            }\
        }\
    while (0,0)

#define VPR(fResult)\
    do\
        {\
        if (PF_EXPR(!(fResult)))\
            {\
            if(OnAssertionFail(eBOOL, 0, TEXT(__FILE__), __LINE__, TEXT("VPR(") TEXT( # fResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            }\
        }\
    while (0,0)


// Verify Windows Result
#define VWR(fResult)\
    do\
        {\
        if (!(PF_EXPR(NULL != (fResult))))\
            {\
            if(OnAssertionFail(eWINDOWS, 0, TEXT(__FILE__), __LINE__, TEXT("VWR(") TEXT( # fResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            }\
        }\
    while (0,0)


// Verify HRESULT (careful not to modify hr)
#define VHR(hResult)\
    do\
        {\
        HRESULT _EHM_hrTmp = (hResult);\
        if(FAILED(_EHM_hrTmp))\
            {\
            if(OnAssertionFail(eHRESULT, _EHM_hrTmp, TEXT(__FILE__), __LINE__, TEXT("VHR(") TEXT( # hResult ) TEXT(")")))\
                {\
                DebugBreak();\
                }\
            }\
        }\
    while (0,0)

// make sure you keep the xTmp, can only eval arg 1x
// todo: dump GetLastError in DWR
#define DWR(fResult) \
    do { if (PF_EXPR(!(fResult))) {DEBUGMSG(1, (TEXT("DWR(") TEXT( # fResult ) TEXT(")\r\n") ));}} while (0,0)
#define DHR(hResult) \
    do { HRESULT hrTmp = hResult; if(FAILED(hrTmp)) {DEBUGMSG(1, (TEXT("DHR(") TEXT( # hResult ) TEXT(")=0x%x\r\n"), hrTmp));}} while (0,0)
#define DPR     DWR     // tmp
#define DBR     DWR     // tmp

#define CHRA(e) _CHRAEx0(e)
#define CPRA(e) _CPRA(e, E_OUTOFMEMORY)
#define CBRA(e) _CBRA(e, E_FAIL)
#define CWRA(e) _CWRA(e, E_FAIL)
#define CHRAEx(e, hrFail) _CHRA(e, hrFail)
#define CPRAEx(e, hrFail) _CPRA(e, hrFail)
#define CBRAEx(e, hrFail) _CBRA(e, hrFail)
#define CWRAEx(e, hrFail) _CWRA(e, hrFail)
#else
#define CHRA CHR
#define CPRA CPR
#define CBRA CBR
#define CWRA CWR
#define CHRAEx CHREx
#define CPRAEx CPREx
#define CBRAEx CBREx
#define CWRAEx CWREx
#define VHR(x) (x)
#define VPR(x) (x)
#define VBR(x) (x)
#define VWR(x) (x)
#define DHR(x) (x)
#define DPR(x) (x)
#define DBR(x) (x)
#define DWR(x) (x)
#endif

#define CHR(e) _CHREx0(e)
#define CPR(e) _CPR(e, E_OUTOFMEMORY)
#define CBR(e) _CBR(e, E_FAIL)
#define CWR(e) _CWR(e, E_FAIL)
#define CHREx(e, hrFail) _CHR(e, hrFail)
#define CPREx(e, hrFail) _CPR(e, hrFail)
#define CBREx(e, hrFail) _CBR(e, hrFail)
#define CWREx(e, hrFail) _CWR(e, hrFail)

// obsolete (but still in use)
// - work around various pseudo-pblms:
//  partial images, CEPC no-radio, etc.
// - also things that we want to know about in dev, but not in QA/stress:
//  an e.g. is our DoVerb stuff, there are 'good' failures (END when no call,
// or TALK w/ 0 entries) and 'bad' failures (e.g. TAPI returns an error), we
// don't want to int3 during stress but we do want to on our dev machines
//
// eventually we'll make these do "if (g_Assert) int3;", then we
// can run w/ g_Assert on for dev and off for stress.
#define xCHRA   CHR     // should be CHRA but...
#define xCPRA   CPR     // should be CPRA but...
#define xCBRA   CBR     // should be CBRA but...
#define xCWRA   CWR     // should be CWRA but...
#define xVHR    DHR     // should be VHR  but...
#define xVPR    DPR     // should be VPR  but...
#define xVBR    DBR     // should be VBR  but...
#define xVWR    DWR     // should be VWR  but...

#endif // _EHM_H_
