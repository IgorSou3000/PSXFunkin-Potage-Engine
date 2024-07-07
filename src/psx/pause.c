/*
	This Source Code Form is subject to the terms of the Mozilla Public
	License, v. 2.0. If a copy of the MPL was not distributed with this
	file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "pause.h"

#include "stage.h"
#include "psx/trans.h"
#include "menu/menu.h"

Pause pause;

void Pause_Tick()
{
	static const char *pause_options[] = {
		"RESUME",
		"RESTART SONG",
		"EXIT TO MENU"
	};

	//Select option if cross or start is pressed
	if (pad_state.press & (PAD_CROSS | PAD_START))
	{
		pause.is_paused = false;
		switch (pause.select)
		{
			case 0: //Resume
				stage.flag &= ~STAGE_FLAG_PAUSED;
				Audio_ResumeXA();
				break;
			case 1: //Retry
				stage.trans = StageTrans_Reload;
				Trans_Start();
				break;
			case 2: //Quit
				stage.trans = StageTrans_Menu;
				Trans_Start();
				break;
		}
	}

	if (pause.scroll == -1)
		pause.scroll = COUNT_OF(pause_options) * FIXED_DEC(32,1);

	//Change option
	pause.select = Menu_Scroll(pause.select, COUNT_OF(pause_options) - 1, &stage.sounds[0]);

	//Draw options
	s32 next_scroll = pause.select * FIXED_DEC(32,1);
	pause.scroll += Lerp(pause.scroll, next_scroll, FIXED_DEC(1,1) >> 3);

	for (u8 i = 0; i < COUNT_OF(pause_options); i++)
	{
		//Get position on screen
		s32 y = (i * 32) - 8 - (pause.scroll >> FIXED_SHIFT);
		if (y <= -SCREEN_HEIGHT2 - 8)
			continue;
		if (y >= SCREEN_HEIGHT2 + 8)
			break;
				
		//Draw text
		stage.font_bold.draw_col(&stage.font_bold,
		pause_options[i],
		20 + y / 4,
		y + SCREEN_HEIGHT2,
		FontAlign_Left,
		//if the option is the one you are selecting, draw in normal color, else, draw gray
		(i == pause.select) ? 0x80 : 0x40,
		(i == pause.select) ? 0x80 : 0x40,
		(i == pause.select) ? 0x80 : 0x40
		);
	}

	//50% Blend
	RECT screen_src = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

	Gfx_BlendRect(&screen_src, 0, 0, 0, 0);
}