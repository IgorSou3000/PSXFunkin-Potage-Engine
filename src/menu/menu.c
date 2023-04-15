/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "menu.h"

#include "psx/mem.h"
#include "psx/main.h"
#include "psx/timer.h"
#include "psx/io.h"
#include "psx/gfx.h"
#include "psx/audio.h"
#include "psx/pad.h"
#include "psx/archive.h"
#include "psx/mutil.h"
#include "psx/save.h"

#include "psx/font.h"
#include "psx/trans.h"
#include "psx/loadscr.h"

#include "stage/stage.h"
#include "characters/gf.h"

//Menu messages
static const char *funny_messages[][2] = {
	{"PSX PORT BY CUCKYDEV", "YOU KNOW IT"},
	{"PORTED BY CUCKYDEV", "WHAT YOU GONNA DO"},
	{"FUNKIN", "FOREVER"},
	{"WHAT THE HELL", "RITZ PSX"},
	{"LIKE PARAPPA", "BUT COOLER"},
	{"THE JAPI", "EL JAPI"},
	{"PICO FUNNY", "PICO FUNNY"},
	{"OPENGL BACKEND", "BY CLOWNACY"},
	{"CUCKYFNF", "SETTING STANDARDS"},
	{"lool", "inverted colours"},
	{"NEVER LOOK AT", "THE ISSUE TRACKER"},
	{"PSXDEV", "HOMEBREW"},
	{"ZERO POINT ZERO TWO TWO EIGHT", "ONE FIVE NINE ONE ZERO FIVE"},
	{"DOPE ASS GAME", "PLAYSTATION MAGAZINE"},
	{"NEWGROUNDS", "FOREVER"},
	{"NO FPU", "NO PROBLEM"},
	{"OK OKAY", "WATCH THIS"},
	{"ITS MORE MALICIOUS", "THAN ANYTHING"},
	{"USE A CONTROLLER", "LOL"},
	{"SNIPING THE KICKSTARTER", "HAHA"},
	{"SHITS UNOFFICIAL", "NOT A PROBLEM"},
	{"SYSCLK", "RANDOM SEED"},
	{"THEY DIDNT HIT THE GOAL", "STOP"},
	{"FCEFUWEFUETWHCFUEZDSLVNSP", "PQRYQWENQWKBVZLZSLDNSVPBM"},
	{"THE FLOORS ARE", "THREE DIMENSIONAL"},
	{"PSXFUNKIN BY CUCKYDEV", "SUCK IT DOWN"},
	{"PLAYING ON EPSXE HUH", "YOURE THE PROBLEM"},
	{"NEXT IN LINE", "ATARI"},
	{"HAXEFLIXEL", "COME ON"},
	{"HAHAHA", "I DONT CARE"},
	{"GET ME TO STOP", "TRY"},
	{"FNF MUKBANG GIF", "THATS UNRULY"},
	{"OPEN SOURCE", "FOREVER"},
	{"ITS A PORT", "ITS WORSE"},
	{"WOW GATO", "WOW GATO"},
	{"BALLS FISH", "BALLS FISH"},
};

//Menu state
static struct
{
	//Menu state
	u8 page, next_page;
	boolean page_swap;
	u8 select, next_select;
	
	fixed_t scroll;
	fixed_t trans_time;

	SFX sounds[10];
	
	//Page specific state
	union
	{
		struct
		{
			u8 funny_message;
		} opening;
		struct
		{
			fixed_t logo_bump;
			fixed_t fade, fadespd;
		} title;
		struct
		{
			fixed_t fade, fadespd;
		} story;
		struct
		{
			fixed_t back_r, back_g, back_b;
		} freeplay;
	} page_state;
	
	union
	{
		struct
		{
			u8 id, diff;
			boolean story;
		} stage;
	} page_param;
	
	//Menu assets
	Gfx_Tex tex_back, tex_ng, tex_story, tex_title;
	FontData font_bold, font_arial, font_cdr;
	
	Character *gf; //Title Girlfriend
} menu;

//Internal menu functions
char menu_text_buffer[0x100];

static const char *Menu_LowerIf(const char *text, boolean lower)
{
	//Copy text
	char *dstp = menu_text_buffer;
	if (lower)
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'A' && *srcp <= 'Z')
				*dstp++ = *srcp | 0x20;
			else
				*dstp++ = *srcp;
		}
	}
	else
	{
		for (const char *srcp = text; *srcp != '\0'; srcp++)
		{
			if (*srcp >= 'a' && *srcp <= 'z')
				*dstp++ = *srcp & ~0x20;
			else
				*dstp++ = *srcp;
		}
	}
	
	//Terminate text
	*dstp++ = '\0';
	return menu_text_buffer;
}

//Since the psxfunkin scroll code is basically the same in every mode, i making this be a function
u8 Menu_Scroll(u8 select, u8 optionsn, SFX* scroll_sfx)
{
	//Change options
	if (pad_state.press & PAD_UP)
	{
		//Play Scroll Sound
		Audio_PlaySFX(*scroll_sfx, 80);
		if (select > 0)
				select--;
		else
				select = optionsn;
	}

	if (pad_state.press & PAD_DOWN)
	{
		//Play Scroll Sound
		Audio_PlaySFX(*scroll_sfx, 80);
		if (select < optionsn)
			select++;
		else
			select = 0;
	}
	return select;
}

static void Menu_DrawBack(boolean flash, s32 scroll, u8 r0, u8 g0, u8 b0, u8 r1, u8 g1, u8 b1)
{
	RECT back_src = {0, 0, 255, 255};
	RECT back_dst = {0, -scroll - SCREEN_WIDEADD2, SCREEN_WIDTH, SCREEN_WIDTH * 4 / 5};
	
	if (flash || (animf_count & 4) == 0)
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r0, g0, b0);
	else
		Gfx_DrawTexCol(&menu.tex_back, &back_src, &back_dst, r1, g1, b1);
}

static void Menu_DifficultySelector(s32 x, s32 y)
{
	//Change difficulty
	if (menu.next_page == menu.page && Trans_Idle())
	{
		if (pad_state.press & PAD_LEFT)
		{
			if (menu.page_param.stage.diff > StageDiff_Easy)
				menu.page_param.stage.diff--;
			else
				menu.page_param.stage.diff = StageDiff_Hard;
		}
		if (pad_state.press & PAD_RIGHT)
		{
			if (menu.page_param.stage.diff < StageDiff_Hard)
				menu.page_param.stage.diff++;
			else
				menu.page_param.stage.diff = StageDiff_Easy;
		}
	}
	
	//Draw difficulty arrows
	static const RECT arrow_src[2][2] = {
		{{223, 64, 16, 32}, {223, 96, 16, 32}}, //left
		{{239, 64, 16, 32}, {239, 96, 16, 32}}, //right
	};

	RECT arrow_dst[2] = {
		{x - 40 - 16, y - 16, 16, 32}, //left
		{x + 40, y - 16, 16, 32}, //right
	};

	//Check if pad left or right has pressed
	u8 left_pressed = ((pad_state.held & PAD_LEFT) != 0);
	u8 right_pressed = ((pad_state.held & PAD_RIGHT) != 0);
	
	Gfx_DrawTex(&menu.tex_story, &arrow_src[0][left_pressed], &arrow_dst[0]);
	Gfx_DrawTex(&menu.tex_story, &arrow_src[1][right_pressed], &arrow_dst[1]);
	
	//Draw difficulty
	static const RECT diff_srcs[] = {
		{  0, 96, 64, 18},
		{ 64, 96, 80, 18},
		{144, 96, 64, 18},
	};
	
	const RECT *diff_src = &diff_srcs[menu.page_param.stage.diff];

	RECT diff_dst = {
		x - (diff_src->w / 2),
		y - 9 + ((pad_state.press & (PAD_LEFT | PAD_RIGHT)) != 0),
		diff_src->w,
		diff_src->h,
	};

	Gfx_DrawTex(&menu.tex_story, diff_src, &diff_dst);
}

static void Menu_DrawWeek(const char *week, s32 x, s32 y, boolean flash)
{
	u8 r, g, b;

	//If flash be true, make the images be cyan
	if (flash)
	{
		r = 45;
		g = 235;
		b = 235;
	}
	else
	{
		r = 128;
		g = 128;
		b = 128;
	}

	//Draw label
	if (week == NULL)
	{
		//Tutorial
		RECT label_src = {0, 0, 112, 32};
		RECT label_dst = {x, y, 112, 32};
		Gfx_DrawTexCol(&menu.tex_story, &label_src, &label_dst, r, g, b);
	}
	else
	{
		//Week
		RECT label_src = {0, 32, 80, 32};
		RECT label_dst = {x, y,  80, 32};

		Gfx_DrawTexCol(&menu.tex_story, &label_src, &label_dst, r, g, b);
		
		//Number
		x += 80;
		for (; *week != '\0'; week++)
		{
			//Draw number
			u8 i = *week - '0';
			
			RECT num_src = {127 + ((i & 3) * 32), ((i / 4) * 32), 32, 32};
			RECT num_dst = {x, y, 32, 32};

			Gfx_DrawTexCol(&menu.tex_story, &num_src, &num_dst, r, g, b);
			x += 32;
		}
	}
}

static void Menu_DrawHealth(u8 i, s16 x, s16 y)
{
	//Icon Size
	u8 icon_size = 38;

	//Get src and dst
	RECT src = {
		(i % 6) * icon_size,
		(i / 6) * icon_size,
		icon_size,
		icon_size
	};
	RECT dst = {
		x,
		y,
		38,
		38
	};
	
	//Draw health icon
	Gfx_DrawTex(&stage.tex_hud1, &src, &dst);
}

//Menu functions
void Menu_Load(MenuPage page)
{
	//Initialize with a special stage id for not trigger any stage events (like gf cheer)
	stage.stage_id = StageId_Max;

	//Load menu assets
	IO_Data menu_arc = IO_Read("\\MENU\\MENU.ARC;1");
	Gfx_LoadTex(&menu.tex_back,  Archive_Find(menu_arc, "back.tim"),  0);
	Gfx_LoadTex(&menu.tex_ng,    Archive_Find(menu_arc, "ng.tim"),    0);
	Gfx_LoadTex(&menu.tex_story, Archive_Find(menu_arc, "story.tim"), 0);
	Gfx_LoadTex(&menu.tex_title, Archive_Find(menu_arc, "title.tim"), 0);
	Gfx_LoadTex(&stage.tex_hud1, Archive_Find(menu_arc, "hud1.tim"), 0);
	Mem_Free(menu_arc);
	
	FontData_Load(&menu.font_bold, Font_Bold, false);
	FontData_Load(&menu.font_arial, Font_Arial, false);
	FontData_Load(&menu.font_cdr, Font_CDR, false);
	
	menu.gf = Char_GF_New(FIXED_DEC(62,1), FIXED_DEC(-12,1));
	stage.camera.x = stage.camera.y = FIXED_DEC(0,1);
	stage.camera.bzoom = FIXED_UNIT;
	stage.gf_speed = 4;
	
	//Initialize menu state
	menu.select = menu.next_select = 0;
	
	switch (menu.page = menu.next_page = page)
	{
		case MenuPage_Opening:
			//Get funny message to use
				menu.page_state.opening.funny_message = ((*((volatile u32*)0xBF801120)) / 8) % COUNT_OF(funny_messages); //sysclk seeding
			break;
		default:
			break;
	}
	
	menu.page_swap = true;
	
	menu.trans_time = 0;
	Trans_Clear();
	
	stage.song_step = 0;

	//Clear Alloc for sound effect work (otherwise the sounds will not work)
	Audio_ClearAlloc();

	//Sound Effects Path
	const char* sfx_path[] = {
		"\\SOUNDS\\SCROLL.VAG;1",
		"\\SOUNDS\\CONFIRM.VAG;1",
		"\\SOUNDS\\CANCEL.VAG;1"
	};

	//Load sound effects
	for (u8 i = 0; i < COUNT_OF(sfx_path); i++)
	{
		menu.sounds[i] = Audio_LoadSFX(sfx_path[i]);
	}
	
	//Play menu music
	Audio_PlayXA_Track(XA_GettinFreaky, 0x40, 0, true);
	
	//Set background colour
	Gfx_SetClear(0, 0, 0);
}

void Menu_Unload(void)
{
	//Free title Girlfriend
	Character_Free(menu.gf);
}

void Menu_ToStage(StageId id, StageDiff diff, boolean story)
{
	menu.next_page = MenuPage_Stage;
	menu.page_param.stage.id = id;
	menu.page_param.stage.story = story;
	menu.page_param.stage.diff = diff;
	Trans_Start();
}

void Menu_Tick(void)
{
	//Clear per-frame flags
	stage.flag &= ~STAGE_FLAG_JUST_STEP;
	
	//Get song position
	u16 bpm = 102; // Freaky BPM(102)

	//Weird formula that make the xa milliseconds be a bpm
	u16 next_step = Audio_TellXA_Milli() / (256 * 60 / bpm);

	if (next_step != stage.song_step)
	{
		if (next_step >= stage.song_step)
			stage.flag |= STAGE_FLAG_JUST_STEP;
		stage.song_step = next_step;
		stage.song_beat = stage.song_step / 4;
	}
	
	//Handle transition out
	if (Trans_Tick())
	{
		//Change to set next page
		menu.page_swap = true;
		menu.page = menu.next_page;
		menu.select = menu.next_select;
	}
	
	//Tick menu page
	MenuPage exec_page;
	switch (exec_page = menu.page)
	{
		case MenuPage_Opening:
		{	
			//Start title screen if opening ended
			if (stage.song_beat >= 16)
			{
				menu.page = menu.next_page = MenuPage_Title;
				menu.page_swap = true;
				//Fallthrough
			}
			else
			{
				//Start title screen if start pressed
				if (pad_state.held & PAD_START)
					menu.page = menu.next_page = MenuPage_Title;
				
				//Draw different text depending on beat
				RECT ng_src = {0, 0, 128, 128};
				RECT ng_dst = {(SCREEN_WIDTH - 128) / 2, SCREEN_HEIGHT2 - 16, 128, 128};
				const char **funny_message = funny_messages[menu.page_state.opening.funny_message];
				
				switch (stage.song_beat)
				{
					case 3:
						menu.font_bold.draw(&menu.font_bold, "PRESENT", SCREEN_WIDTH2, SCREEN_HEIGHT2 + 32, FontAlign_Center);
				//Fallthrough
					case 2:
					case 1:
						menu.font_bold.draw(&menu.font_bold, "NINJAMUFFIN",   SCREEN_WIDTH2, SCREEN_HEIGHT2 - 32, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "PHANTOMARCADE", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 16, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "KAWAISPRITE",   SCREEN_WIDTH2, SCREEN_HEIGHT2,      FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "EVILSKER",      SCREEN_WIDTH2, SCREEN_HEIGHT2 + 16, FontAlign_Center);
						break;
					
					case 7:
						menu.font_bold.draw(&menu.font_bold, "NEWGROUNDS",    SCREEN_WIDTH2, SCREEN_HEIGHT2 - 32, FontAlign_Center);
						Gfx_DrawTex(&menu.tex_ng, &ng_src, &ng_dst);
				//Fallthrough
					case 6:
					case 5:
						menu.font_bold.draw(&menu.font_bold, "IN ASSOCIATION", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 64, FontAlign_Center);
						menu.font_bold.draw(&menu.font_bold, "WITH",           SCREEN_WIDTH2, SCREEN_HEIGHT2 - 48, FontAlign_Center);
						break;
					
					case 11:
						menu.font_bold.draw(&menu.font_bold, funny_message[1], SCREEN_WIDTH2, SCREEN_HEIGHT2, FontAlign_Center);
				//Fallthrough
					case 10:
					case 9:
						menu.font_bold.draw(&menu.font_bold, funny_message[0], SCREEN_WIDTH2, SCREEN_HEIGHT2 - 16, FontAlign_Center);
						break;
					
					case 15:
						menu.font_bold.draw(&menu.font_bold, "FUNKIN", SCREEN_WIDTH2, SCREEN_HEIGHT2 + 8, FontAlign_Center);
				//Fallthrough
					case 14:
						menu.font_bold.draw(&menu.font_bold, "NIGHT", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 8, FontAlign_Center);
				//Fallthrough
					case 13:
						menu.font_bold.draw(&menu.font_bold, "FRIDAY", SCREEN_WIDTH2, SCREEN_HEIGHT2 - 24, FontAlign_Center);
						break;
				}
				break;
			}
		}
	//Fallthrough
		case MenuPage_Title:
		{
			//Initialize page
			if (menu.page_swap)
			{
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(90,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade / FIXED_UNIT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Go to main menu when start is pressed
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if ((pad_state.press & PAD_START) && menu.next_page == menu.page && Trans_Idle())
			{
				//Play Confirm Sound
				Audio_PlaySFX(menu.sounds[1], 80);
				menu.trans_time = FIXED_UNIT;
				menu.page_state.title.fade = FIXED_DEC(255,1);
				menu.page_state.title.fadespd = FIXED_DEC(300,1);
				menu.next_page = MenuPage_Main;
				menu.next_select = 0;
			}
			
			//Draw Friday Night Funkin' logo
			if ((stage.flag & STAGE_FLAG_JUST_STEP) && (stage.song_step & 0x3) == 0 && menu.page_state.title.logo_bump == 0)
				menu.page_state.title.logo_bump = (FIXED_DEC(7,1) / 24) - 1;
			
			static const fixed_t logo_scales[] = {
				FIXED_DEC(1,1),
				FIXED_DEC(101,100),
				FIXED_DEC(102,100),
				FIXED_DEC(103,100),
				FIXED_DEC(105,100),
				FIXED_DEC(110,100),
				FIXED_DEC(97,100),
			};
			fixed_t logo_scale = logo_scales[(menu.page_state.title.logo_bump * 24) / FIXED_UNIT];
			u32 x_rad = (logo_scale * (176 / 2)) / FIXED_UNIT;
			u32 y_rad = (logo_scale * (112 / 2)) / FIXED_UNIT;
			
			RECT logo_src = {0, 0, 176, 112};
			RECT logo_dst = {
				100 - x_rad + (SCREEN_WIDEADD2 / 2),
				68 - y_rad,
				x_rad * 2,
				y_rad * 2
			};
			Gfx_DrawTex(&menu.tex_title, &logo_src, &logo_dst);
			
			if (menu.page_state.title.logo_bump > 0)
				if ((menu.page_state.title.logo_bump -= timer_dt) < 0)
					menu.page_state.title.logo_bump = 0;
			
			//Draw "Press Start to Begin"
			if (menu.next_page == menu.page)
			{
				//Blinking blue
				s16 press_lerp = (MUtil_Cos(animf_count * 8) + 256) / 2;
				u8 press_r = 51 / 2;
				u8 press_g = (58  + ((press_lerp * (255 - 58))  / 256)) / 2;
				u8 press_b = (206 + ((press_lerp * (255 - 206)) / 256)) / 2;
				
				RECT press_src = {0, 203, 207, 18};
				RECT press_dst = {50, SCREEN_HEIGHT - 35, 207, 18};
				Gfx_DrawTexCol(&menu.tex_title, &press_src, &press_dst, press_r, press_g, press_b);
			}
			else
			{
				//Flash white
				RECT press_src = {0, (animf_count & 1) ? 203 : 221, 207, 18};
				RECT press_dst = {50, SCREEN_HEIGHT - 35, 207, 18};
				Gfx_DrawTex(&menu.tex_title, &press_src, &press_dst);
			}
			
			//Draw Girlfriend
			menu.gf->tick(menu.gf);
			break;
		}
		case MenuPage_Main:
		{
			static const char *menu_options[] = {
				"STORY MODE",
				"FREEPLAY",
				"CREDITS",
				"OPTIONS",
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = menu.select * FIXED_DEC(8,1);
			
			//Draw version identification
			menu.font_bold.draw(&menu.font_bold,
				"PSXFUNKIN BY CUCKYDEV",
				16,
				SCREEN_HEIGHT - 32,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					switch (menu.select)
					{
						case 0: //Story Mode
							menu.next_page = MenuPage_Story;
							break;
						case 1: //Freeplay
							menu.next_page = MenuPage_Freeplay;
							break;
						case 2: //Credits
							menu.next_page = MenuPage_Credits;
							break;
						case 3: //Options
							menu.next_page = MenuPage_Options;
							break;
					}
					//Play Confirm Sound
					Audio_PlaySFX(menu.sounds[1], 80);

					menu.next_select = 0;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to title screen if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);
					menu.next_page = MenuPage_Title;
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(8,1);

			menu.scroll += Lerp(menu.scroll, next_scroll, FIXED_DEC(1,1) / 4);
			
			if (menu.next_page == menu.page || menu.next_page == MenuPage_Title)
			{
				//Draw all options
				for (u8 i = 0; i < COUNT_OF(menu_options); i++)
				{
					menu.font_bold.draw(&menu.font_bold,
						Menu_LowerIf(menu_options[i], menu.select != i),
						SCREEN_WIDTH2,
						SCREEN_HEIGHT2 + (i * 32) - 48 - (menu.scroll / FIXED_UNIT),
						FontAlign_Center
					);
				}
			}
			else if (animf_count & 2)
			{
				//Draw selected option
				menu.font_bold.draw(&menu.font_bold,
					menu_options[menu.select],
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + (menu.select * 32) - 48 - (menu.scroll / FIXED_UNIT),
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				menu.next_page == menu.page || menu.next_page == MenuPage_Title, 
				menu.scroll / (FIXED_UNIT * 4),
				253 / 2, 231 / 2, 113 / 2,
				253 / 2, 113 / 2, 155 / 2
			);
			break;
		}
		case MenuPage_Story:
		{
			static const struct
			{
				const char *week;
				StageId stage;
				const char *name;
				const char *tracks[3];
			} menu_options[] = {
				{NULL, StageId_Tutorial, "TUTORIAL", {"TUTORIAL", NULL, NULL}},
				{"1", StageId_DadBattle, "DADDY DEAREST", {"BOPEEBO", "FRESH", "DADBATTLE"}},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = 0;
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.title.fade = FIXED_DEC(0,1);
				menu.page_state.title.fadespd = FIXED_DEC(0,1);
			}
			
			//Draw white fade
			if (menu.page_state.title.fade > 0)
			{
				static const RECT flash = {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};
				u8 flash_col = menu.page_state.title.fade / FIXED_UNIT;
				Gfx_BlendRect(&flash, flash_col, flash_col, flash_col, 1);
				menu.page_state.title.fade -= FIXED_MUL(menu.page_state.title.fadespd, timer_dt);
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 52, SCREEN_HEIGHT2 + 19);
			
			//Handle option and selection
			if (menu.trans_time > 0 && (menu.trans_time -= timer_dt) <= 0)
				Trans_Start();
			
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					//Play Confirm Sound
					Audio_PlaySFX(menu.sounds[1], 80);

					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = true;
					menu.trans_time = FIXED_UNIT;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 0; //Story Mode
					Trans_Start();
				}
			}
			
			//Draw week name and tracks
			menu.font_arial.draw(&menu.font_arial,
				menu_options[menu.select].name,
				SCREEN_WIDTH - 12,
				10,
				FontAlign_Right
			);
			
			const char * const *trackp = menu_options[menu.select].tracks;
			for (size_t i = 0; i < COUNT_OF(menu_options[menu.select].tracks); i++, trackp++)
			{
				if (*trackp != NULL)
					menu.font_arial.draw_col(&menu.font_arial,
						*trackp,
						40,
						SCREEN_WIDTH2 + (i * 12),
						FontAlign_Center,
						229 >> 1,
						87 >> 1,
						119 >> 1
					);
			}
			
			//Draw upper strip
			RECT name_bar = {0, 22, SCREEN_WIDTH, 97};
			Gfx_DrawRect(&name_bar, 249, 207, 81);

			//Draw "tracks"
			RECT track_src = {74, 70, 58, 10};
			RECT track_dst = {
				15,
				SCREEN_WIDTH2 - 15,
				58,
				10
			};
			Gfx_DrawTex(&menu.tex_story, &track_src, &track_dst);
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(48,1);
			menu.scroll += Lerp(menu.scroll,next_scroll, FIXED_DEC(1,1) / 8);
			
			//Draw all options
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				s32 y = 132 + (i * 48) - (menu.scroll / FIXED_UNIT);
				if (y <= 16)
					continue;
				if (y >= SCREEN_HEIGHT)
					break;

				//Draw the selected option with a cool effect
				if (menu.next_page == MenuPage_Stage && animf_count & 2)
					Menu_DrawWeek(menu_options[menu.select].week, SCREEN_WIDTH2 - 64, 132 + (menu.select * 48) - (menu.scroll / FIXED_UNIT), true);

				//Draw all options
				Menu_DrawWeek(menu_options[i].week, SCREEN_WIDTH2 - 64, y, false);
		}
			break;
	}
		case MenuPage_Freeplay:
		{
			static const struct
			{
				StageId stage; //The stage
				u32 col; //Background color (use hex)
				const char *text; //The text of the song
				u8 icon; //The character icon
			} menu_options[] = {
				{StageId_Tutorial, 0xFF9271FD, "TUTORIAL", 2},
				{StageId_Bopeebo, 0xFF9271FD, "BOPEEBO", 1},
				{StageId_Fresh, 0xFF9271FD, "FRESH", 1},
				{StageId_DadBattle, 0xFF9271FD, "DADBATTLE", 1},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(30 + SCREEN_HEIGHT2,1);
				menu.page_param.stage.diff = StageDiff_Normal;
				menu.page_state.freeplay.back_r = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_g = FIXED_DEC(255,1);
				menu.page_state.freeplay.back_b = FIXED_DEC(255,1);
			}
			
			//Draw difficulty selector
			Menu_DifficultySelector(SCREEN_WIDTH - 100, SCREEN_HEIGHT2 - 48);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//Change option
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Select option if cross is pressed
				if (pad_state.press & (PAD_START | PAD_CROSS))
				{
					menu.next_page = MenuPage_Stage;
					menu.page_param.stage.id = menu_options[menu.select].stage;
					menu.page_param.stage.story = false;
					Trans_Start();
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 1; //Freeplay
					Trans_Start();
				}
			}

			//Draw Score
			char scoredisp[0x100];
			sprintf(scoredisp, "PERSONAL BEST: %d", stage.save.savescore[menu_options[menu.select].stage][menu.page_param.stage.diff] * 10);

			#ifndef NOSAVE
				menu.font_arial.draw(&menu.font_arial,
					scoredisp,
					SCREEN_WIDTH - 170,
					SCREEN_HEIGHT / 2 - 75,
					FontAlign_Left
				);
			#endif
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(30,1);
			menu.scroll += Lerp(menu.scroll, next_scroll, FIXED_DEC(1,1) / 16);
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 30) - (menu.scroll / FIXED_UNIT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;

				//Draw Icon
				Menu_DrawHealth(menu_options[i].icon, strlen(menu_options[i].text) * 13 + 38 + 4 + (y / 6), SCREEN_HEIGHT2 + y - 38);
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					48 + (y / 6),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			fixed_t tgt_r = (fixed_t)((menu_options[menu.select].col >> 16) & 0xFF) * FIXED_UNIT;
			fixed_t tgt_g = (fixed_t)((menu_options[menu.select].col >>  8) & 0xFF) * FIXED_UNIT;
			fixed_t tgt_b = (fixed_t)((menu_options[menu.select].col >>  0) & 0xFF) * FIXED_UNIT;
			
			menu.page_state.freeplay.back_r += Lerp(menu.page_state.freeplay.back_r, tgt_r, FIXED_DEC(1,1) / 16);
			menu.page_state.freeplay.back_g += Lerp(menu.page_state.freeplay.back_g, tgt_g, FIXED_DEC(1,1) / 16);
			menu.page_state.freeplay.back_b += Lerp(menu.page_state.freeplay.back_b, tgt_b, FIXED_DEC(1,1) / 16);
			
			Menu_DrawBack(
				true,
				8,
				menu.page_state.freeplay.back_r / (FIXED_UNIT * 2),
				menu.page_state.freeplay.back_g / (FIXED_UNIT * 2),
				menu.page_state.freeplay.back_b / (FIXED_UNIT * 2),
				0, 0, 0
			);
			break;
		}
		case MenuPage_Credits:
		{
			static const struct
			{
				const char *text; //The text (NULL skip that text)
			} menu_options[] = {
				{"potage engine by"},
				{NULL},
				{"IGORSOU"},
				{"LUCKY"},
				{"UNSTOPABLE"},
				{"SPICYJPEG"},
				{"SPARK"},
				{NULL},
				{"special thanks"},
				{NULL},
				{"MAXDEV"},
				{"CUCKYDEV"},
			};
			
			//Initialize page
			if (menu.page_swap)
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				//skip by 1 if next or previous text be NULL
				s8 skip;

				if (menu_options[menu.select - 1].text == NULL && pad_state.press & (PAD_UP))
					skip = -1;

				else if (menu_options[menu.select + 1].text == NULL && pad_state.press & (PAD_DOWN))
					skip = 1;

				else
					skip = 0;

				//Change options
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]) + skip;
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 2; //Credits
					Trans_Start();
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll / FIXED_UNIT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				255 / 2, 165 / 2, 0 / 2,
				0, 0, 0
			);
			break;
		}
		case MenuPage_Options:
		{
			static const struct
			{
				const char *text;
				u8 page;
			} menu_options[] = {
				{"VISUAL", MenuOptions_Visual},
				{"GAMEPLAY", MenuOptions_Gameplay},
			};

			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Go to option when cross is pressed
				if (pad_state.press & (PAD_CROSS | PAD_START))
				{
					menu.page = menu.next_page = menu_options[menu.select].page;
				}

				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.next_page = MenuPage_Main;
					menu.next_select = 3; //Options
					Trans_Start();
				}
			}

			//Save your game
			#ifndef NOSAVE
				if (pad_state.press & PAD_SELECT)
					WriteSave();

				RECT save_src = {0,120, 49, 7};
				RECT save_dst = {20, 23, 49*2, 7 *2};
				Gfx_DrawTex(&menu.tex_story, &save_src, &save_dst);

				//Reset Your Save
				if (pad_state.press & PAD_TRIANGLE)
					DefaultSettings();

				RECT reset_src = {0, 64, 57, 14};
				RECT reset_dst = {57*2 + 20, 9, 57*2, 14*2};
				Gfx_DrawTex(&menu.tex_story, &reset_src, &reset_dst);
		  #endif
			
			//Draw options
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8;
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(menu_options[i].text, menu.select != i),
					SCREEN_WIDTH2,
					SCREEN_HEIGHT2 + y - 8,
					FontAlign_Center
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				253 / 2, 113 / 2, 155 / 2,
				0, 0, 0
			);
			break;
		}
		case MenuOptions_Visual:
		{
			static const struct
			{
				enum
				{
					OptType_Boolean,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
				} spec;
			} menu_options[] = {
				{OptType_Boolean, "CAMERA ZOOM", &stage.save.canbump, {.spec_boolean = {0}}},
				{OptType_Boolean, "SPLASH", &stage.save.splash, {.spec_boolean = {0}}},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"VISUALS AND UI",
				16,
				16,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Handle option changing
				if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
					*((boolean*)menu_options[menu.select].value) ^= 1;
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.page = menu.next_page = MenuPage_Options;
				}
			}

			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll / FIXED_UNIT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				char text[0x80];
				sprintf(text, "%s %s", menu_options[i].text, *((boolean*)menu_options[i].value) ? "ON" : "OFF");

				//Draw text
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(text, menu.select != i),
					48 + (y / 4),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				253 / 2, 113 / 2, 155 / 2,
				0, 0, 0
			);
			break;
		}
		case MenuOptions_Gameplay:
		{
			static const char *gamemode_strs[] = {"NORMAL", "SWAP", "TWO PLAYER"};
			static const struct
			{
				enum
				{
					OptType_Boolean,
					OptType_Enum,
				} type;
				const char *text;
				void *value;
				union
				{
					struct
					{
						int a;
					} spec_boolean;
					struct
					{
						s32 max;
						const char **strs;
					} spec_enum;
				} spec;
			} menu_options[] = {
				{OptType_Enum,    "GAMEMODE", &stage.mode, {.spec_enum = {COUNT_OF(gamemode_strs), gamemode_strs}}},
				{OptType_Boolean, "GHOST TAP ", &stage.save.ghost, {.spec_boolean = {0}}},
				{OptType_Boolean, "DOWNSCROLL", &stage.save.downscroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "MIDDLESCROLL", &stage.save.middlescroll, {.spec_boolean = {0}}},
				{OptType_Boolean, "BOTPLAY", &stage.save.botplay, {.spec_boolean = {0}}},
				{OptType_Boolean, "SHOW TIMER", &stage.save.showtimer, {.spec_boolean = {0}}},
			};
			
			//Initialize page
			if (menu.page_swap)
			{
				menu.select = 0;
				menu.scroll = COUNT_OF(menu_options) * FIXED_DEC(24 + SCREEN_HEIGHT2,1);
			}
			
			//Draw page label
			menu.font_bold.draw(&menu.font_bold,
				"GAMEPLAY",
				16,
				16,
				FontAlign_Left
			);
			
			//Handle option and selection
			if (menu.next_page == menu.page && Trans_Idle())
			{
				menu.select = Menu_Scroll(menu.select, COUNT_OF(menu_options) - 1, &menu.sounds[0]);
				
				//Handle option changing
				switch (menu_options[menu.select].type)
				{
					case OptType_Boolean:
						if (pad_state.press & (PAD_CROSS | PAD_LEFT | PAD_RIGHT))
							*((boolean*)menu_options[menu.select].value) ^= 1;
						break;
					case OptType_Enum:
						if (pad_state.press & PAD_LEFT)
							if (--*((s32*)menu_options[menu.select].value) < 0)
								*((s32*)menu_options[menu.select].value) = menu_options[menu.select].spec.spec_enum.max - 1;
						if (pad_state.press & PAD_RIGHT)
							if (++*((s32*)menu_options[menu.select].value) >= menu_options[menu.select].spec.spec_enum.max)
								*((s32*)menu_options[menu.select].value) = 0;
						break;
				}
				
				//Return to main menu if circle is pressed
				if (pad_state.press & PAD_CIRCLE)
				{
					//Play Cancel Sound
					Audio_PlaySFX(menu.sounds[2], 80);

					menu.page = menu.next_page = MenuPage_Options;
				}
			}
			
			//Draw options
			s32 next_scroll = menu.select * FIXED_DEC(24,1);
			menu.scroll += (next_scroll - menu.scroll) / 16;
			
			for (u8 i = 0; i < COUNT_OF(menu_options); i++)
			{
				//Get position on screen
				s32 y = (i * 24) - 8 - (menu.scroll / FIXED_UNIT);
				if (y <= -SCREEN_HEIGHT2 - 8)
					continue;
				if (y >= SCREEN_HEIGHT2 + 8)
					break;
				
				//Draw text
				char text[0x80];
				switch (menu_options[i].type)
				{
					case OptType_Boolean:
						sprintf(text, "%s %s", menu_options[i].text, *((boolean*)menu_options[i].value) ? "ON" : "OFF");
						break;
					case OptType_Enum:
						sprintf(text, "%s %s", menu_options[i].text, menu_options[i].spec.spec_enum.strs[*((s32*)menu_options[i].value)]);
						break;
				}
				menu.font_bold.draw(&menu.font_bold,
					Menu_LowerIf(text, menu.select != i),
					48 + (y / 4),
					SCREEN_HEIGHT2 - 16 + y - 8,
					FontAlign_Left
				);
			}
			
			//Draw background
			Menu_DrawBack(
				true,
				8,
				253 / 2, 113 / 2, 155 / 2,
				0, 0, 0
			);
			break;
		}

		case MenuPage_Stage:
		{
			//Unload menu state
			Menu_Unload();
			
			//Load new stage
			LoadScr_Start();
			Stage_Load(menu.page_param.stage.id, menu.page_param.stage.diff, menu.page_param.stage.story);
			gameloop = GameLoop_Stage;
			LoadScr_End();
			break;
		}
		default:
			break;
	}
	
	//Clear page swap flag
	menu.page_swap = menu.page != exec_page;
}
