/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "week5.h"

#include "psx/archive.h"
#include "psx/mem.h"

//Week 5 background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
	
	//Textures
	Gfx_Tex tex_back0; //Back wall
	Gfx_Tex tex_back1; //Second floor
	Gfx_Tex tex_back2; //Lower bop
	Gfx_Tex tex_back3; //Santa
	Gfx_Tex tex_back4; //Upper bop
	Gfx_Tex tex_back5; //Tree
} Back_Week5;

//Week 5 background functions
void Back_Week5_DrawBG(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	fixed_t fx, fy, fscroll;
	
	fixed_t beat_bop;
	if ((stage.song_step % 0x4) == 0)
		beat_bop = FIXED_UNIT - ((stage.note_scroll / 24) & FIXED_LAND);
	else
		beat_bop = 0;
	
	//Draw Santa
	
	//Draw snow
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT snow_src = {120, 155, 135, 100};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-350,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(570,1),
		FIXED_DEC(27,1)
	};
	
	Stage_DrawTex(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	snow_src.y = 255; 
	snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 3;
	Stage_DrawTex(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom);
	
	//Draw boppers
	static const struct Back_Week5_LowerBop
	{
		RECT src;
		RECT_FIXED dst;
	} lbop_piece[] = {
		{{0, 0, 80, 102}, {FIXED_DEC(-315,1), FIXED_DEC(-30,1), FIXED_DEC(80,1), FIXED_DEC(102,1)}},
		{{175, 3, 74, 150}, {FIXED_DEC(-120,1), FIXED_DEC(-80,1), FIXED_DEC(74,1), FIXED_DEC(151,1)}},
		{{81, 0, 70, 128}, {FIXED_DEC(30,1), FIXED_DEC(-70,1), FIXED_DEC(70,1), FIXED_DEC(128,1)}},
		{{151, 0, 23, 132}, {FIXED_DEC(100,1), FIXED_DEC(-70,1), FIXED_DEC(23,1), FIXED_DEC(132,1)}},
		{{0, 109, 41, 139}, {FIXED_DEC(123,1), FIXED_DEC(-69,1), FIXED_DEC(41,1), FIXED_DEC(139,1)}},
		{{41, 126, 69, 129}, {FIXED_DEC(164,1), FIXED_DEC(-52,1), FIXED_DEC(69,1), FIXED_DEC(130,1)}},
	};
	
	const struct Back_Week5_LowerBop *lbop_p = lbop_piece;
	for (size_t i = 0; i < COUNT_OF(lbop_piece); i++, lbop_p++)
	{
		RECT_FIXED lbop_dst = {
			lbop_p->dst.x - fx - (beat_bop * 2),
			lbop_p->dst.y - fy + (beat_bop * 8),
			lbop_p->dst.w + (beat_bop * 4),
			lbop_p->dst.h - (beat_bop * 8),
		};
		Stage_DrawTex(&this->tex_back2, &lbop_p->src, &lbop_dst, stage.camera.bzoom);
	}
	
	//Draw tree
	fscroll = FIXED_DEC(4,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT tree_src = {0, 0, 174, 210};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-86,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(174,1),
		FIXED_DEC(210,1)
	};
	
	Stage_DrawTex(&this->tex_back5, &tree_src, &tree_dst, stage.camera.bzoom);
	
	//Draw second floor
	fscroll = FIXED_DEC(3,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	static const struct Back_Week5_FloorPiece
	{
		RECT src;
		fixed_t scale;
	} floor_piece[] = {
		{{  0, 0, 161, 255}, FIXED_DEC(14,10)},
		{{161, 0,   9, 255}, FIXED_DEC(7,1)},
		{{171, 0,  84, 255}, FIXED_DEC(14,10)},
	};
	
	RECT_FIXED floor_dst = {
		FIXED_DEC(-220,1) - fx,
		FIXED_DEC(-115,1) - fy,
		0,
		FIXED_DEC(180,1)
	};
	
	const struct Back_Week5_FloorPiece *floor_p = floor_piece;
	for (size_t i = 0; i < COUNT_OF(floor_piece); i++, floor_p++)
	{
		floor_dst.w = floor_p->src.w ? (floor_p->src.w * floor_p->scale) : floor_p->scale;
		Stage_DrawTex(&this->tex_back1, &floor_p->src, &floor_dst, stage.camera.bzoom);
		floor_dst.x += floor_dst.w;
	}
	
	//Draw boppers
	static const struct Back_Week5_UpperBop
	{
		RECT src;
		RECT_FIXED dst;
	} ubop_piece[] = {
		{{0, 0, 255, 76}, {FIXED_DEC(-200,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}},
		{{0, 76, 255, 76}, {FIXED_DEC(50,1), FIXED_DEC(-132,1), FIXED_DEC(256,1)*6/7, FIXED_DEC(76,1)*6/7}}
	};
	
	const struct Back_Week5_UpperBop *ubop_p = ubop_piece;
	for (size_t i = 0; i < COUNT_OF(ubop_piece); i++, ubop_p++)
	{
		RECT_FIXED ubop_dst = {
			ubop_p->dst.x - fx,
			ubop_p->dst.y - fy + (beat_bop << 2),
			ubop_p->dst.w,
			ubop_p->dst.h - (beat_bop << 2),
		};
		Stage_DrawTex(&this->tex_back4, &ubop_p->src, &ubop_dst, stage.camera.bzoom);
	}
	
	//Draw back wall
	fscroll = FIXED_DEC(1,10);
	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	static const struct Back_Week5_WallPiece
	{
		RECT src;
		fixed_t scale;
	} wall_piece[] = {
		{{  0, 0, 113, 255}, FIXED_DEC(1,1)},
		{{113, 0,   6, 255}, FIXED_DEC(17,1)},
		{{119, 0, 137, 255}, FIXED_DEC(1,1)},
	};
	
	RECT_FIXED wall_dst = {
		FIXED_DEC(-180,1) - fx,
		FIXED_DEC(-130,1) - fy,
		0,
		FIXED_DEC(190,1)
	};
	
	RECT wall_src = {0, 255, 0, 0};
	RECT_FIXED wall_fill;
	wall_fill.x = wall_dst.x;
	wall_fill.y = wall_dst.y + wall_dst.h - FIXED_UNIT;
	wall_fill.w = FIXED_DEC(500,1);
	wall_fill.h = FIXED_DEC(100,1);
	Stage_DrawTex(&this->tex_back0, &wall_src, &wall_fill, stage.camera.bzoom);
	
	const struct Back_Week5_WallPiece *wall_p = wall_piece;
	for (size_t i = 0; i < COUNT_OF(wall_piece); i++, wall_p++)
	{
		wall_dst.w = wall_p->src.w ? (wall_p->src.w * wall_p->scale) : wall_p->scale;
		Stage_DrawTex(&this->tex_back0, &wall_p->src, &wall_dst, stage.camera.bzoom);
		wall_dst.x += wall_dst.w;
	}
}

//evil!!
void Back_Week5_DrawBGEvil(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	fixed_t fx, fy;
	
	//Draw snow but evil!!
	fx = stage.camera.x;
	fy = stage.camera.y;
	
	RECT snow_src = {120, 155, 135, 100};
	RECT_FIXED snow_dst = {
		FIXED_DEC(-350,1) - fx,
		FIXED_DEC(44,1) - fy,
		FIXED_DEC(570,1),
		FIXED_DEC(27,1)
	};
	
	Stage_DrawTexCol(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 210 >> 1, 115 >> 1, 177 >> 1);
	snow_src.y = 255; snow_src.h = 0;
	snow_dst.y += snow_dst.h - FIXED_UNIT;
	snow_dst.h *= 4;
	Stage_DrawTexCol(&this->tex_back2, &snow_src, &snow_dst, stage.camera.bzoom, 210 >> 1, 115 >> 1, 177 >> 1);
	
	//Draw tree but evil!!!
	fx = stage.camera.x >> 3;
	fy = stage.camera.y >> 3;
	
	RECT tree_src = {0, 0, 156, 197};
	RECT_FIXED tree_dst = {
		FIXED_DEC(-86,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(156,1),
		FIXED_DEC(197,1)
	};
	
	Stage_DrawTex(&this->tex_back5, &tree_src, &tree_dst, stage.camera.bzoom);
	
	//Draw back wall but evil!!
	
	static struct Back_Week5_WallPiece
	{
		RECT src;
		fixed_t scale;
	} wall_piece[] = {
		{{  0, 0, 113, 255}, FIXED_DEC(1,1)},
		{{113, 0,   6, 255}, FIXED_DEC(17,1)},
		{{119, 0, 136, 255}, FIXED_DEC(1,1)},
	};
	
	RECT_FIXED wall_dst = {
		FIXED_DEC(-180,1) - fx,
		FIXED_DEC(-130,1) - fy,
		0,
		FIXED_DEC(190,1)
	};
	
	RECT wall_src = {0, 255, 0, 0};
	RECT_FIXED wall_fill;
	wall_fill.x = wall_dst.x;
	wall_fill.y = wall_dst.y + wall_dst.h - FIXED_UNIT;
	wall_fill.w = FIXED_DEC(500,1);
	wall_fill.h = FIXED_DEC(100,1);
	Stage_DrawTex(&this->tex_back0, &wall_src, &wall_fill, stage.camera.bzoom);
	
	struct Back_Week5_WallPiece *wall_p = wall_piece;
	for (size_t i = 0; i < COUNT_OF(wall_piece); i++, wall_p++)
	{
		wall_dst.w = wall_p->src.w ? (wall_p->src.w * wall_p->scale) : wall_p->scale;
		Stage_DrawTex(&this->tex_back0, &wall_p->src, &wall_dst, stage.camera.bzoom);
		wall_dst.x += wall_dst.w;
	}
}

void Back_Week5_Free(StageBack *back)
{
	Back_Week5 *this = (Back_Week5*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Week5_New()
{
	//Allocate background structure
	Back_Week5 *this = (Back_Week5*)Mem_Alloc(sizeof(Back_Week5));
	if (this == NULL)
		return NULL;
	
	//Set background functions
	this->back.tick = NULL;
	this->back.draw_fg = NULL;
	this->back.draw_md = NULL;
	this->back.draw_bg = (stage.stage_id != StageId_5_3) ? Back_Week5_DrawBG : Back_Week5_DrawBGEvil; //evil!!;
	this->back.free = Back_Week5_Free;

	//Load HUD textures
	Gfx_LoadTex(&stage.tex_hud0, IO_Read("\\STAGE\\HUD0.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud1, IO_Read("\\STAGE\\HUD1.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud2, IO_Read("\\STAGE\\HUD2.TIM;1"), GFX_LOADTEX_FREE);
	
	//Load background textures
	if (stage.stage_id != StageId_5_3)
	{
		IO_Data arc_back = IO_Read("\\WEEK5\\BACK.ARC;1");
		Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
		Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
		Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
		Gfx_LoadTex(&this->tex_back3, Archive_Find(arc_back, "back3.tim"), 0);
		Gfx_LoadTex(&this->tex_back4, Archive_Find(arc_back, "back4.tim"), 0);
		Gfx_LoadTex(&this->tex_back5, Archive_Find(arc_back, "back5.tim"), 0);
		Mem_Free(arc_back);
	}
	//Load background textures but evil!!
	else
	{
		IO_Data arc_back = IO_Read("\\WEEK5\\EVIL.ARC;1");
		Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0e.tim"), 0);
		Gfx_LoadTex(&this->tex_back2, Archive_Find(arc_back, "back2.tim"), 0);
		Gfx_LoadTex(&this->tex_back5, Archive_Find(arc_back, "back5e.tim"), 0);
		Mem_Free(arc_back);
	}
	
	return (StageBack*)this;
}
