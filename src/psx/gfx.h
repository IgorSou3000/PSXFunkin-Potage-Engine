/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "psx.h"
#include "io.h"
#include "fixed.h"

//Gfx constants
#define SCREEN_WIDTH   320
#define SCREEN_HEIGHT  240
#define SCREEN_WIDTH2  (SCREEN_WIDTH >> 1)
#define SCREEN_HEIGHT2 (SCREEN_HEIGHT >> 1)

#define SCREEN_WIDEADD (SCREEN_WIDTH - 320)
#define SCREEN_TALLADD (SCREEN_HEIGHT - 240)
#define SCREEN_WIDEADD2 (SCREEN_WIDEADD >> 1)
#define SCREEN_TALLADD2 (SCREEN_TALLADD >> 1)

#define SCREEN_WIDEOADD (SCREEN_WIDEADD > 0 ? SCREEN_WIDEADD : 0)
#define SCREEN_TALLOADD (SCREEN_TALLADD > 0 ? SCREEN_TALLADD : 0)
#define SCREEN_WIDEOADD2 (SCREEN_WIDEOADD >> 1)
#define SCREEN_TALLOADD2 (SCREEN_TALLOADD >> 1)

//Gfx structures
typedef struct
{
	u32 tim_mode;
	RECT tim_prect, tim_crect;
	u16 tpage, clut;
	u8 pxshift;
} Gfx_Tex;

typedef struct
{
	u8 r;
	u8 g;
	u8 b;
} RGB;

//Gfx functions
void Gfx_Init(void);
void Gfx_Quit(void);
void Gfx_Flip(void);
void Gfx_FlipWithoutOT(void);
RECT* Gfx_GetDrawClip(void);
void Gfx_SetClear(u8 r, u8 g, u8 b);
void Gfx_EnableClear(void);
void Gfx_DisableClear(void);

typedef u8 Gfx_LoadTex_Flag;
#define GFX_LOADTEX_FREE   (1 << 0)
#define GFX_LOADTEX_NOTEX  (1 << 1)
#define GFX_LOADTEX_NOCLUT (1 << 2)
void Gfx_LoadTex(Gfx_Tex *tex, IO_Data data, Gfx_LoadTex_Flag flag);

void Gfx_DrawRect(const RECT *rect, u8 r, u8 g, u8 b);
void Gfx_BlendRect(const RECT *rect, u8 r, u8 g, u8 b, u8 mode);
void Gfx_BlitTexCol(Gfx_Tex *tex, const RECT *src, s32 x, s32 y, u8 r, u8 g, u8 b);
void Gfx_BlitTex(Gfx_Tex *tex, const RECT *src, s32 x, s32 y);
void Gfx_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 r, u8 g, u8 b);
void Gfx_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT *dst);