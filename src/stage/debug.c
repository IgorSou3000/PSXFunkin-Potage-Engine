/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "debug.h"
#include "psx/pad.h"
#include "stage.h"

#ifdef DEBUG_MODE
Debug debug;

void Debug_Load(void)
{
	stage.save.showtimer = false;
	stage.save.botplay = true;
	//Initializing this for avoid any memory issues
	debug.select = 0;
	debug.mode = debug.next_mode = 0;

	for (u8 i = 0; i < 10; i++)
	{
		debug.ogpositions[i].x = debug.positions[i].x = 0;
		debug.ogpositions[i].y = debug.positions[i].y = 0;
		debug.ogpositions[i].w = debug.positions[i].w = 0;
		debug.ogpositions[i].h = debug.positions[i].h = 0;
		sprintf(debug.tex_names[i], "none");
	}
}

void Debug_MoveTexture(RECT_FIXED* src, u8 select, const char* name, fixed_t camera_x, fixed_t camera_y)
{
	src->x += debug.positions[select].x;
	src->y += debug.positions[select].y;
	src->w += debug.positions[select].w;
	src->h += debug.positions[select].h;
	sprintf(debug.tex_names[select], name);

	debug.ogpositions[select].x = src->x + camera_x;
	debug.ogpositions[select].y = src->y + camera_y;
	debug.ogpositions[select].w = src->w;
	debug.ogpositions[select].h = src->h;
}

void Debug_Tick(void)
{
	if (debug.mode != debug.next_mode)
	{
		debug.mode = debug.next_mode;
		debug.select = 0;
	}

	char name_text[25];
	char mode_text[25];
	char texture_text[100];

	Character* chars[3] = {
		stage.player,
		stage.opponent,
		stage.gf
	};

	static const char* mode_options[] = {
		"TEXTURE MODE",
		"FREECAMERA MODE",
		"CHARACTER MODE"
	};

	sprintf(mode_text, "%s", mode_options[debug.mode]);
	stage.font_cdr.draw(&stage.font_cdr,
			mode_text,
			30,
			-112,
			FontAlign_Left
		);

	//Switch select
	if (pad_state.press & PAD_L1)
	{
		if (debug.select > 0)
				debug.select--;
		else
				debug.select = 9;
	}

	if (pad_state.press & PAD_R1)
	{
		if (debug.select < 9)
			debug.select++;
		else
			debug.select = 0;
	}

	//Switch mode
	if (pad_state.press & PAD_L2)
	{
		if (debug.next_mode > 0)
				debug.next_mode--;
		else
				debug.next_mode = COUNT_OF(mode_options) - 1;
	}

	if (pad_state.press & PAD_R2)
	{
		if (debug.next_mode < COUNT_OF(mode_options) - 1)
			debug.next_mode++;
		else
			debug.next_mode = 0;
	}
	
	switch (debug.mode)
	{
		case 0: //Texture mode
		{
			sprintf(name_text, "%d - %s", debug.select, debug.tex_names[debug.select]);
			stage.font_cdr.draw(&stage.font_cdr,
				name_text,
				-140,
				-112,
				FontAlign_Left
			);

			sprintf(texture_text, "X: %d, Y: %d, W: %d, H: %d", 
			debug.ogpositions[debug.select].x >> FIXED_SHIFT,
			debug.ogpositions[debug.select].y >> FIXED_SHIFT,
			debug.ogpositions[debug.select].w >> FIXED_SHIFT,
			debug.ogpositions[debug.select].h >> FIXED_SHIFT
			);

			stage.font_cdr.draw(&stage.font_cdr,
				texture_text,
				-160,
				-52,
				FontAlign_Left
			);

			if (pad_state.held & PAD_LEFT)
				debug.positions[debug.select].x -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_RIGHT)
				debug.positions[debug.select].x += FIXED_DEC(1,1);
			if (pad_state.held & PAD_UP)
				debug.positions[debug.select].y -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_DOWN)
				debug.positions[debug.select].y += FIXED_DEC(1,1);

			//Strench texture
			if (pad_state.held & PAD_SQUARE)
				debug.positions[debug.select].w -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_CIRCLE)
				debug.positions[debug.select].w += FIXED_DEC(1,1);

			if (pad_state.held & PAD_TRIANGLE)
				debug.positions[debug.select].h -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_CROSS)
				debug.positions[debug.select].h += FIXED_DEC(1,1);
			break;
		}
		case 1: //Freecam
		{
			sprintf(texture_text, "X: %d, Y: %d, Zoom: %d", 
			stage.camera.x >> FIXED_SHIFT,
			stage.camera.y >> FIXED_SHIFT,
			stage.camera.zoom
			);

			stage.font_cdr.draw(&stage.font_cdr,
				texture_text,
				-160,
				-52,
				FontAlign_Left
			);

			if (pad_state.held & PAD_LEFT)
				stage.camera.x -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_UP)
				stage.camera.y -= FIXED_DEC(1,1);
			if (pad_state.held & PAD_RIGHT)
				stage.camera.x += FIXED_DEC(1,1);
			if (pad_state.held & PAD_DOWN)
				stage.camera.y += FIXED_DEC(1,1);
			if (pad_state.held & PAD_TRIANGLE)
				stage.camera.zoom -= FIXED_DEC(1,100);
			if (pad_state.held & PAD_CROSS)
				stage.camera.zoom += FIXED_DEC(1,100);
		break;
		}

		case 2: //Character mode
		{
			sprintf(name_text, "%d - Character%d", debug.select, debug.select);
			stage.font_cdr.draw(&stage.font_cdr,
				name_text,
				-140,
				-112,
				FontAlign_Left
			);

			if (debug.select < COUNT_OF(chars))
			{
				sprintf(texture_text, "X: %d, Y: %d", 
				chars[debug.select]->x >> FIXED_SHIFT,
				chars[debug.select]->y >> FIXED_SHIFT
				);

				stage.font_cdr.draw(&stage.font_cdr,
					texture_text,
					-160,
					-52,
					FontAlign_Left
				);

				if (pad_state.held & PAD_LEFT)
					chars[debug.select]->x -= FIXED_DEC(1,1);
				if (pad_state.held & PAD_RIGHT)
					chars[debug.select]->x += FIXED_DEC(1,1);
				if (pad_state.held & PAD_UP)
					chars[debug.select]->y -= FIXED_DEC(1,1);
				if (pad_state.held & PAD_DOWN)
					chars[debug.select]->y += FIXED_DEC(1,1);
			}
			break;
		}
	}
}

#else
void Debug_Load(void)
{}
void Debug_MoveTexture(RECT_FIXED* src, u8 select, const char* name, fixed_t camera_x, fixed_t camera_y)
{
	(void)src;
	(void)select;
	(void)name;
	(void)camera_x;
	(void)camera_y;
}
void Debug_Tick(void)
{}

#endif