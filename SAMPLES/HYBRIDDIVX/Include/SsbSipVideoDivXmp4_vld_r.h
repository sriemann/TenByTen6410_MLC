/**
 ** Copyright (C) 2002 DivXNetworks, all rights reserved.
 **
 ** DivXNetworks, Inc. Proprietary & Confidential
 **
 ** This source code and the algorithms implemented therein constitute
 ** confidential information and may comprise trade secrets of DivXNetworks
 ** or its associates, and any use thereof is subject to the terms and
 ** conditions of the Non-Disclosure Agreement pursuant to which this
 ** source code was originally received.
 **
 **/

/** $Id: SsbSipVideoDivXmp4_vld_r.h,v 1.1.1.1 2003/04/23 23:24:25 c0redumb Exp $
 **
 **/

/*************************************************************************/

// mp4_vldr.h //

/** Reversible VLD using mixed solution lookup + length sorting
**/
/*******************************************************************************
            Samsung India Software Operations Pvt. Ltd. (SISO)
                    Copyright 2006
;*******************************************************************************/

#ifndef ___SSBSIPVLDR_H__
#define ___SSBSIPVLDR_H__

#ifdef __cplusplus
extern "C"
{
#endif

/*
// -----------------------------------------------------------------------------
// Struct/Union Types and define
// -----------------------------------------------------------------------------
*/
#define ERR (65535) // 0xFFFF
#define ESC (7167)

// codes beginning with "1" and with lenght > 10
// sorted according their length and the last bit before the sign bit

STATIC tab_type SsbSipVideotableR1_intra[] =
{  
  {72449, 11}, {72705, 11}, {74241, 12}, {74497, 12}, {74753, 13}, {75009, 13}, {75521, 14}, {75777, 14}
};

STATIC tab_type SsbSipVideotableR1_inter[] =
{ 
  {72449,11}, {72705,11}, {74241,12}, {74497,12}, {74753,13}, {75009,13}, {75521,14}, {75777,14}
};

// codes beginning with "0" and with length > 10
// sorted according the second "0" position and the last bit before the sign bit

STATIC tab_type SsbSipVideotableR0_intra11bits[] =
{
  {3329,11},    {2306,11},    {1283,11},    {1539,11},    {1795,11},    {772,11},    {517,11},    {518,11},    {264,11},    {265,11},
  {15,11},    {16,11},    {17,11},    {65539,11},    {66050,11},    {72193,11}
};

STATIC tab_type SsbSipVideotableR0_intra12bits[] =
{
  {2562,12},    {1028,12},    {1284,12},    {1540,12},    {773,12},    {1029,12},    {266,12},    {18,12},    {19,12},    {22,12},
  {65795,12},    {66306,12},    {66562,12},    {72961,12},    {73217,12},    {73473,12},    {73729,12},    {73985,12}
};

STATIC tab_type SsbSipVideotableR0_intra13bits[] =
{
  {3585,13},  {3841,13},  {2818,13},  {2051,13},  {2307,13},  {1796,13},  {774,13},  {519,13},  {520,13},  {521,13},
  {267,13},  {20,13},  {21,13},  {23,13},  {65540,13},  {66818,13},  {67074,13},  {67330,13},  {67586,13},  {67842,13}
};

STATIC tab_type SsbSipVideotableR0_intra14bits[] =
{
  {4097,14},  {4353,14},  {4609,14},  {2052,14},  {1285,14},  {1030,14},  {1286,14},  {775,14},  {776,14},  {522,14},
  {523,14},  {268,14},  {269,14},  {24,14},  {25,14},  {26,14},  {65541,14},  {65796,14},  {68098,14},  {68354,14},  {68610,14},
  {75265,14}
};

STATIC tab_type SsbSipVideotableR0_intra15bits[] =
{
  {27,15},  {777,15},  {1541,15},  {1797,15},  {2308,15},  {3074,15},  {4865,15},  {65797,15},  {66051,15},  {68866,15},  {76033,15},
  {76289,15},  {76545,15},  {76801,15}
};

//

STATIC tab_type SsbSipVideotableR0_inter11bits[] =
{
  {10,11},  {11,11},  {262,11},   {516,11},  {1027,11},  {1283,11},  {2562,11},  {5377,11},  {5633,11},  {5889,11},
  {6145,11},  {6401,11},  {6657,11},  {65539,11},  {66050,11},  {72193,11}
};

STATIC tab_type SsbSipVideotableR0_inter12bits[] =
{
  {12,12},  {263,12},  {517,12},  {772,12},  {1539,12},  {1795,12},  {2818,12},  {6913,12},  {7169,12},  {7425,12},
  {65795,12},  {66306,12},  {66562,12},  {72961,12},  {73217,12},  {73473,12},  {73729,12},  {73985,12}
};

STATIC tab_type SsbSipVideotableR0_inter13bits[] =
{
  {13,13},  {14,13},  {15,13},  {16,13},  {264,13},  {773,13},  {1028,13},  {1284,13},  {2051,13},  {3074,13},
  {7681,13},  {7937,13},  {8193,13},  {8449,13},    {65540,13},  {66818,13},  {67074,13},  {67330,13},  {67586,13},  {67842,13}
};

STATIC tab_type SsbSipVideotableR0_inter14bits[] =
{
  {17,14},  {18,14},  {265,14},  {266,14},  {518,14},  {519,14},  {774,14},  {1540,14},  {2307,14},  {3330,14},
  {3586,14},  {3842,14},  {4098,14},  {8705,14},  {8961,14},  {9217,14},  {65541,14},  {65796,14},  {68098,14},  {68354,14},
  {68610,14},  {75265,14}
};

STATIC tab_type SsbSipVideotableR0_inter15bits[] =
{
  {19,15},  {775,15},  {1029,15},  {1796,15},  {4354,15},  {9473,15},  {9729,15},  {65797,15},  {66051,15},  {68866,15},
  {76033,15},  {76289,15},  {76545,15},  {76801,15}
};

// all the other codes, code length from 4 to 10 (0000 to 1000000011)
// 0xffff (65535, 0) is a not valid code

STATIC tab_type SsbSipVideotableRmain_intra[] =
{
  {ESC,4}, {257,4}, {ERR,0}, {ERR,0}, {513,5}, {769,5}, {1,3}, {2,3}, {258,5}, {4,5},
  {3,4}, {65537,4}, {1025,6}, {1281,6}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {65793,5}, {66049,5},
  {5,6}, {6,6}, {ERR,0}, {ERR,0}, {66305,6}, {66561,6}, {ERR,0}, {ERR,0}, {1537,7}, {1793,7},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {66817,6}, {67073,6}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {514,7}, {259,7}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {7,7}, {67329,7}, {ERR,0}, {ERR,0}, {67585,7}, {67841,7}, {ERR,0}, {ERR,0},
  {2049,8}, {2305,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {68097,7}, {68353,7}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {770,8}, {1026,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {260,8}, {261,8},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {8,8}, {9,8}, {ERR,0}, {ERR,0},
  {65538,8}, {68609,8}, {ERR,0}, {ERR,0}, {2561,9}, {1282,9}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {68865,8}, {69121,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {515,9}, {771,9},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {262,9}, {10,9}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {11,9}, {65794,9}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {69377,9}, {69633,9}, {ERR,0}, {ERR,0}, {69889,9}, {70145,9},
  {ERR,0}, {ERR,0}, {2817,10}, {3073,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {70401,9}, {70657,9},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {1538,10}, {1794,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {2050,10}, {1027,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {516,10}, {263,10}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {12,10}, {13,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {14,10}, {70913,10}, {ERR,0}, {ERR,0}, {71169,10}, {71425,10}, {ERR,0}, {ERR,0}, {3329,11}, {2306,11},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {71681,10}, {71937,10}
};

STATIC tab_type SsbSipVideotableRmain_inter[] =
{
  {ESC,4}, {2,4}, {ERR,0}, {ERR,0}, {3,5}, {769,5}, {1,3}, {257,3}, {1025,5}, {1281,5},
  {513,4}, {65537,4}, {258,6}, {1537,6}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {65793,5}, {66049,5},
  {1793,6}, {2049,6}, {ERR,0}, {ERR,0}, {66305,6}, {66561,6}, {ERR,0}, {ERR,0}, {4,7},  {514,7},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {66817,6}, {67073,6}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {2305,7}, {2561,7}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {2817,7}, {67329,7}, {ERR,0}, {ERR,0}, {67585,7}, {67841,7}, {ERR,0}, {ERR,0},
  {5,8}, {6,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {68097,7}, {68353,7}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {259,8}, {770,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {1026,8}, {3073,8}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {3329,8}, {3585,8}, {ERR,0}, {ERR,0}, {65538,8},
  {68609,8}, {ERR,0}, {ERR,0}, {7,9}, {260,9}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {68865,8},
  {69121,8}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {515,9}, {1282,9}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {3841,9},
  {4097,9}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {4353,9}, {65794,9}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {69377,9}, {69633,9}, {ERR,0}, {ERR,0}, {69889,9}, {70145,9}, {ERR,0},
  {ERR,0}, {8,10}, {9,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {70401,9}, {70657,9}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {261,10},
  {771,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {1538,10}, {1794,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {2050,10}, {2306,10}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0},
  {ERR,0}, {4609,10}, {4865,10}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {ERR,0}, {5121,10},
  {70913,10}, {ERR,0}, {ERR,0}, {71169,10}, {71425,10}, {ERR,0}, {ERR,0}, {10,11}, {11,11}, {ERR,0},
  {ERR,0}, {ERR,0}, {ERR,0}, {71681,10}, {71937,10}
};
EXPORT event_t rvld_intra_dct(REFERENCE * ref,unsigned int *zigzag,int *i,int *m) ;
EXPORT event_t rvld_inter_dct(REFERENCE * ref,unsigned int *zigzag,int *i,int *m) ;
#ifdef __cplusplus
extern "C"
}
#endif
#endif // ___SSBSIPVLDR_H__


