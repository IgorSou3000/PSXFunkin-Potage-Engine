/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"

#include "psx/archive.h"
#include "psx/mem.h"
#include "stage/stage.h"

//Stage background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
	
	//Textures
	Gfx_Tex tex_back0; //Stage and back
	Gfx_Tex tex_back1; //Curtains
} Back_Stage;

//Stage functions
void Back_Stage_Tick(StageBack *back)
{
	//Back_Stage *this = (Back_Stage*)back;
	(void)back; //Only for remove warning

	//Stage specific events
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		switch (stage.stage_id)
		{
			case StageId_Bopeebo:
				//BF peace
				if (stage.song_step >= 0 && (stage.song_step % 32) == 28)
					stage.player->set_anim(stage.player, CharAnim_Special1);
				break;
			case StageId_Tutorial:
				//BF and GF peace + cheer
				stage.gf_speed = 8;
				if (stage.song_step > 64 && stage.song_step < 192 && (stage.song_step % 64) == 60)
				{
					stage.player->set_anim(stage.player, CharAnim_Special1);
					stage.opponent->set_anim(stage.opponent, CharAnim_UpAlt);
				}
				break;
			default:
				break;
		}
	}
}

void Back_Stage_DrawBG(StageBack *back)
{
	Back_Stage *this = (Back_Stage*)back;
	
	fixed_t fx, fy, fscroll;

	//Draw curtains
	fscroll = FIXED_DEC(12,10);

	fx = FIXED_MUL(stage.camera.x, fscroll);
	fy = FIXED_MUL(stage.camera.y, fscroll);
	
	RECT curtainl_src = {0, 0, 107, 221};
	RECT_FIXED curtainl_dst = {
		FIXED_DEC(-250,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(107,1),
		FIXED_DEC(221,1)
	};
	RECT curtainr_src = {122, 0, 133, 255};
	RECT_FIXED curtainr_dst = {
		FIXED_DEC(110,1) - fx,
		FIXED_DEC(-150,1) - fy,
		FIXED_DEC(134,1),
		FIXED_DEC(255,1)
	};
	
	Stage_DrawTex(&this->tex_back1, &curtainl_src, &curtainl_dst, stage.camera.bzoom);
	Stage_DrawTex(&this->tex_back1, &curtainr_src, &curtainr_dst, stage.camera.bzoom);

	fx = stage.camera.x;
	fy = stage.camera.y;
	
	//Draw Stage
	RECT stage_src = {0, 0, 255, 59};
	RECT_FIXED stage_dst = {
		FIXED_DEC(-230,1) - fx,
		FIXED_DEC(50,1) - fy,
		FIXED_DEC(410,1),
		FIXED_DEC(123,1)
	};

	Debug_MoveTexture(&stage_dst, 1, "stage", 0, 0);
	
	Stage_DrawTex(&this->tex_back0, &stage_src, &stage_dst, stage.camera.bzoom);
	
	//Draw back
	fx = stage.camera.x / 2;
	fy = stage.camera.y / 2;
	
	RECT backl_src = {0, 59, 121, 105};
	RECT_FIXED backl_dst = {
		FIXED_DEC(-190,1) - fx,
		FIXED_DEC(-100,1) - fy,
		FIXED_DEC(121,1),
		FIXED_DEC(105,1)
	};

	Debug_MoveTexture(&backl_dst, 0, "back left", fx, fy);
	RECT backr_src = {120, 59, 135, 120};
	RECT_FIXED backr_dst = {
		FIXED_DEC(60,1) - fx,
		FIXED_DEC(-110,1) - fy,
		FIXED_DEC(136,1),
		FIXED_DEC(120,1)
	};
	RECT backf_src = {0, 59, 1, 1};
	RECT backf_dst = {
		0,
		0,
		SCREEN_WIDTH,
		SCREEN_HEIGHT,
	};
	
	Stage_DrawTex(&this->tex_back0, &backl_src, &backl_dst, stage.camera.bzoom);
	Stage_DrawTex(&this->tex_back0, &backr_src, &backr_dst, stage.camera.bzoom);
	Gfx_DrawTex(&this->tex_back0, &backf_src, &backf_dst);
}

void Back_Stage_Free(StageBack *back)
{
	Back_Stage *this = (Back_Stage*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Stage_New(void)
{
	//Allocate background structure
	Back_Stage *this = (Back_Stage*)Mem_Alloc(sizeof(Back_Stage));
	if (this == NULL)
		return NULL;
	
	//Set background functions
	this->back.tick = Back_Stage_Tick;
	this->back.draw_fg = NULL;
	this->back.draw_md = NULL;
	this->back.draw_bg = Back_Stage_DrawBG;
	this->back.free = Back_Stage_Free;
	
	//Load background textures
	IO_Data arc_back = IO_Read("\\STAGE\\BACK.ARC;1");
	Gfx_LoadTex(&this->tex_back0, Archive_Find(arc_back, "back0.tim"), 0);
	Gfx_LoadTex(&this->tex_back1, Archive_Find(arc_back, "back1.tim"), 0);
	Mem_Free(arc_back);
	
	return (StageBack*)this;
}
