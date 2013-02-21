;/*************************************************************************************
; 
;	Project Name : S3C6400 Validation
;
;	Copyright 2006 by Samsung Electronics, Inc.
;	All rights reserved.
;
;	Project Description :
;		This software is only for validating functions of the S3C6400.
;		Anybody can use this software without our permission.
;
;--------------------------------------------------------------------------------------
;
;	File Name : option.inc
;  
;	File Description : This file defines basic setting and configuration.
;
;	Author	: Haksoo,Kim
;	Dept. : AP Development Team
;	Created Date : 2006/11/08
;	Version : 0.1 
; 
;	History
;	- Created(Haksoo,Kim 2006/11/08)
; 	- Added PLL M,P,S Value (wonjoon.jang 2007/01/31) 
;*************************************************************************************/

					GBLL	S3C6400
S3C6400				SETL	{TRUE}

					GBLL	SILICON
SILICON				SETL	{TRUE}

					GBLL	SYNCMODE
SYNCMODE			SETL	{TRUE}

					GBLL	ONE_ERROR
ONE_ERROR			SETL	{TRUE}


					GBLS	DMC
DMC					SETS	"DMC1"				
					
					GBLS	DRAM
DRAM				SETS	"DDR"									

 [ (DMC = "DMC0")
DRAM_BaseAddress	EQU		(0x40000000)
 ]
 [ (DMC = "DMC1")
DRAM_BaseAddress	EQU		(0x50000000)
	]
	

;_ISR_STARTADDRESS	EQU	(DRAM_BaseAddress + 0x03ffff00) ; 0x53ffff00	; 64MB case
_ISR_STARTADDRESS	EQU	(DRAM_BaseAddress + 0x07ffff00) ; 0x57ffff00	; 128MB case
top_of_stacks		EQU	_ISR_STARTADDRESS
;_STACK_BASEADDRESS	EQU (DRAM_BaseAddress + 0x03ffff00) ; 0x53ffff00

;; Clock Define
FIN				EQU	12000000

SYNC_HCLK			EQU	1
ASYNC_HCLK			EQU	0
										
;ARM: 532MHZ, AXI/AHB: 133MHz APB: 33MHz
 [ (1 = 0)
;VCO=1064MHz, Fout=532MHz
APLL_MVAL		EQU	532
APLL_PVAL		EQU	6
APLL_SVAL		EQU	1
;VCO=1064MHz, Fout=266MHz
MPLL_MVAL		EQU	532
MPLL_PVAL		EQU	6
MPLL_SVAL		EQU	2

APLL_DIV		EQU	0
MPLL_DIV		EQU	1	;DOUT_MPLL = MPLL_Fout/2
HCLK_DIV		EQU	1	;AHB_CLK = HCLKx2/2
   [ (SYNC_HCLK = 1)
HCLKx2_DIV		EQU	(1)	;HCLKx2 = APLL_Fout/2
   ]
   [ (ASYNC_HCLK = 1)
HCLKx2_DIV		EQU	(0)	;HCLKx2 = MPLL_Fout
   ]
PCLK_DIV		EQU	7	;PCLK = HCLKx2/8
OND_DIV			EQU 3 ;OND_CLK=HCLKx2/4
	]
	
;ARM: 532MHZ, AXI/AHB: 133MHz APB: 66MHz
 [ ( 1 = 0)
;VCO=1064MHz, Fout=532MHz
APLL_MVAL		EQU	532
APLL_PVAL		EQU	6
APLL_SVAL		EQU	1
;VCO=1064MHz, Fout=266MHz
MPLL_MVAL		EQU	532
MPLL_PVAL		EQU	6
MPLL_SVAL		EQU	2

APLL_DIV		EQU	0
MPLL_DIV		EQU	1	;DOUT_MPLL = MPLL_Fout/2
HCLK_DIV		EQU	1	;AHB_CLK = HCLKx2/2
   [ (SYNC_HCLK = 1)
HCLKx2_DIV		EQU	1	;HCLKx2 = APLL_Fout/2
   ]
   [ (ASYNC_HCLK = 1)
HCLKx2_DIV		EQU	0	;HCLKx2 = MPLL_Fout
   ]
PCLK_DIV		EQU	3	;PCLK = HCLKx2/4
OND_DIV			EQU 3 ;OND_CLK=HCLKx2/4
 ]

;ARM: 400MHZ, AXI/AHB: 100MHz APB: 50MHz
 [ ( 1 = 0)
;VCO=1600MHz, Fout=400MHz
APLL_MVAL		EQU	400
APLL_PVAL		EQU	3
APLL_SVAL		EQU	2
;VCO=1600MHz, Fout=200MHz
MPLL_MVAL		EQU	400
MPLL_PVAL		EQU	3	
MPLL_SVAL		EQU	3

APLL_DIV		EQU	0
MPLL_DIV		EQU	1	;DOUT_MPLL = MPLL_Fout/2
HCLK_DIV		EQU	1	;AHB_CLK = HCLKx2/2
   [ (SYNC_HCLK = 1)
HCLKx2_DIV		EQU	1	;HCLKx2 = APLL_Fout/2
   ]
   [ (ASYNC_HCLK = 1)
HCLKx2_DIV		EQU	0	;HCLKx2 = MPLL_Fout
   ]
PCLK_DIV		EQU	3	;PCLK = HCLKx2/4
OND_DIV			EQU 3 ;OND_CLK=HCLKx2/4
	]

;ARM: 400MHZ, AXI/AHB: 100MHz APB: 25MHz
 [ ( 1 = 1)
;VCO=1600MHz, Fout=400MHz
APLL_MVAL		EQU	400
APLL_PVAL		EQU	3
APLL_SVAL		EQU	2
;VCO=1600MHz, Fout=200MHz
MPLL_MVAL		EQU	400
MPLL_PVAL		EQU	3	
MPLL_SVAL		EQU	3

APLL_DIV		EQU	0
MPLL_DIV		EQU	1	;DOUT_MPLL = MPLL_Fout/2
HCLK_DIV		EQU	1	;AHB_CLK = HCLKx2/2
   [ (SYNC_HCLK = 1)
HCLKx2_DIV		EQU	1	;HCLKx2 = APLL_Fout/2
   ]
   [ (ASYNC_HCLK = 1)
HCLKx2_DIV		EQU	0	;HCLKx2 = MPLL_Fout
   ]
PCLK_DIV		EQU	7	;PCLK = HCLKx2/8
OND_DIV			EQU 3 ;OND_CLK=HCLKx2/4
 ]

Startup_APLL		EQU	(((FIN>>APLL_SVAL)/APLL_PVAL)*APLL_MVAL)
Startup_MPLL		EQU	(((FIN>>MPLL_SVAL)/MPLL_PVAL)*MPLL_MVAL)
Startup_ARMCLK		EQU	(Startup_APLL/(APLL_DIV+1))
 [ (SYNC_HCLK = 1)
Startup_HCLK		EQU	(Startup_APLL/(HCLKx2_DIV+1)/(HCLK_DIV+1))
 ]
 [ (ASYNC_HCLK = 1)
Startup_HCLK		EQU	(Startup_MPLL/(HCLKx2_DIV+1)/(HCLK_DIV+1))
	]	
Startup_PCLK		EQU	(Startup_HCLK/(PCLK_DIV+1))




	END
