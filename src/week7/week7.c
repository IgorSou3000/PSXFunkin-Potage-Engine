/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week7.h"

#include "psx/archive.h"
#include "psx/mem.h"
#include "psx/random.h"
#include "psx/mutil.h"
#include "psx/timer.h"

//Week 7 background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
	
	//Car state
	fixed_t tank_x;
	fixed_t tank_timer;
	
	//Textures
	IO_Data arc_hench, arc_hench_ptr[2];
	
	Gfx_Tex tex_back0; //Foreground
	Gfx_Tex tex_back1; //Background
	Gfx_Tex tex_back2; //Sniper and Tank
	Gfx_Tex tex_back3; //Background mountains
} Back_Week7;

//Week 7 background functions
#define TANK_START_X FIXED_DEC(-400,1)
#define TANK_END_X   FIXED_DEC(400,1)
#define TANK_TIME_A FIXED_DEC(35,1)
#define TANK_TIME_B FIXED_DEC(55,1)

void Back_Week7_DrawBG(StageBack *back)
{
	Back_Week7 *this = (Back_Week7*)back;
	
	fixed_t fx, fy, fscroll;
	
	//Draw foreground
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT fg_src = {0, 0, 255, 127};
	RECT_FIXED fg_dst = {
		FIXED_DEC(-340,1) - fx,
		FIXED_DEC(18,1) - fy,
		FIXED_DEC(340,1),
		FIXED_DEC(170,1)
	};
	
	Stage_DrawTex(&this->tex_back0, &fg_src, &fg_dst, stage.camera.bzoom);
	fg_dst.x += fg_dst.w;
	fg_src.y += 128;
	Stage_DrawTex(&this->tex_back0, &fg_src, &fg_dst, stage.camera.bzoom);
	
	//Move tank
	this->tank_timer -= timer_dt;
	if (this->tank_timer <= 0)
	{
		this->tank_timer = RandomRange(TANK_TIME_A, TANK_TIME_B);
		this->tank_x = TANK_START_X;
	}
	
	if (this->tank_x < TANK_END_X)
		this->tank_x += timer_dt * 30;
	
	//Get tank position
	fscroll = FIXED_DEC(6,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);

	u8 tank_angle = this->tank_x * 0x34 / TANK_END_X;
	
	s16 tank_sin = MUtil_Sin(tank_angle);
	s16 tank_cos = MUtil_Cos(tank_angle);
	
	fixed_t tank_y = FIXED_DEC(175,1) - ((tank_cos * FIXED_DEC(180,1)) / 256);
	
	//Get tank rotated points
	POINT tank_p0 = {-45, -45};
	MUtil_RotatePoint(&tank_p0, tank_sin, tank_cos);
	
	POINT tank_p1 = { 45, -45};
	MUtil_RotatePoint(&tank_p1, tank_sin, tank_cos);
	
	POINT tank_p2 = {-45,  45};
	MUtil_RotatePoint(&tank_p2, tank_sin, tank_cos);
	
	POINT tank_p3 = { 45,  45};
	MUtil_RotatePoint(&tank_p3, tank_sin, tank_cos);
	
	//Draw tank
	RECT tank_src = {129, 1, 126, 126};
	if (animf_count & 2)
		tank_src.y += 128;
	
	POINT_FIXED tank_d0 = {
		this->tank_x + ((fixed_t)tank_p0.x * FIXED_UNIT) - fx,
		      tank_y + ((fixed_t)tank_p0.y * FIXED_UNIT) - fy
	};
	POINT_FIXED tank_d1 = {
		this->tank_x + ((fixed_t)tank_p1.x * FIXED_UNIT) - fx,
		      tank_y + ((fixed_t)tank_p1.y * FIXED_UNIT) - fy
	};
	POINT_FIXED tank_d2 = {
		this->tank_x + ((fixed_t)tank_p2.x * FIXED_UNIT) - fx,
		      tank_y + ((fixed_t)tank_p2.y * FIXED_UNIT) - fy
	};
	POINT_FIXED tank_d3 = {
		this->tank_x + ((fixed_t)tank_p3.x * FIXED_UNIT) - fx,
		      tank_y + ((fixed_t)tank_p3.y * FIXED_UNIT) - fy
	};
	
	Stage_DrawTexArb(&this->tex_back2, &tank_src, &tank_d0, &tank_d1, &tank_d2, &tank_d3, stage.camera.bzoom);
	
	//Draw sniper
	fscroll = FIXED_DEC(5,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT snipe_src = {0, 0, 128, 255};
	RECT_FIXED snipe_dst = {
		FIXED_DEC(-190,1) - fx,
		FIXED_DEC(-90,1) - fy,
		FIXED_DEC(90,1),
		FIXED_DEC(180,1),
	};
	
	fixed_t snipe_bop;
	if ((stage.song_step % 0x4) == 0)
		snipe_bop = FIXED_UNIT - ((stage.note_scroll / 24) & FIXED_LAND);
	else
		snipe_bop = 0;
	
	snipe_dst.x -= snipe_bop * 2;
	snipe_dst.y += snipe_bop * 4;
	snipe_dst.w += snipe_bop * 4;
	snipe_dst.h -= snipe_bop * 4;
	
	Stage_DrawTex(&this->tex_back2, &snipe_src, &snipe_dst, stage.camera.bzoom);
	
	//Draw ruins
	fscroll = FIXED_DEC(3,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT ruinsf_src = {0, 0, 255, 72};
	RECT_FIXED ruinsf_dst = {
		FIXED_DEC(-240,1) - fx,
		FIXED_DEC(-70,1) - fy,
		FIXED_DEC(480,1),
		FIXED_DEC(135,1)
	};
	
	Stage_DrawTex(&this->tex_back1, &ruinsf_src, &ruinsf_dst, stage.camera.bzoom);
	
	fscroll = FIXED_DEC(1,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT ruinsb_src = {0, 72, 255, 44};
	RECT_FIXED ruinsb_dst = {
		FIXED_DEC(-240,1) - fx,
		FIXED_DEC(-60,1) - fy,
		FIXED_DEC(480,1),
		FIXED_DEC(135,1)
	};
	
	Stage_DrawTex(&this->tex_back1, &ruinsb_src, &ruinsb_dst, stage.camera.bzoom);
	
	//Draw clouds
	fscroll = FIXED_DEC(1,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT cloud_src = {0, 116, 255, 53};
	RECT_FIXED cloud_dst = {
		FIXED_DEC(-260,1) - fx,
		FIXED_DEC(-130,1) - fy,
		FIXED_DEC(260,1),
		FIXED_DEC(260,1) * 53 / 256
	};
	
	Stage_DrawTex(&this->tex_back1, &cloud_src, &cloud_dst, stage.camera.bzoom);
	cloud_dst.x += cloud_dst.w;
	cloud_dst.h = cloud_dst.w * 83 / 256;
	cloud_src.y = 173;
	cloud_src.h = 82;
	Stage_DrawTex(&this->tex_back1, &cloud_src, &cloud_dst, stage.camera.bzoom);
	
	//Draw mountains
	fscroll = FIXED_DEC(1,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT mountain_src = {0, 0, 255, 127};
	RECT_FIXED mountain_dst = {
		FIXED_DEC(-260,1) - fx,
		FIXED_DEC(-110,1) - fy,
		FIXED_DEC(260,1),
		FIXED_DEC(130,1)
	};
	
	Stage_DrawTex(&this->tex_back3, &mountain_src, &mountain_dst, stage.camera.bzoom);
	mountain_dst.x += mountain_dst.w;
	mountain_src.y += 128;
	Stage_DrawTex(&this->tex_back3, &mountain_src, &mountain_dst, stage.camera.bzoom);
}

void Back_Week7_Free(StageBack *back)
{
	Back_Week7 *this = (Back_Week7*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Week7_New(void)
{
	//Allocate background structure
	Back_Week7 *this = (Back_Week7*)Mem_Alloc(sizeof(Back_Week7));
	if (this == NULL)
		return NULL;
	
	//Set background functions
	this->back.tick = NULL;
	this->back.draw_fg = NULL;
	this->back.draw_md = NULL;
	this->back.draw_bg = Back_Week7_DrawBG;
	this->back.free = Back_Week7_Free;

	//Load HUD textures
	Gfx_LoadTex(&stage.tex_hud0, IO_Read("\\STAGE\\HUD0.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud1, IO_Read("\\STAGE\\HUD1.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud2, IO_Read("\\STAGE\\HUD2.TIM;1"), GFX_LOADTEX_FREE);
	
	//Load background textures
	IO_Data arc_back = IO_Read("\\WEEK7\\BACK.ARC;1");
	Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
	Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
	Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
	Gfx_LoadTex(&this->tex_back3, Archive_Find(arc_back, "back3.tim"), 0);
	Mem_Free(arc_back);
	
	//Initialize tank state
	this->tank_x = TANK_END_X;
	this->tank_timer = RandomRange(TANK_TIME_A, TANK_TIME_B);
	
	//Use sky coloured background
	Gfx_SetClear(245, 202, 81);
	
	return (StageBack*)this;
}
