/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once 

#include "events.h"
#include "debug.h"
#include "psx/io.h"
#include "psx/gfx.h"
#include "psx/pad.h"

#include "psx/fixed.h"
#include "psx/character.h"
#include "psx/player.h"
#include "psx/object.h"

#include "psx/audio.h"
#include "psx/font.h"

//Stage constants
#define INPUT_LEFT  (PAD_LEFT  | PAD_SQUARE)
#define INPUT_DOWN  (PAD_DOWN  | PAD_CROSS)
#define INPUT_UP    (PAD_UP    | PAD_TRIANGLE)
#define INPUT_RIGHT (PAD_RIGHT | PAD_CIRCLE)

#define STAGE_FLAG_JUST_STEP     (1 << 0) //Song just stepped this frame
#define STAGE_FLAG_VOCAL_ACTIVE  (1 << 1) //Song's vocal track is currently active
#define STAGE_FLAG_PAUSED 			 (1 << 2) //Game Is Paused

#define STAGE_LOAD_PLAYER     (1 << 0) //Reload player character
#define STAGE_LOAD_OPPONENT   (1 << 1) //Reload opponent character
#define STAGE_LOAD_GIRLFRIEND (1 << 2) //Reload girlfriend character
#define STAGE_LOAD_STAGE      (1 << 3) //Reload stage
#define STAGE_LOAD_FLAG       (1 << 7)

//Stage enums
typedef enum
{
	StageId_Bopeebo,
	StageId_Fresh,
	StageId_DadBattle,
	StageId_Tutorial,
	
	StageId_Max
} StageId;

typedef enum
{
	StageDiff_Easy,
	StageDiff_Normal,
	StageDiff_Hard,

	StageDiff_Max,
} StageDiff;

typedef enum
{
	StageMode_Normal,
	StageMode_Swap,
	StageMode_2P,
} StageMode;

typedef enum
{
	StageTrans_Menu,
	StageTrans_NextStage,
	StageTrans_Reload,
} StageTrans;

//Stage background
typedef struct StageBack
{
	//Stage background functions
	void (*tick)(struct StageBack*);
	void (*draw_fg)(struct StageBack*);
	void (*draw_md)(struct StageBack*);
	void (*draw_bg)(struct StageBack*);
	void (*free)(struct StageBack*);
} StageBack;

//Stage definitions
typedef struct
{
	//Characters
	struct
	{
		Character* (*new)();
		fixed_t x, y;
	} pchar, ochar, gchar;
	
	//Stage background
	StageBack* (*back)();
	
	//Song info	
	const char* week;
	u8 week_song;
	XA_Track track;
	u8 channel;
	
	StageId next_stage;
	u8 next_load;
} StageDef;

//Stage state
#define SECTION_FLAG_OPPFOCUS (1 << 15) //Focus on opponent
#define SECTION_FLAG_BPM_MASK 0x7FFF //1/24

typedef struct
{
	u16 end; //1/12 steps
	u16 flag;
} Section;

#define NOTE_FLAG_OPPONENT    (1 << 2) //Note is opponent's
#define NOTE_FLAG_SUSTAIN     (1 << 3) //Note is a sustain note
#define NOTE_FLAG_SUSTAIN_END (1 << 4) //Is either end of sustain
#define NOTE_FLAG_ALT_ANIM    (1 << 5) //Note plays alt animation
#define NOTE_FLAG_MINE        (1 << 6) //Note is a mine
#define NOTE_FLAG_HIT         (1 << 7) //Note has been hit

typedef struct
{
	u16 pos; //1/12 steps
	u16 type;
} Note;

typedef struct
{
	IO_Data data;
	Section *sections;
	Note *notes;
	Event* events;

	Section *cur_section; //Current section
	Note *cur_note; //First visible and hittable note, used for drawing and hit detection
	Event* cur_event; //Current event
} Chart;

typedef struct
{
	Character *character;
	
	fixed_t arrow_hitan[4]; //Arrow hit animation for presses
	
	s16 health;
	u16 combo;
	
	boolean refresh_score;
	s32 score, max_score;
	char score_text[25];

	boolean refresh_misses;
	u16 misses;
	char misses_text[25];

	boolean refresh_accuracy;
	u32 min_accuracy, max_accuracy, accuracy;
	char accuracy_text[25];

	//Score, misses and accuracy
	char info_text[25*3];
	
	u16 pad_held, pad_press;
} PlayerState;

typedef struct
{
	//Stage settings
	s32 mode;

	//Variables that you want save
	struct
	{
		boolean ghost, middlescroll, downscroll, healthdrain, showtimer, botplay, canbump, splash;

		u32 savescore[StageId_Max][StageDiff_Max];	
	}save;
	
	u32 offset;
	
	//HUD textures
	Gfx_Tex tex_hud0, tex_hud1, tex_intro;

	//Game over
	Gfx_Tex tex_gameover;
	IO_Data gameover_tim;
	char gameover_path[30];

	//Notes
	fixed_t note_x[8];
	fixed_t note_y[8];

	//Font Stuff
	FontData font_cdr, font_bold;

	//Sound Effects
	SFX introsound[4];
	SFX sounds[6];

	//Pause state
	u8 pause_select;
  fixed_t pause_scroll;
	
	//Stage data
	const StageDef *stage_def;
	StageId stage_id;
	StageDiff stage_diff;
	
	Chart main_chart;
	Chart event_chart; //event.json
	boolean exist_event_json;
	
	fixed_t speed, ogspeed;
	fixed_t step_crochet, step_time;
	fixed_t early_safe, late_safe, early_sus_safe, late_sus_safe;
	
	//Stage state
	boolean story;
	u8 flag;
	StageTrans trans;

	//Timer song
	char timer_display[10];
	u16 timer, timersec, timermin;
	
	struct
	{
		fixed_t x, y, zoom;
		fixed_t tx, ty, tz, td;
		fixed_t bzoom;
	} camera;
	fixed_t bump, sbump, character_bump;
	
	StageBack *back;
	
	Character *player;
	Character *opponent;
	Character *gf;
	
	fixed_t note_scroll, song_time, interp_time, interp_ms, interp_speed;

	//week 6 combo
	boolean pixelcombo;
	
	u16 last_bpm;
	
	fixed_t time_base;
	u16 step_base;
	Section *section_base;
	
	s32 song_step;
	s16 song_beat; //Step / 4
	
	u8 gf_speed; //Typically 4 steps, changes in Fresh
	
	PlayerState player_state[2];
	s32 max_score;
	
	enum
	{
		StageState_Play, 			//Game is playing as normal
		StageState_Dead,      //Unload a lot of data
		StageState_DeadLoop,	//Retry prompt
	} state;
	
	u8 note_swap;
	
	//Object lists
	ObjectList objlist_splash, objlist_fg, objlist_bg;
} Stage;

extern Stage stage;

//Stage drawing functions
void Stage_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, u8 r, u8 g, u8 b);
void Stage_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom);
void Stage_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, u8 r, u8 g, u8 b, fixed_t zoom);
void Stage_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom);

//Stage functions
void Stage_Load(StageId id, StageDiff difficulty, boolean story);
void Stage_LoadScr(StageId id, StageDiff difficulty, boolean story);
void Stage_UnloadChart(Chart* chart);
void Stage_Unload(void);
void Stage_Tick(void);
