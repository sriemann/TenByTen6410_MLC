// **************************************************************************
// GUTS.H
// 
// This file is used for project wide defines.
//
// Copyright 2000 Microsoft Corporation, All Rights Reserved
//

#pragma once

//
// Enabling watson for OZUP
//
// This must be turned off *BEFORE* Ozone update ships.
//
#ifndef WATSON_ENABLED
#define WATSON_ENABLED
#endif // !WATSON_ENABLED

//
//
// Common project defines (for Pocket PC, Smartfon, etc.)
//
//

// Misc. typedefs
typedef UINT        BITBOOL;

// Misc. macros
#ifndef RECTWIDTH
#define RECTWIDTH(rc)    ((rc).right - (rc).left)
#endif

#ifndef RECTHEIGHT
#define RECTHEIGHT(rc)   ((rc).bottom - (rc).top)
#endif

#ifndef PRECTWIDTH
#define PRECTWIDTH(prc)  ((prc)->right - (prc)->left)
#endif

#ifndef PRECTHEIGHT
#define PRECTHEIGHT(prc) ((prc)->bottom - (prc)->top)
#endif

#ifndef ARRAYSIZE
#define ARRAYSIZE(a)   (sizeof(a)/sizeof(a[0]))
#endif

#ifndef COMPILETIME_ASSERT
#define COMPILETIME_ASSERT(f) switch (0) case 0: case f:
#endif

#ifndef BOOLIFY
#define BOOLIFY(e)      (!!(e))
#endif

#ifndef SIZEOF
#define SIZEOF(a)       sizeof(a)
#endif

#ifndef ISARRAYMEMBER
#define ISARRAYMEMBER(array, member) \
    ((member) && \
    ((array) <= (member)) && \
    ((member) < (array)+ARRAYSIZE(array)) && \
    (((((BYTE*)(member)) - ((BYTE*)(array))) % sizeof(*(array))) == 0))
#endif

#ifndef SAFECAST
#define SAFECAST(p, c)  (static_cast<c> (p))
#endif

// BUGBUG Do we need this?  Waiting for email from AndyP  6/19/00
//***   CASSERT -- compile-time assert
#ifndef CASSERT
#define CASSERT(e)  extern int dummary_array[(e)]
#endif

// Need a real implementation of this...
#ifndef ASSERTSZ
#ifdef DEBUG
#define ASSERTSZ(x, sz) do {if(!(x)) {OutputDebugString(TEXT(sz)); ASSERT(0);}} while (0,0)
#else
#define ASSERTSZ(x, sz)     ((void)0)
#endif
#endif

// PREfix modified forms of ASSERT and VERIFY.  These are the same as
// the ordinary ones, except that they handle PREfix simulations

#ifdef ASSERT
#undef ASSERT
#endif

#ifdef VERIFY
#undef VERIFY
#endif

#if defined(DEBUG) || defined(_DEBUG)
#define __GUTSTEXT(quote) L##quote
#define GUTSTEXT(quote) __GUTSTEXT(quote)
#define ASSERT(x)   ( (x) || (RETAILMSG(1, (GUTSTEXT("ASSERT FAILURE at %s line %d\r\n"), GUTSTEXT(__FILE__), __LINE__)), DebugBreak(), 0) )
#else
#ifdef _PREFIX_
#define ASSERT(x)   ( (x) || (ExitThread(0), 0) )
#else
#define ASSERT(x)   ((void) 0)
#endif	// _PREFIX_
#endif // defined(DEBUG) || defined(_DEBUG)

#if defined(DEBUG) || defined(_DEBUG) || defined(_PREFIX_)
#define VERIFY(f)   ASSERT(f)
#else
#define VERIFY(f)   ((void)(f))
#endif  // defined(DEBUG) || defined(_DEBUG) || defined(_PREFIX_)



// Macro to make sure that a function is always called on the same thread.
// (not that there may be some multithreaded re-entrancy problems with
// this but it should generally work)

// REVIEW: This code could be made better by using InterlockedTestExchange
// instead of an explicit if test.  If we ever actually see this fail, we
// should fix it.
 
#ifdef DEBUG
#define ASSERT_SAME_THREAD() \
    do \
        {\
        static DWORD s_dwThreadID = 0;\
        DWORD dwThreadID = GetCurrentThreadId(); \
        if(0 == s_dwThreadID) \
            { \
            s_dwThreadID = dwThreadID; \
            } \
        else \
            { \
            ASSERTSZ(s_dwThreadID == dwThreadID, "Multiple threads calling function that only expects to be called on one thread"); \
            } \
        } \
    while(FALSE)
#else
#define ASSERT_SAME_THREAD()
#endif

#ifndef RC_INVOKED
// Macro for doing funky error handling (see coding conventions doc)
// DON'T define this for .rc files
#define BEGIN for (;;) {
#define END   }
#endif

// Selective promotion of warnings to errors.  We support three
// different groups of promoted level 4 warnings: "ignore",
// "required", and "desired".  By default, all components must compile
// with PW_REQUIRED set.

#define _PW_IGNORE    0
#define _PW_REQUIRED  1
#define _PW_DESIRED   2

// BOGUSBOGUS: This should be _PW_REQUIRED, but that's for later on.

#ifndef PW_LEVEL
#define PW_LEVEL     _PW_IGNORE
#endif  // !defined(PW_LEVEL)

// Components are required to contain none of these warnings

#if (PW_LEVEL >= _PW_REQUIRED)

// enumerate 'identifier' in switch of enum 'identifier' is not
// explicitly handled by a case label
#pragma warning( 3 : 4061)

// The operator was used with the address of a string
// literal. Unexpected code was generated.  (this catches things like
// the following: psz = "AString"; if (psz == "AString")...
#pragma warning( 3 : 4130) 

// The constant was not initialized.
#pragma warning( 3 : 4132) 

// 'identifier' : local variable is initialized but not referenced"
#pragma warning( 3 : 4189)

// You attempted to convert a signed const that has a negative value
// to an unsigned.
#pragma warning( 3 : 4245) 

// A const global or static instance of a non-trivial class is
// initialized with a compiler-generated default constructor
#pragma warning( 3 : 4268) 

// The given function is local and not referenced in the body of the
// module; therefore, the function is dead code
#pragma warning( 3 : 4505) 

// A user-defined copy constructor for the specified thrown object is
// not accessible. The object cannot be constructed when it is
// thrown. Check that the copy constructor has public access
#pragma warning( 3 : 4671) 

// A user-defined destructor for the specified thrown object is not
// accessible. The object cannot be destroyed after it is
// thrown. Check that the destructor has public access.
#pragma warning( 3 : 4674)

// You may have used the local variable name without first assigning
// it a value, which could lead to unpredictable results.
#pragma warning( 3 : 4701) 

// An instruction will never be executed.
#pragma warning( 3 : 4702) 

// statement has no effect
#pragma warning( 3 : 4705)

#endif // (PW_LEVEL >= _PW_REQUIRED)


// Components are supposed to contain none of these warnings

#if (PW_LEVEL >= _PW_DESIRED)

// This warning can be caused by mixing signed and unsigned types, and
// short and long types
#pragma warning( 3 : 4057) 

// 'identifier' : unreferenced formal parameter
#pragma warning( 3 : 4100)

// Microsoft C++ allows you to give a value within brackets when
// deleting an array with the delete operator. The value in brackets
// is ignored.
#pragma warning( 3 : 4208) 

// Microsoft C/C++ allows redefinition of an extern identifier as
// static.
#pragma warning( 3 : 4211) 

// The function prototype had variable arguments, but the function
// definition did not.
#pragma warning( 3 : 4212) 

// There was an attempt to compare pointers to functions that had
// similar parameters, but with one having variable arguments.
#pragma warning( 3 : 4220) 

// Microsoft C/C++ allows you to initialize an aggregate type (array,
// struct, or union) with the address of a local (automatic) variable.
#pragma warning( 3 : 4221) 

// A nonstatic value was given as the address of a function declared
// with the dllimport modifier.
#pragma warning( 3 : 4232) 

// The compiler could not resolve the type after looking ahead in the
// code
#pragma warning( 3 : 4504) 

// After an identifier was declared with the default C++ linkage, it
// was explicitly declared as having C++ linkage
#pragma warning( 3 : 4507) 

// You created a namespace that uses itself.
#pragma warning( 3 : 4515) 

// The test value in a conditional expression was the result of an
// assignment.
#pragma warning( 3 : 4706) 

// A function contains a recursive call, but otherwise has no side
// effects. A call to this function is being deleted
#pragma warning( 3 : 4718) 

// An unknown character was found in the optimization list of an
// optimize pragma statement
#pragma warning( 3 : 4918) 

#endif // (PW_LEVEL >= _PW_DESIRED)

#ifdef _PREFAST_
//
// Standard Naming:     PF_*
//                  Keep it SHORT
//
// Current version of PREFast chokes on "if (!(0,0))" so we
// wrote these MACRO helpers.
//
// The reason we have the (0,fResult) thing to begin with 
// (rather than just 'fResult') is that the latter causes warnings 
// "constant in conditional expr" for things like "if (1) ..." and 
// "if (0) ..." (which are what ASSERTs and even non-ASSERTs 
// can sometimes end up generating in a macro-laden world).
//
#pragma warning(disable: 4127)
#define PF_EXPR(fResult)     (fResult)
#else
#define PF_EXPR(fResult)     (0,(fResult))
#endif

