/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_STRNOOB_H
#define PSXF_GUARD_STRNOOB_H

#include "../psx.h"

// Just a header that have the psn00b mdec declarations
typedef struct {
  u16 ac0[2];
  u32 ac2[8], ac3[64];
  u16 ac4[8], ac5[8], ac7[16], ac8[32];
  u16 ac9[32], ac10[32], ac11[32], ac12[32];
} VLC_TableV2;

typedef struct {
  u16 ac0[2];
  u32 ac2[8], ac3[64];
  u16 ac4[8], ac5[8], ac7[16], ac8[32];
  u16 ac9[32], ac10[32], ac11[32], ac12[32];
  u8  dc[128], dc_len[9];
  u8  _reserved[3];
} VLC_TableV3;

typedef enum {
  DECDCT_MODE_24BPP   = 1,
  DECDCT_MODE_16BPP   = 0,
  DECDCT_MODE_16BPP_BIT15 = 2,
  DECDCT_MODE_RAW     = -1
} DECDCTMODE;

typedef struct {
  const u32  *input;
  u32    window, next_window, remaining;
  s8      is_v3, bit_offset, block_index, coeff_index;
  u16    quant_scale;
  s16     last_y, last_cr, last_cb;
} VLC_Context;

typedef struct {
  u32 mdec0_header;
  u16 quant_scale;
  u16 version;
} BS_Header;


int DecDCTvlcStart(VLC_Context *ctx, u32 *buf, size_t max_size, const u32 *bs);
int DecDCTvlcContinue(VLC_Context *ctx, u32 *buf, size_t max_size);
void DecDCTvlcCopyTableV2(VLC_TableV2 *addr);
void DecDCTvlcCopyTableV3(VLC_TableV3 *addr);

#endif