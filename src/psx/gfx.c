/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "gfx.h"

#include "mem.h"
#include "main.h"
#include "mutil.h"

//Gfx constants
#define OTLEN 8

//Gfx state
DISPENV disp[2];
DRAWENV draw[2];
u8 db;

static u32 ot[2][OTLEN];    //Ordering table length
static u8 pribuff[2][32768]; //Primitive buffer
static u8 *nextpri;          //Next primitive pointer

//Gfx functions
void Gfx_Init(void)
{
	//Reset GPU
	ResetGraph(0);
	
	//Clear screen
	RECT dst = {0, 0, 320, 480};
	ClearImage(&dst, 0, 0, 0);
	
	//Initialize display environment
	SetDefDispEnv(&disp[0], 0, 0, 320, 240);
	SetDefDispEnv(&disp[1], 0, 240, 320, 240);
	
	//Initialize draw environment
	SetDefDrawEnv(&draw[0], 0, 240, 320, 240);
	SetDefDrawEnv(&draw[1], 0, 0, 320, 240);
	
	//Set video mode depending on BIOS region
	switch(*(char*)0xbfc7ff52)
	{
		case 'E':
			SetVideoMode(MODE_PAL);
			SsSetTickMode(SS_TICK50);
			disp[0].screen.y = disp[1].screen.y = 24;
			break;
		default:
			SetVideoMode(MODE_NTSC);
			SsSetTickMode(SS_TICK60);
			break;
	}
	
	//Set draw background
	draw[0].isbg = draw[1].isbg = 1;
	setRGB0(&draw[0], 0, 0, 0);
	setRGB0(&draw[1], 0, 0, 0);
	
	//Load font
	FntLoad(960, 0);
	FntOpen(0, 8, 320, 224, 0, 100);
	
	//Initialize drawing state
	nextpri = pribuff[0];
	db = 0;
	Gfx_Flip();
	Gfx_Flip();
}

void Gfx_Quit(void)
{
	
}

void Gfx_Flip(void)
{
	//Sync
	DrawSync(0);
	VSync(0);
	
	//Apply environments
	PutDispEnv(&disp[db]);
	PutDrawEnv(&draw[db]);
	
	//Enable display
	SetDispMask(1);
	
	//Draw screen
	DrawOTag(ot[db] + OTLEN - 1);
	FntFlush(-1);
	
	//Flip buffers
	db ^= 1;
	nextpri = pribuff[db];
	ClearOTagR(ot[db], OTLEN);
}

void Gfx_SetClear(u8 r, u8 g, u8 b)
{
	setRGB0(&draw[0], r, g, b);
	setRGB0(&draw[1], r, g, b);
}

void Gfx_EnableClear(void)
{
	draw[0].isbg = draw[1].isbg = 1;
}

void Gfx_DisableClear(void)
{
	draw[0].isbg = draw[1].isbg = 0;
}

void Gfx_LoadTex(Gfx_Tex *tex, IO_Data data, Gfx_LoadTex_Flag flag)
{
	//Catch NULL data
	if (data == NULL)
	{
		sprintf(error_msg, "[Gfx_LoadTex] data is NULL");
		ErrorLock();
	}
	
	//Read TIM information
	TIM_IMAGE tparam;
	OpenTIM(data);
	ReadTIM(&tparam);
	
	if (tex != NULL)
	{
		tex->tim_mode = tparam.mode;
		tex->pxshift = (2 - (tparam.mode & 0x3));
	}
	
	//Upload pixel data to framebuffer
	if (!(flag & GFX_LOADTEX_NOTEX))
	{
		if (tex != NULL)
		{
			tex->tim_prect = *tparam.prect;
			tex->tpage = getTPage(tparam.mode, 0, tparam.prect->x, tparam.prect->y);
		}
		LoadImage(tparam.prect, (u32*)tparam.paddr);
		DrawSync(0);
	}
	
	//Upload CLUT to framebuffer if present
	if ((tparam.mode & 0x8) && !(flag & GFX_LOADTEX_NOCLUT))
	{
		if (tex != NULL)
		{
			tex->tim_crect = *tparam.crect;
			tex->clut = getClut(tparam.crect->x, tparam.crect->y);
		}
		LoadImage(tparam.crect, (u32*)tparam.caddr);
		DrawSync(0);
	}
	
	//Free data
	if (flag & GFX_LOADTEX_FREE)
		Mem_Free(data);
}

void Gfx_DrawRect(const RECT *rect, u8 r, u8 g, u8 b)
{
	//Add quad
	POLY_F4 *quad = (POLY_F4*)nextpri;
	setPolyF4(quad);
	setXYWH(quad, rect->x, rect->y, rect->w, rect->h);
	setRGB0(quad, r, g, b);
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_F4);
}

void Gfx_BlendRect(const RECT *rect, u8 r, u8 g, u8 b, u8 mode)
{
	//Add quad
	POLY_F4 *quad = (POLY_F4*)nextpri;
	setPolyF4(quad);
	setXYWH(quad, rect->x, rect->y, rect->w, rect->h);
	setRGB0(quad, r, g, b);
	setSemiTrans(quad, 1);
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_F4);
	
	//Add tpage change (this controls transparency mode)
	DR_TPAGE *tpage = (DR_TPAGE*)nextpri;
	setDrawTPage(tpage, 0, 1, getTPage(0, mode, 0, 0));
	
	addPrim(ot[db], tpage);
	nextpri += sizeof(DR_TPAGE);
}

void Gfx_DrawTexRotateCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 angle, u8 r, u8 g, u8 b)
{	
	s16 sin = MUtil_Sin(angle);
	s16 cos = MUtil_Cos(angle);

	RECT pdst = {dst->x, dst->y, dst->w / 2, dst->h / 2};

	//Get rotated points
	POINT p0 = {-pdst.w, -pdst.h};
	MUtil_RotatePoint(&p0, sin, cos);
	
	POINT p1 = { pdst.w, -pdst.h};
	MUtil_RotatePoint(&p1, sin, cos);
	
	POINT p2 = {-pdst.w,  pdst.h};
	MUtil_RotatePoint(&p2, sin, cos);
	
	POINT p3 = { pdst.w,  pdst.h};
	MUtil_RotatePoint(&p3, sin, cos);
	
	POINT d0 = {
		pdst.w + pdst.x + p0.x,
		pdst.h + pdst.y + p0.y
	};
	POINT d1 = {
		pdst.w + pdst.x + p1.x,
		pdst.h + pdst.y + p1.y
	};
	POINT d2 = {
    pdst.w + pdst.x + p2.x,
		pdst.h + pdst.y + p2.y
	};
	POINT d3 = {
    pdst.w + pdst.x + p3.x,
		pdst.h + pdst.y + p3.y
	};
	
  //Add quad
	POLY_FT4 *quad = (POLY_FT4*)nextpri;
	setPolyFT4(quad);
	setUVWH(quad, src->x, src->y, src->w, src->h);
	setXY4(quad, d0.x, d0.y, d1.x, d1.y, d2.x, d2.y, d3.x, d3.y);
	setRGB0(quad, r, g, b);
	quad->tpage = tex->tpage;
	quad->clut = tex->clut;
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_FT4);
}

void Gfx_DrawTexRotate(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 angle)
{
	Gfx_DrawTexRotateCol(tex, src, dst, angle, 128, 128, 128);
}

void Gfx_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT *dst, u8 r, u8 g, u8 b)
{
	//Add quad
	POLY_FT4 *quad = (POLY_FT4*)nextpri;
	setPolyFT4(quad);
	setUVWH(quad, src->x, src->y, src->w, src->h);
	setXYWH(quad, dst->x, dst->y, dst->w, dst->h);
	setRGB0(quad, r, g, b);
	quad->tpage = tex->tpage;
	quad->clut = tex->clut;
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_FT4);
}

void Gfx_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT *dst)
{
	Gfx_DrawTexCol(tex, src, dst, 128, 128, 128);
}

void Gfx_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, u8 r, u8 g, u8 b)
{
	//Add quad
	POLY_FT4 *quad = (POLY_FT4*)nextpri;
	setPolyFT4(quad);
	setUVWH(quad, src->x, src->y, src->w, src->h);
	setXY4(quad, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
	setRGB0(quad, r, g, b);
	quad->tpage = tex->tpage;
	quad->clut = tex->clut;
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_FT4);
}

void Gfx_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3)
{
	Gfx_DrawTexArbCol(tex, src, p0, p1, p2, p3, 128, 128, 128);
}

void Gfx_BlendTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, u8 r, u8 g, u8 b, u8 mode)
{
	//Add quad
	POLY_FT4 *quad = (POLY_FT4*)nextpri;
	setPolyFT4(quad);
	setUVWH(quad, src->x, src->y, src->w, src->h);
	setXY4(quad, p0->x, p0->y, p1->x, p1->y, p2->x, p2->y, p3->x, p3->y);
	setRGB0(quad, r, g, b);
	setSemiTrans(quad, 1);
	quad->tpage = tex->tpage | getTPage(0, mode, 0, 0);
	quad->clut = tex->clut;
	
	addPrim(ot[db], quad);
	nextpri += sizeof(POLY_FT4);
}

void Gfx_BlendTexArb(Gfx_Tex *tex, const RECT *src, const POINT *p0, const POINT *p1, const POINT *p2, const POINT *p3, u8 mode)
{
	Gfx_BlendTexArbCol(tex, src, p0, p1, p2, p3, 128, 128, 128, mode);
}