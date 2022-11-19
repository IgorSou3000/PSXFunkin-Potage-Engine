/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "dummy.h"

#include "psx/mem.h"

//Dummy background structure
typedef struct
{
	//Stage background base structure
	StageBack back;
} Back_Dummy;

//Dummy background functions
void Back_Dummy_Free(StageBack *back)
{
	Back_Dummy *this = (Back_Dummy*)back;
	
	//Free structure
	Mem_Free(this);
}

StageBack *Back_Dummy_New(void)
{
	//Allocate background structure
	Back_Dummy *this = (Back_Dummy*)Mem_Alloc(sizeof(Back_Dummy));
	if (this == NULL)
		return NULL;

	//Load HUD textures
	Gfx_LoadTex(&stage.tex_hud0, IO_Read("\\STAGE\\HUD0.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud1, IO_Read("\\STAGE\\HUD1.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud2, IO_Read("\\STAGE\\HUD2.TIM;1"), GFX_LOADTEX_FREE);
	
	//Set background functions
	this->back.tick = NULL;
	this->back.draw_fg = NULL;
	this->back.draw_md = NULL;
	this->back.draw_bg = NULL;
	this->back.free = Back_Dummy_Free;
	
	//Use non-pitch black background
	Gfx_SetClear(62, 48, 64);
	
	return (StageBack*)this;
}
