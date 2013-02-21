//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this source code is subject to the terms of your Microsoft Windows CE
// Source Alliance Program license form.  If you did not accept the terms of
// such a license, you are not authorized to use this source code.
//

#include <windows.h>
#include "aes_defs.h"
#include "aes.h"


AESTable g_KSKeyTable;

u32b*  AES_SetKey(const u32b in_key[], const u32b key_len)
{
    if (key_len > 0) {
        aeskey(&g_KSKeyTable, (BYTE*)in_key, AES_ROUNDS_128);
    } else {
        SecureZeroMemory(&g_KSKeyTable, sizeof(g_KSKeyTable));
    }

    return (u32b*)&g_KSKeyTable;
}


void AES_Encrypt(const u32b in_blk[4], u32b *out_blk)
{
    aes((BYTE*)out_blk, (BYTE*)in_blk, &g_KSKeyTable, 1);
}


void AES_Decrypt(const u32b in_blk[4], u32b *out_blk)
{
    aes((BYTE*)out_blk, (BYTE*)in_blk, &g_KSKeyTable, 0);
}


