/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "stage.h"

#include "psx/mem.h"
#include "psx/timer.h"
#include "psx/audio.h"
#include "psx/pad.h"
#include "psx/main.h"
#include "psx/random.h"
#include "psx/movie.h"
#include "psx/mutil.h"

#include "menu/menu.h"
#include "psx/trans.h"
#include "psx/loadscr.h"

#include "object/combo.h"
#include "object/splash.h"

//Stage constants
//#define DEBUG_MODE
#define NOTE_SIZE 32

static const u16 note_key[] = {INPUT_LEFT, INPUT_DOWN, INPUT_UP, INPUT_RIGHT};
static const u8 note_anims[4][3] = {
	{CharAnim_Left,  CharAnim_LeftAlt,  PlayerAnim_LeftMiss},
	{CharAnim_Down,  CharAnim_DownAlt,  PlayerAnim_DownMiss},
	{CharAnim_Up,    CharAnim_UpAlt,    PlayerAnim_UpMiss},
	{CharAnim_Right, CharAnim_RightAlt, PlayerAnim_RightMiss},
};

//Stage definitions
#include "characters/bf.h"
#include "characters/dad.h"
#include "characters/gf.h"

#include "weeks/dummy.h"
#include "weeks/stage.h"

static const StageDef stage_defs[StageId_Max] = {
	#include "stagedef_disc1.h"
};

//Stage state
Stage stage;

//Stage music functions
static void Stage_StartVocal(void)
{
	if (!(stage.flag & STAGE_FLAG_VOCAL_ACTIVE))
	{
		Audio_ChannelXA(stage.stage_def->channel);
		stage.flag |= STAGE_FLAG_VOCAL_ACTIVE;
	}
}

static void Stage_CutVocal(void)
{
	if (stage.flag & STAGE_FLAG_VOCAL_ACTIVE)
	{
		Audio_ChannelXA(stage.stage_def->channel + 1);
		stage.flag &= ~STAGE_FLAG_VOCAL_ACTIVE;
	}
}

//Stage camera functions
static void Stage_FocusCharacter(Character *ch, fixed_t div)
{
	//Use character focus settings to update target position and zoom
	stage.camera.tx = ch->x + ch->focus_x;
	stage.camera.ty = ch->y + ch->focus_y;
	stage.camera.tz = ch->focus_zoom;
	stage.camera.td = div;
}

static void Stage_ScrollCamera(void)
{
	//Don't do anything if game is paused
	if (!(stage.flag & STAGE_FLAG_PAUSED))
	{
		#ifdef DEBUG_MODE
			Debug_Tick();
		#else
			//Scroll based off current divisor
			stage.camera.x += Lerp(stage.camera.x, stage.camera.tx, stage.camera.td);
			stage.camera.y += Lerp(stage.camera.y, stage.camera.ty, stage.camera.td);
			stage.camera.zoom += Lerp(stage.camera.zoom, stage.camera.tz, stage.camera.td);
		#endif
		
		//Update other camera stuff
		stage.camera.bzoom = FIXED_MUL(stage.camera.zoom, stage.character_bump);
	}
}

//Stage section functions
static void Stage_ChangeBPM(u16 bpm, u16 step)
{
	//Update last BPM
	stage.last_bpm = bpm;
	
	//Update timing base
	if (stage.step_crochet)
		stage.time_base += FIXED_DIV(((fixed_t)step - stage.step_base) << FIXED_SHIFT, stage.step_crochet);
	stage.step_base = step;
	
	//Get new crochet and times
	stage.step_crochet = ((fixed_t)bpm << FIXED_SHIFT) * 8 / 240; //15/12/24
	stage.step_time = FIXED_DIV(FIXED_DEC(12,1), stage.step_crochet);
	
	//Get new crochet based values
	stage.early_safe = stage.late_safe = stage.step_crochet / 6; //10 frames
	stage.late_sus_safe = stage.late_safe;
	stage.early_sus_safe = stage.early_safe * 2 / 5;
}

static Section *Stage_GetPrevSection(Section *section)
{
	if (section > stage.main_chart.sections)
		return section - 1;
	return NULL;
}

static u16 Stage_GetSectionStart(Section *section)
{
	Section *prev = Stage_GetPrevSection(section);
	if (prev == NULL)
		return 0;
	return prev->end;
}

//Section scroll structure
typedef struct
{
	fixed_t start;   //Seconds
	fixed_t length;  //Seconds
	u16 start_step;  //Sub-steps
	u16 length_step; //Sub-steps
	
	fixed_t size; //Note height
} SectionScroll;

static void Stage_GetSectionScroll(SectionScroll *scroll, Section *section)
{
	//Get BPM
	u16 bpm = section->flag & SECTION_FLAG_BPM_MASK;
	
	//Get section step info
	scroll->start_step = Stage_GetSectionStart(section);
	scroll->length_step = section->end - scroll->start_step;
	
	//Get section time length
	scroll->length = (scroll->length_step * FIXED_DEC(15,1) / 12) * 24 / bpm;
	
	//Get note height
	scroll->size = FIXED_MUL(stage.speed, scroll->length * (12 * 150) / scroll->length_step) + FIXED_UNIT;
}

//Note hit detection
static u8 Stage_HitNote(PlayerState *this, u8 type, fixed_t offset)
{
	//Get hit type
	if (offset < 0)
		offset = -offset;
	
	u8 hit_type;
	if (offset > stage.late_safe * 9 / 11)
		hit_type = 3; //SHIT
	else if (offset > stage.late_safe * 6 / 11)
		hit_type = 2; //BAD
	else if (offset > stage.late_safe * 3 / 11)
		hit_type = 1; //GOOD
	else
		hit_type = 0; //SICK
	
	//Increment combo, score and accuracy
	this->combo++;
	
	static const s32 score_inc[] = {
		35, //SICK
		20, //GOOD
		10, //BAD
		 5, //SHIT
	};
	this->score += score_inc[hit_type];
	this->refresh_score = true;

	this->min_accuracy += 10;
	this->max_accuracy += 10 + (hit_type*3);
	this->refresh_accuracy = true;
	
	//Restore vocals and health
	Stage_StartVocal();
	this->health += 230;
	
	//Create combo object telling of our combo
	Obj_Combo *combo = Obj_Combo_New(
		this->character,
		hit_type,
		this->combo >= 10 ? this->combo : 0xFFFF
	);
	if (combo != NULL)
		ObjectList_Add(&stage.objlist_fg, (Object*)combo);
	
	//Create note splashes if SICK
	if (hit_type == 0)
	{
		for (int i = 0; i < 3; i++)
		{
			//Create splash object
			if (stage.save.splash == true)
			{
				Obj_Splash *splash = Obj_Splash_New(
					stage.note_x[type],
					stage.note_y[type] * (stage.save.downscroll ? -1 : 1),
					type % 4
				);
				if (splash != NULL)
					ObjectList_Add(&stage.objlist_splash, (Object*)splash);
			}
		}
	}
	
	return hit_type;
}

static void Stage_MissNote(PlayerState *this)
{
	//Added misses and fuck accuracy
	this->misses++;
	this->refresh_misses = true;

	this->max_accuracy += 10;
	this->refresh_accuracy = true;

	if (this->combo)
	{
		//Kill combo
		if (stage.gf != NULL && this->combo > 5)
			stage.gf->set_anim(stage.gf, CharAnim_DownAlt); //Cry if we lost a large combo
		this->combo = 0;
		
		//Create combo object telling of our lost combo
		Obj_Combo *combo = Obj_Combo_New(
			this->character,
			0xFF,
			0
		);
		if (combo != NULL)
			ObjectList_Add(&stage.objlist_fg, (Object*)combo);
	}
}

static void Stage_NoteCheck(PlayerState *this, u8 type)
{
	//Perform note check
	for (Note *note = stage.main_chart.cur_note;; note++)
	{
		if (!(note->type & NOTE_FLAG_MINE))
		{
			//Check if note can be hit
			fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
			if (note_fp - stage.early_safe > stage.note_scroll)
				break;
			if (note_fp + stage.late_safe < stage.note_scroll)
				continue;
			if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
				continue;
			
			//Hit the note
			note->type |= NOTE_FLAG_HIT;
			
			if (this->character != NULL)
				this->character->set_anim(this->character, note_anims[type % 4][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);

			u8 hit_type = Stage_HitNote(this, type, stage.note_scroll - note_fp);
			this->arrow_hitan[type % 4] = stage.step_time;

			(void)hit_type;
			return;
		}
		else
		{
			//Check if mine can be hit
			fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
			if (note_fp - (stage.late_safe * 3 / 5) > stage.note_scroll)
				break;
			if (note_fp + (stage.late_safe * 2 / 5) < stage.note_scroll)
				continue;
			if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || (note->type & NOTE_FLAG_SUSTAIN))
				continue;
			
			//Hit the mine
			note->type |= NOTE_FLAG_HIT;
			
			this->health -= 2000;

			//Make character sing if he exist
			if (this->character != NULL)
			{
				if (this->character->spec & CHAR_SPEC_MISSANIM)
				this->character->set_anim(this->character, note_anims[type % 4][2]);

				else
					this->character->set_anim(this->character, note_anims[type % 4][0]);
			}

			this->arrow_hitan[type % 4] = -1;
			return;
		}
	}
	
	//Missed a note
	this->arrow_hitan[type % 4] = -1;
	
	if (!stage.save.ghost)
	{	
		//Make character sing if he exist
			if (this->character != NULL)
			{
				if (this->character->spec & CHAR_SPEC_MISSANIM)
				this->character->set_anim(this->character, note_anims[type % 4][2]);

				else
					this->character->set_anim(this->character, note_anims[type % 4][0]);
			}
		
		this->health -= 550;
		this->score -= 1;
		this->refresh_score = true;
	}
}

static void Stage_SustainCheck(PlayerState *this, u8 type)
{
	//Perform note check
	for (Note *note = stage.main_chart.cur_note;; note++)
	{
		//Check if note can be hit
		fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
		if (note_fp - stage.early_sus_safe > stage.note_scroll)
			break;
		if (note_fp + stage.late_sus_safe < stage.note_scroll)
			continue;
		if ((note->type & NOTE_FLAG_HIT) || (note->type & (NOTE_FLAG_OPPONENT | 0x3)) != type || !(note->type & NOTE_FLAG_SUSTAIN))
			continue;
		
		//Hit the note
		note->type |= NOTE_FLAG_HIT;
		
		//Make character sing if he exist
		if (this->character != NULL)
			this->character->set_anim(this->character, note_anims[type % 4][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);
		
		Stage_StartVocal();
		this->health += 210;
		this->arrow_hitan[type % 4] = stage.step_time;
	}
}

static void Stage_ProcessPlayer(PlayerState *this, Pad *pad, boolean playing)
{
	//Handle player note presses
	if (!stage.save.botplay)
	{
		if (playing)
		{
			u8 i = (this->character == stage.opponent) ? NOTE_FLAG_OPPONENT : 0;
			
			this->pad_held = this->character->pad_held = pad->held;
			this->pad_press = pad->press;
			
			if (this->pad_held & INPUT_LEFT)
				Stage_SustainCheck(this, 0 | i);
			if (this->pad_held & INPUT_DOWN)
				Stage_SustainCheck(this, 1 | i);
			if (this->pad_held & INPUT_UP)
				Stage_SustainCheck(this, 2 | i);
			if (this->pad_held & INPUT_RIGHT)
				Stage_SustainCheck(this, 3 | i);
			
			if (this->pad_press & INPUT_LEFT)
				Stage_NoteCheck(this, 0 | i);
			if (this->pad_press & INPUT_DOWN)
				Stage_NoteCheck(this, 1 | i);
			if (this->pad_press & INPUT_UP)
				Stage_NoteCheck(this, 2 | i);
			if (this->pad_press & INPUT_RIGHT)
				Stage_NoteCheck(this, 3 | i);
		}
		else
		{
			this->pad_held = this->character->pad_held = 0;
			this->pad_press = 0;
		}
	}
	
	//Do perfect note checks
	if (stage.save.botplay)
	{
		if (playing)
		{
			u8 i = (this->character == stage.opponent) ? NOTE_FLAG_OPPONENT : 0;
			
			u8 hit[4] = {0, 0, 0, 0};
			for (Note *note = stage.main_chart.cur_note;; note++)
			{
				//Check if note can be hit
				fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
				if (note_fp - stage.early_safe - FIXED_DEC(12,1) > stage.note_scroll)
					break;
				if (note_fp + stage.late_safe < stage.note_scroll)
					continue;
				if ((note->type & NOTE_FLAG_MINE) || (note->type & NOTE_FLAG_OPPONENT) != i)
					continue;
				
				//Handle note hit
				if (!(note->type & NOTE_FLAG_SUSTAIN))
				{
					if (note->type & NOTE_FLAG_HIT)
						continue;
					if (stage.note_scroll >= note_fp)
						hit[note->type % 4] |= 1;
					else if (!(hit[note->type % 4] & 8))
						hit[note->type % 4] |= 2;
				}
				else if (!(hit[note->type % 4] & 2))
				{
					if (stage.note_scroll <= note_fp)
						hit[note->type % 4] |= 4;
					hit[note->type % 4] |= 8;
				}
			}
			
			//Handle input
			this->pad_held = 0;
			this->pad_press = 0;
			
			for (u8 j = 0; j < 4; j++)
			{
				if (hit[j] & 5)
				{
					this->pad_held |= note_key[j];
					Stage_SustainCheck(this, j | i);
				}
				if (hit[j] & 1)
				{
					this->pad_press |= note_key[j];
					Stage_NoteCheck(this, j | i);
				}
			}
			
			this->character->pad_held = this->pad_held;
		}
		else
		{
			this->pad_held = this->character->pad_held = 0;
			this->pad_press = 0;
		}
	}
}

//Stage drawing functions
void Stage_DrawTexCol(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom, u8 cr, u8 cg, u8 cb)
{
	fixed_t xz = dst->x;
	fixed_t yz = dst->y;
	fixed_t wz = dst->w;
	fixed_t hz = dst->h;
	
	#ifdef STAGE_NOHUD
		if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
			return;
	#endif
	
	fixed_t left = (SCREEN_WIDTH2  << FIXED_SHIFT) + FIXED_MUL(xz, zoom);// + FIXED_DEC(1,2);
	fixed_t top = (SCREEN_HEIGHT2 << FIXED_SHIFT) + FIXED_MUL(yz, zoom);// + FIXED_DEC(1,2);
	fixed_t right = left + FIXED_MUL(wz, zoom);
	fixed_t bottom = top + FIXED_MUL(hz, zoom);
	
	left >>= FIXED_SHIFT;
	top >>= FIXED_SHIFT;
	right >>= FIXED_SHIFT;
	bottom >>= FIXED_SHIFT;
	
	RECT sdst = {
		left,
		top,
		right - left,
		bottom - top,
	};
	Gfx_DrawTexCol(tex, src, &sdst, cr, cg, cb);
}

void Stage_DrawTex(Gfx_Tex *tex, const RECT *src, const RECT_FIXED *dst, fixed_t zoom)
{
	Stage_DrawTexCol(tex, src, dst, zoom, 0x80, 0x80, 0x80);
}

void Stage_DrawTex3DCol(Gfx_Tex *tex, const RECT *src, RECT_FIXED *dst, fixed_t camera_x, fixed_t camera_y, u8 r, u8 g, u8 b, fixed_t zoom)
{
	//Draw stage
	fixed_t fx, fy;

	fx = camera_x * 3 / 2;
	fy = camera_y * 3 / 2;
	
	POINT_FIXED point_2 = {
		dst->x - fx,
		dst->y + dst->h - fy,
	};
	POINT_FIXED point_3 = {
		dst->x + dst->w - fx,
		dst->y + dst->h - fy,
	};
	
	fx = camera_x >> 1;
	fy = camera_y >> 1;
	
	POINT_FIXED point_0 = {
		dst->x - fx,
		dst->y - fy,
	};
	POINT_FIXED point_1 = {
		dst->x + dst->w - fx,
		dst->y - fy,
	};

	Stage_DrawTexArbCol(tex, src, &point_0, &point_1, &point_2, &point_3, r, g, b, zoom);
}

void Stage_DrawTex3D(Gfx_Tex *tex, const RECT *src, RECT_FIXED *dst, fixed_t camera_x, fixed_t camera_y, fixed_t zoom)
{
	Stage_DrawTex3DCol(tex, src, dst, camera_x, camera_y, 128, 128, 128, zoom);
}

void Stage_DrawTexArbCol(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, u8 r, u8 g, u8 b, fixed_t zoom)
{
	//Don't draw if HUD and HUD is disabled
	#ifdef STAGE_NOHUD
		if (tex == &stage.tex_hud0 || tex == &stage.tex_hud1)
			return;
	#endif
	
	//Get screen-space points
	POINT s0 = {SCREEN_WIDTH2 + (FIXED_MUL(p0->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p0->y, zoom) >> FIXED_SHIFT)};
	POINT s1 = {SCREEN_WIDTH2 + (FIXED_MUL(p1->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p1->y, zoom) >> FIXED_SHIFT)};
	POINT s2 = {SCREEN_WIDTH2 + (FIXED_MUL(p2->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p2->y, zoom) >> FIXED_SHIFT)};
	POINT s3 = {SCREEN_WIDTH2 + (FIXED_MUL(p3->x, zoom) >> FIXED_SHIFT), SCREEN_HEIGHT2 + (FIXED_MUL(p3->y, zoom) >> FIXED_SHIFT)};
	
	Gfx_DrawTexArbCol(tex, src, &s0, &s1, &s2, &s3, r, g, b);
}

void Stage_DrawTexArb(Gfx_Tex *tex, const RECT *src, const POINT_FIXED *p0, const POINT_FIXED *p1, const POINT_FIXED *p2, const POINT_FIXED *p3, fixed_t zoom)
{
	Stage_DrawTexArbCol(tex, src, p0, p1, p2, p3, 0x80, 0x80, 0x80, zoom);
}

//Stage HUD functions
static void Stage_TimerTick(void)
{
	//Has the song started?
	if (stage.song_step >= 0) //If song starts decrease the timer
	{
			//Don't change anything if timer be 0
			if (stage.timer != 0)
   			stage.timer = Audio_GetLength(stage.stage_def->track) - (stage.song_time >> FIXED_SHIFT);
  }

  else //If not keep the timer at the song starting length	
 	    stage.timer = Audio_GetLength(stage.stage_def->track); //Seconds (ticks down)

  stage.timermin = stage.timer / 60; //Minutes left till song ends
  stage.timersec = stage.timer % 60; //Seconds left till song ends
}

static void Stage_TimerDraw(void)
{
	Stage_TimerTick();

	RECT bar_fill = {0, 248,200 - (200 * stage.timer / Audio_GetLength(stage.stage_def->track)), 6};
	RECT bar_back = {0, 248,200, 6};

	RECT_FIXED bar_dst = {FIXED_DEC(-50,1), FIXED_DEC(-110,1), 0, FIXED_DEC(4,1)};

	if (stage.save.downscroll)
		bar_dst.y = -bar_dst.y - bar_dst.h;

	//Format timer string
	sprintf(stage.timer_display, "%d : %s%d", 
		stage.timermin,
		(stage.timersec < 10) ? "0" : "", //Making this for avoid number like this 1:4
		stage.timersec
	);

	//Display timer
	if (stage.save.showtimer)
	{
		stage.font_cdr.draw(&stage.font_cdr,
			stage.timer_display,
			-11, 
			(stage.save.downscroll) ? SCREEN_HEIGHT2 - 8 - 9 : -SCREEN_HEIGHT2 + 8,
			FontAlign_Left
		);

		bar_dst.w = (bar_fill.w / 2) << FIXED_SHIFT;
		Stage_DrawTex(&stage.tex_hud1, &bar_fill, &bar_dst, stage.bump);
		bar_dst.w = (bar_back.w / 2) << FIXED_SHIFT;
		Stage_DrawTexCol(&stage.tex_hud1, &bar_back, &bar_dst, stage.bump, 0, 0, 0);
	}
}

static void Stage_DrawCountdown(void)
{
	const s8 start_draw_step = -25;
	const u8 increase_count_step = 5;
	const u8 intro_width = 120;
	const u8 intro_height = 50;

	//Calculate the countdown number to display based on the current song step
	s8 countdown_number = 2 - (-stage.song_step / increase_count_step); 

	if (stage.song_step >= start_draw_step)
	{
		//Get src and dst of countdown
		RECT countdown_src = {
			(countdown_number % 2) * intro_width,
			(countdown_number / 2) * intro_height,
			intro_width,
			intro_height
		};

		RECT_FIXED countdown_dst = {
			FIXED_DEC(-countdown_src.w,1),
			FIXED_DEC(-43,1),
			FIXED_DEC(countdown_src.w * 2, 1),
			FIXED_DEC(countdown_src.h * 2, 1),
		};

		//Draw the countdown texture if the countdown number is non-negative
		if (countdown_number >= 0)
			Stage_DrawTex(&stage.tex_intro, &countdown_src, &countdown_dst, stage.camera.bzoom);

		//Play the appropriate intro sound if the song step is a multiple of increase_count_step
		if (stage.flag & STAGE_FLAG_JUST_STEP && (stage.song_step % increase_count_step) == 0)
			Audio_PlaySFX(stage.introsound[countdown_number + 2], 90);
	}
}

static void Stage_DrawHealthBar(u32 health_bar_color, s8 offsetx)
{
	//Extract the RGB components of the healthBarColor
  u8 r = ((health_bar_color >> 16) & 0xFF) / 2;
  u8 g = ((health_bar_color >> 8) & 0xFF) / 2;
  u8 b = ((health_bar_color) & 0xFF) / 2;

	// Determine the width of the health bar based on player position
  u8 health_width = (offsetx > 0) ? 200 : (200 - (200 * stage.player_state[0].health / 20000));

	//Get src and dst
	RECT health_src = {0, 248, health_width, 6};
	RECT_FIXED health_dst = {FIXED_DEC(-100,1), (SCREEN_HEIGHT2 - 28) << FIXED_SHIFT, health_width << FIXED_SHIFT, FIXED_DEC(6,1)};

	//Adjust the Y position for downscroll mode
	if (stage.save.downscroll)
			health_dst.y = -health_dst.y - health_dst.h + FIXED_DEC(5,1);
				
	Stage_DrawTexCol(&stage.tex_hud1, &health_src, &health_dst, stage.bump, r, g, b);
}
static void Stage_DrawHealthIcon(s16 health, Character* this, s8 offsetx)
{
	//Icon Size
	u8 icon_size = 38;

	//Check if we should use 'dying' icon
	s8 dying;
	if (offsetx < 0)
		dying = (health >= 18000) * icon_size;
	else
		dying = (health <= 2000) * icon_size;
	
	//Get src and dst
	fixed_t hx = (100 << FIXED_SHIFT) * (10000 - health) / 10000;
	RECT src = {
		(this->health_i % 3) * (icon_size * 2) + dying,
		(this->health_i / 3) * icon_size,
		icon_size,
		icon_size
	};

	RECT_FIXED dst = {
		hx + offsetx * FIXED_DEC(17,1) - FIXED_DEC(16,1),
		FIXED_DEC(SCREEN_HEIGHT2 - 32 + 4 - (icon_size / 2), 1),
		src.w << FIXED_SHIFT,
		src.h << FIXED_SHIFT
	};

	if (stage.save.downscroll)
		dst.y = -dst.y - dst.h;

	//Swap icons X if be swap mode
	if (stage.mode == StageMode_Swap)
	{
		dst.x += dst.w;
		dst.w = -dst.w;
	}

	//Swap icon if character is player
	if (this->spec & CHAR_SPEC_ISPLAYER)
	{
		dst.x += dst.w;
		dst.w = -dst.w;
	}
	
	//Draw health icon
	Stage_DrawTex(&stage.tex_hud1, &src, &dst, FIXED_MUL(stage.bump, stage.sbump));
}

static void Stage_DrawStrum(u8 i, RECT *note_src, RECT_FIXED *note_dst)
{
	(void)note_dst;
	
	PlayerState *this = &stage.player_state[((i ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0];
	i %= 0x4;
	
	if (this->arrow_hitan[i] > 0)
	{
		//Play hit animation
		u8 frame = ((this->arrow_hitan[i] * 2) / stage.step_time) % 2;
		note_src->x = (i + 1) * 32;
		note_src->y = 64 - (frame * 32);
		
		this->arrow_hitan[i] -= timer_dt;
		if (this->arrow_hitan[i] <= 0)
		{
			if (this->pad_held & note_key[i])
				this->arrow_hitan[i] = 1;
			else
				this->arrow_hitan[i] = 0;
		}
	}
	else if (this->arrow_hitan[i] < 0)
	{
		//Play depress animation
		note_src->x = (i + 1) * 32;
		note_src->y = 96;
		if (!(this->pad_held & note_key[i]))
			this->arrow_hitan[i] = 0;
	}
	else
	{
		note_src->x = 0;
		note_src->y = i * 32;
	}
}

//When You Paused The Game
static void Stage_DrawPause(void)
{
  static const char *stage_options[] = {
        "RESUME",
        "RESTART SONG",
        "EXIT TO MENU"
      };

  //Select option if cross or start is pressed
  if (pad_state.press & (PAD_CROSS | PAD_START))
  {
    switch (stage.pause_select)
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

   //Change option
   stage.pause_select = Menu_Scroll(stage.pause_select, COUNT_OF(stage_options) - 1, &stage.sounds[0]);
        

  //Draw options
  if (stage.pause_scroll == -1)
      stage.pause_scroll = COUNT_OF(stage_options) * FIXED_DEC(33,1);

  //Draw options
  s32 next_scroll = stage.pause_select * FIXED_DEC(33,1);
  stage.pause_scroll += Lerp(stage.pause_scroll, next_scroll, FIXED_DEC(1,1) / 8);

  for (u8 i = 0; i < COUNT_OF(stage_options); i++)
  {
    //Get position on screen
    s32 y = (i * 33) - 8 - (stage.pause_scroll >> FIXED_SHIFT);
    if (y <= -SCREEN_HEIGHT2 - 8)
      continue;
    if (y >= SCREEN_HEIGHT2 + 8)
      break;
        
    //Draw text
    stage.font_bold.draw_col(&stage.font_bold,
    stage_options[i],
    20 + y /4,
    y + 120,
    FontAlign_Left,
    //if the option is the one you are selecting, draw in normal color, else, draw gray
    (i == stage.pause_select) ? 0x80 : 160 / 2,
    (i == stage.pause_select) ? 0x80 : 160 / 2,
    (i == stage.pause_select) ? 0x80 : 160 / 2
    );
  }
  //50% Blend
  RECT screen_src = {0, 0 ,SCREEN_WIDTH, SCREEN_HEIGHT};

  Gfx_BlendRect(&screen_src, 0, 0, 0, 0);
}

static void Stage_DrawNotes(void)
{
	//Check if opponent should draw as bot
	u8 bot = (stage.mode >= StageMode_2P) ? 0 : NOTE_FLAG_OPPONENT;
	
	//Initialize scroll state
	SectionScroll scroll;
	scroll.start = stage.time_base;
	
	Section *scroll_section = stage.section_base;
	Stage_GetSectionScroll(&scroll, scroll_section);
	
	//Push scroll back until cur_note is properly contained
	while (scroll.start_step > stage.main_chart.cur_note->pos)
	{
		//Look for previous section
		Section *prev_section = Stage_GetPrevSection(scroll_section);
		if (prev_section == NULL)
			break;
		
		//Push scroll back
		scroll_section = prev_section;
		Stage_GetSectionScroll(&scroll, scroll_section);
		scroll.start -= scroll.length;
	}
	
	//Draw notes
	for (Note *note = stage.main_chart.cur_note; note->pos != 0xFFFF; note++)
	{
		//Update scroll
		while (note->pos >= scroll_section->end)
		{
			//Push scroll forward
			scroll.start += scroll.length;
			Stage_GetSectionScroll(&scroll, ++scroll_section);
		}
		
		//Get note information
		u8 i = ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) != 0;
		PlayerState *this = &stage.player_state[i];
		
		fixed_t note_fp = (fixed_t)note->pos << FIXED_SHIFT;
		fixed_t time = (scroll.start - stage.song_time) + (scroll.length * (note->pos - scroll.start_step) / scroll.length_step);
		fixed_t y = stage.note_y[note->type % 8] + FIXED_MUL(stage.speed, time * 150);
		
		//Check if went above screen
		if (y < FIXED_DEC(-16 - SCREEN_HEIGHT2, 1))
		{
			//Wait for note to exit late time
			if (note_fp + stage.late_safe >= stage.note_scroll)
				continue;
			
			//Miss note if player's note
			if (!((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT | NOTE_FLAG_MINE)))
			{
				//Missed note
				Stage_CutVocal();
				Stage_MissNote(this);
				this->health -= 575;	
			}
			
			//Update current note
			stage.main_chart.cur_note++;
		}
		else
		{
			//Don't draw if below screen
			RECT note_src;
			RECT_FIXED note_dst;
			if (y > (FIXED_DEC(SCREEN_HEIGHT,2) + scroll.size) || note->pos == 0xFFFF)
				break;
			
			//Draw note
			if (note->type & NOTE_FLAG_SUSTAIN)
			{
				//Check for sustain clipping
				fixed_t clip;
				y -= scroll.size;
				if (((note->type ^ stage.note_swap) & (bot | NOTE_FLAG_HIT)) || ((this->pad_held & note_key[note->type % 4]) && (note_fp + stage.late_sus_safe >= stage.note_scroll)))
				{
					clip = FIXED_DEC(32 - SCREEN_HEIGHT2, 1) - y;
					if (clip < 0)
						clip = 0;
				}
				else
				{
					clip = 0;
				}
				
				//Draw sustain
				if (note->type & NOTE_FLAG_SUSTAIN_END)
				{
					if (clip < (24 << FIXED_SHIFT))
					{
						note_src.x = 160;
						note_src.y = ((note->type % 4) * 32) + (clip >> FIXED_SHIFT);
						note_src.w = 32;
						note_src.h = 28 - (clip >> FIXED_SHIFT);
						
						note_dst.x = stage.note_x[(note->type % 8)] - FIXED_DEC(16,1);
						note_dst.y = y + clip;
						note_dst.w = note_src.w << FIXED_SHIFT;
						note_dst.h = (note_src.h << FIXED_SHIFT);
						
						if (stage.save.downscroll)
						{
							note_dst.y = -note_dst.y;
							note_dst.h = -note_dst.h;
						}
						Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
					}
				}
				else
				{
					//Get note height
					fixed_t next_time = (scroll.start - stage.song_time) + (scroll.length * (note->pos + 12 - scroll.start_step) / scroll.length_step);
					fixed_t next_y = stage.note_y[note->type % 8] + FIXED_MUL(stage.speed, next_time * 150) - scroll.size;
					fixed_t next_size = next_y - y;
					
					if (clip < next_size)
					{
						note_src.x = 160;
						note_src.y = ((note->type % 4) * 32);
						note_src.w = 32;
						note_src.h = 16;
						
						note_dst.x = stage.note_x[(note->type % 8)] - FIXED_DEC(16,1);
						note_dst.y = y + clip;
						note_dst.w = note_src.w << FIXED_SHIFT;
						note_dst.h = (next_y - y) - clip;
						
						if (stage.save.downscroll)
							note_dst.y = -note_dst.y - note_dst.h;
						Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
					}
				}
			}
			else if (note->type & NOTE_FLAG_MINE)
			{
				//Don't draw if already hit
				if (note->type & NOTE_FLAG_HIT)
					continue;
				
				//Draw note body
				note_src.x = 192 + ((note->type % 2) * 32);
				note_src.y = (note->type / 2) * 16;
				note_src.w = 32;
				note_src.h = 32;
				
				note_dst.x = stage.note_x[(note->type % 8)] - FIXED_DEC(16,1);
				note_dst.y = y - FIXED_DEC(16,1);
				note_dst.w = note_src.w << FIXED_SHIFT;
				note_dst.h = note_src.h << FIXED_SHIFT;
				
				if (stage.save.downscroll)
					note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				
				/*
				if (stage.stage_id == StageId_Clwn_4)
				{
					//Draw note halo
					note_src.x = 160;
					note_src.y = 128 + ((animf_count % 4) * 8);
					note_src.w = 32;
					note_src.h = 8;
					
					note_dst.y -= FIXED_DEC(6,1);
					note_dst.h /= 4;
					
					Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				}
				else
				*/
				{
					//Draw note fire
					note_src.x = 192 + ((animf_count % 2) * 32);
					note_src.y = 64 + ((animf_count / 2) * 24);
					note_src.w = 32;
					note_src.h = 48;
					
					if (stage.save.downscroll)
					{
						note_dst.y += note_dst.h;
						note_dst.h = note_dst.h * -3 / 2;
					}
					else
					{
						note_dst.h = note_dst.h * 3 / 2;
					}
					Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				}
			}
			else
			{
				//Don't draw if already hit
				if (note->type & NOTE_FLAG_HIT)
					continue;
				
				//Draw note
				note_src.x = 32 + ((note->type % 4) * 32);
				note_src.y = 0;
				note_src.w = 32;
				note_src.h = 32;
				
				note_dst.x = stage.note_x[(note->type % 8)] - FIXED_DEC(16,1);
				note_dst.y = y - FIXED_DEC(16,1);
				note_dst.w = note_src.w << FIXED_SHIFT;
				note_dst.h = note_src.h << FIXED_SHIFT;
				
				if (stage.save.downscroll)
					note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
			}
		}
	}
}

//Stage loads

static void Stage_LoadNotesPos(void)
{
	//Middle scroll
	if (stage.save.middlescroll)
	{
		for (u8 i = 0; i < 4; i++)
		{
			//BF
			stage.note_x[i ^ stage.note_swap] =  FIXED_DEC(26 + i * NOTE_SIZE - 78,1) + FIXED_DEC(SCREEN_WIDEADD,4); //+34

			//Opponent
			stage.note_x[(i + 4) ^ stage.note_swap] =  FIXED_DEC(400,1) + FIXED_DEC(SCREEN_WIDEADD,4); //-34
		}
	}

	//Normal scroll
	else
	{
		for (u8 i = 0; i < 4; i++)
		{
			//BF
			stage.note_x[i] =  FIXED_DEC(26 + i * NOTE_SIZE,1) + FIXED_DEC(SCREEN_WIDEADD,4); //+34

			//Opponent
			stage.note_x[i + 4] =  FIXED_DEC(-128 + i * NOTE_SIZE,1) + FIXED_DEC(SCREEN_WIDEADD,4); //-34
		}
	}
	
	//Note Y
	for (u8 i = 0; i < 8; i++)
	stage.note_y[i] = FIXED_DEC(NOTE_SIZE - SCREEN_HEIGHT2, 1);
}
static void Stage_LoadPlayer(void)
{
	//Load player character
	Character_Free(stage.player);
	if (stage.stage_def->pchar.new != NULL)
	stage.player = stage.stage_def->pchar.new(stage.stage_def->pchar.x, stage.stage_def->pchar.y);
	else
		stage.player = NULL;
}

static void Stage_LoadOpponent(void)
{
	//Load opponent character
	Character_Free(stage.opponent);
	if (stage.stage_def->ochar.new != NULL)
	stage.opponent = stage.stage_def->ochar.new(stage.stage_def->ochar.x, stage.stage_def->ochar.y);
	else
		stage.opponent = NULL;
}

static void Stage_LoadGirlfriend(void)
{
	//Load girlfriend character
	Character_Free(stage.gf);
	if (stage.stage_def->gchar.new != NULL)
		stage.gf = stage.stage_def->gchar.new(stage.stage_def->gchar.x, stage.stage_def->gchar.y);
	else
		stage.gf = NULL;
}

static void Stage_LoadStage(void)
{
	//Load back
	if (stage.back != NULL)
		stage.back->free(stage.back);
	stage.back = stage.stage_def->back();
}

static void GetChart_Values(Chart* chart)
{
	u8 *chart_byte = (u8*)(chart->data);

	u8* section_address = chart_byte + 8; //Get the section (skip the speed bytes (4 bytes) and section size (2 bytes))
	u16 section_size = *((u16*)(chart_byte + 4)); //Get the section size (2 bytes)
	u16 note_size = *((u16*)(chart_byte + 6)); //Get the note size (2 bytes)

	//Directly use section, notes and events pointers
	chart->sections = (Section*)section_address;
	chart->notes = (Note*)(chart_byte + section_size);
	chart->events = (Event*)(chart_byte + section_size + note_size);
}

static void Stage_LoadChart(void)
{
	//Load stage data
	char chart_path[64];

	//Use standard path convention
	sprintf(chart_path, "\\%s\\%d%c.CHT;1", stage.stage_def->week, stage.stage_def->week_song, "ENH"[stage.stage_diff]);
	
	Stage_UnloadChart(&stage.main_chart);
	Stage_UnloadChart(&stage.event_chart);

	stage.main_chart.data = IO_Read(chart_path);

	//Normal chart
	GetChart_Values(&stage.main_chart);
		

	sprintf(chart_path, "\\%s\\%dEVENT.CHT;1", stage.stage_def->week, stage.stage_def->week_song);

	if (IO_ExistFile(chart_path) == true)
	{
		stage.event_chart.data = IO_Read(chart_path);
		//Events.json chart
		GetChart_Values(&stage.event_chart);
		stage.exist_event_json = true;
	}
	else
	{
		stage.event_chart.data = NULL;
		stage.event_chart.events = NULL;
		stage.event_chart.notes = NULL;
		stage.event_chart.sections = NULL;
		stage.exist_event_json = false;
	}
	
	//Count max scores
	stage.player_state[0].max_score = 0;
	stage.player_state[1].max_score = 0;
	for (Note *note = stage.main_chart.notes; note->pos != 0xFFFF; note++)
	{
		if (note->type & (NOTE_FLAG_SUSTAIN | NOTE_FLAG_MINE))
			continue;
		if (note->type & NOTE_FLAG_OPPONENT)
			stage.player_state[1].max_score += 35;
		else
			stage.player_state[0].max_score += 35;
	}
	if (stage.mode >= StageMode_2P && stage.player_state[1].max_score > stage.player_state[0].max_score)
		stage.max_score = stage.player_state[1].max_score;
	else
		stage.max_score = stage.player_state[0].max_score;
	
	stage.main_chart.cur_section = stage.main_chart.sections;
	stage.main_chart.cur_note = stage.main_chart.notes;
	stage.main_chart.cur_event = stage.main_chart.events;

	stage.event_chart.cur_section = stage.event_chart.sections;
	stage.event_chart.cur_note = stage.event_chart.notes;
	stage.event_chart.cur_event = stage.event_chart.events;
	
	stage.speed = stage.ogspeed = *((fixed_t*)stage.main_chart.data); //Get the speed value (4 bytes)
	
	stage.step_crochet = 0;
	stage.time_base = 0;
	stage.step_base = 0;
	stage.section_base = stage.main_chart.cur_section;
	Stage_ChangeBPM(stage.main_chart.cur_section->flag & SECTION_FLAG_BPM_MASK, 0);
}

static void Stage_LoadSFX(void)
{
	//Clear Alloc for sound effect work
	Audio_ClearAlloc();

	//Load Intro Sound Effects
	static const char* intro_path[] = {
		"\\SOUNDS\\INTRO3.VAG;1",
		"\\SOUNDS\\INTRO2.VAG;1",
		"\\SOUNDS\\INTRO1.VAG;1",
		"\\SOUNDS\\INTRO0.VAG;1",
	};

	for (u8 i = 0; i < COUNT_OF(intro_path); i++)
	{
		stage.introsound[i] = Audio_LoadSFX(intro_path[i]);
	}

	//Load General Sound Effects
	const char* sfx_path[] = {
		"\\SOUNDS\\SCROLL.VAG;1",
	};

	for (u8 i = 0; i < COUNT_OF(sfx_path); i++)
		stage.sounds[i] = Audio_LoadSFX(sfx_path[i]);
}

static void Stage_LoadMusic(void)
{
	//Offset sing ends
	if (stage.player != NULL)
		stage.player->sing_end -= stage.note_scroll;

	if (stage.opponent != NULL)
		stage.opponent->sing_end -= stage.note_scroll;

	if (stage.gf != NULL)
		stage.gf->sing_end -= stage.note_scroll;
	
	//Find music file and begin seeking to it
	Audio_SeekXA_Track(stage.stage_def->track);
	
	//Initialize music state
	stage.note_scroll = FIXED_DEC(-5 * 5 * 12,1);
	stage.song_time = FIXED_DIV(stage.note_scroll, stage.step_crochet);
	stage.interp_time = 0;
	stage.interp_ms = 0;
	stage.interp_speed = 0;
	
	//Offset sing ends again
	if (stage.player != NULL)
		stage.player->sing_end += stage.note_scroll;

	if (stage.opponent != NULL)
		stage.opponent->sing_end += stage.note_scroll;
	
	if (stage.gf != NULL)
		stage.gf->sing_end += stage.note_scroll;
}

static void Stage_LoadState(void)
{
	//Initialize stage state
	stage.flag = STAGE_FLAG_VOCAL_ACTIVE;
	
	stage.gf_speed = 4;
	
	stage.state = StageState_Play;
	
	if (stage.mode == StageMode_Swap)
	{
		stage.player_state[0].character = stage.opponent;
		stage.player_state[1].character = stage.player;
	}
	else
	{
		stage.player_state[0].character = stage.player;
		stage.player_state[1].character = stage.opponent;
	}

	//Update score
	if (stage.mode != StageMode_2P)
	{
		if (stage.player_state[0].score > (s32)stage.save.savescore[stage.stage_id][stage.stage_diff] && stage.save.botplay == false)
				stage.save.savescore[stage.stage_id][stage.stage_diff] = stage.player_state[0].score;
	}

	for (int i = 0; i < 2; i++)
	{
		memset(stage.player_state[i].arrow_hitan, 0, sizeof(stage.player_state[i].arrow_hitan));
		
		stage.player_state[i].health = 10000;
		stage.player_state[i].combo = 0;
		
		stage.player_state[i].refresh_score = false;
		stage.player_state[i].score = 0;
		strcpy(stage.player_state[i].score_text, "Score: ?");

		stage.player_state[i].refresh_misses = false;
		stage.player_state[i].misses = 0;
		strcpy(stage.player_state[i].misses_text, "| Misses: 0");

		stage.player_state[i].refresh_accuracy = false;
		stage.player_state[i].min_accuracy = stage.player_state[i].max_accuracy = 0;
		strcpy(stage.player_state[i].accuracy_text, "| Accuracy: ?");
		
		stage.player_state[i].pad_held = stage.player_state[i].pad_press = 0;
	}
	
	Debug_Load();
	ObjectList_Free(&stage.objlist_splash);
	ObjectList_Free(&stage.objlist_fg);
	ObjectList_Free(&stage.objlist_bg);
}

//Stage functions
void Stage_Load(StageId id, StageDiff difficulty, boolean story)
{
	//Get stage definition
	stage.stage_def = &stage_defs[stage.stage_id = id];
	stage.stage_diff = difficulty;
	stage.story = story;
	stage.pixelcombo = false;

	//Check movies
	//Don't play movie if you are retrying the song
	if (stage.trans != StageTrans_Reload)
		CheckMovies();
	
	//Load stage background
	Stage_LoadStage();
	
	//Load characters
	Stage_LoadPlayer();
	Stage_LoadOpponent();
	Stage_LoadGirlfriend();

	//Load HUD textures
	Gfx_LoadTex(&stage.tex_hud0, IO_Read("\\HUD\\HUD0.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_hud1, IO_Read("\\HUD\\HUD1.TIM;1"), GFX_LOADTEX_FREE);
	Gfx_LoadTex(&stage.tex_intro, IO_Read("\\HUD\\INTRO.TIM;1"), GFX_LOADTEX_FREE);
	
	//Load stage chart
	Stage_LoadChart();
	
	//Initialize stage state
	stage.story = story;
	
	//Initialize stage according to mode
	stage.note_swap = (stage.mode == StageMode_Swap) ? NOTE_FLAG_OPPONENT : 0;

	Stage_LoadState();
	Stage_LoadNotesPos();

	//Load Fonts
	FontData_Load(&stage.font_cdr, Font_CDR, true);
	FontData_Load(&stage.font_bold, Font_Bold, false);
	
	//Initialize camera
	if ((stage.main_chart.cur_section->flag & SECTION_FLAG_OPPFOCUS && stage.opponent != NULL) || (stage.player == NULL))
		Stage_FocusCharacter(stage.opponent, FIXED_UNIT);
	else
		Stage_FocusCharacter(stage.player, FIXED_UNIT);
	stage.camera.x = stage.camera.tx;
	stage.camera.y = stage.camera.ty;
	stage.camera.zoom = stage.camera.tz;
	
	stage.bump = FIXED_UNIT;
	stage.sbump = FIXED_UNIT;

	//Load Sound effects
	Stage_LoadSFX();
	
	//Load music
	stage.note_scroll = 0;
	Stage_LoadMusic();

	//Load Psych events
	Events_Load();
	
	//Test offset
	stage.offset = 0;
}

void Stage_UnloadChart(Chart* chart)
{
	if (chart->data != NULL)
	{
		Mem_Free(chart->data);
		chart->data = NULL;
	}
}

void Stage_Unload(void)
{
	//Unload stage background
	if (stage.back != NULL)
		stage.back->free(stage.back);
	stage.back = NULL;

	//Unload gameover texture
	Mem_Free(stage.gameover_tim);
	stage.gameover_tim = NULL;
	
	//Unload stage data
	Stage_UnloadChart(&stage.main_chart);
	Stage_UnloadChart(&stage.event_chart);
	
	//Free objects
	ObjectList_Free(&stage.objlist_splash);
	ObjectList_Free(&stage.objlist_fg);
	ObjectList_Free(&stage.objlist_bg);
	
	//Free characters
	Character_Free(stage.player);
	stage.player = NULL;
	Character_Free(stage.opponent);
	stage.opponent = NULL;
	Character_Free(stage.gf);
	stage.gf = NULL;
}

static boolean Stage_NextLoad(void)
{
	u8 load = stage.stage_def->next_load;
	if (load == 0)
	{
		//Do stage loadscreen if full reload
		return false;
	}
	else
	{
		//Get stage definition
		stage.stage_def = &stage_defs[stage.stage_id = stage.stage_def->next_stage];

		//Check movies
		CheckMovies();
		
		//Load stage background
		if (load & STAGE_LOAD_STAGE)
			Stage_LoadStage();
		
		//Load characters
		if (load & STAGE_LOAD_PLAYER)
		{
			Stage_LoadPlayer();
		}
		else if (stage.player != NULL)
		{
			stage.player->x = stage.stage_def->pchar.x;
			stage.player->y = stage.stage_def->pchar.y;
		}
		if (load & STAGE_LOAD_OPPONENT)
		{
			Stage_LoadOpponent();
		}
		else if (stage.opponent != NULL)
		{
			stage.opponent->x = stage.stage_def->ochar.x;
			stage.opponent->y = stage.stage_def->ochar.y;
		}
		if (load & STAGE_LOAD_GIRLFRIEND)
		{
			Stage_LoadGirlfriend();
		}
		else if (stage.gf != NULL)
		{
			stage.gf->x = stage.stage_def->gchar.x;
			stage.gf->y = stage.stage_def->gchar.y;
		}
		
		//Load stage chart
		Stage_LoadChart();
		
		//Initialize stage state
		Stage_LoadState();

		//Load notes position
		Stage_LoadNotesPos();
		
		//Load music
		Stage_LoadMusic();

		//Load Psych events
		Events_Load();
		
		//Reset timer
		Timer_Reset();

		return true;
	}
}

void Stage_Tick(void)
{
	//Reload song when start or cross is pressed
	if (pad_state.press & PAD_START && stage.state != StageState_Play)
	{
		stage.trans = StageTrans_Reload;
		Trans_Start();
	}

	//Tick transition
	if (Trans_Tick())
	{
		stage.flag &= ~STAGE_FLAG_PAUSED;
		switch (stage.trans)
		{
			case StageTrans_Menu:
				//Load appropriate menu
				Stage_Unload();
				
				LoadScr_Start();
				if (stage.story)
					Menu_Load(MenuPage_Story);
				else
					Menu_Load(MenuPage_Freeplay);

				LoadScr_End();
				
				gameloop = GameLoop_Menu;
				return;
			case StageTrans_NextStage:
				//Load next song
				if (!Stage_NextLoad())
				{
				Stage_Unload();
				
				LoadScr_Start();
				Stage_Load(stage.stage_def->next_stage, stage.stage_diff, stage.story);
				LoadScr_End();
				}
				break;
			case StageTrans_Reload:
				//Reload song
				Stage_Unload();
				
				LoadScr_Start();
				Stage_Load(stage.stage_id, stage.stage_diff, stage.story);
				LoadScr_End();
				break;
		}
	}
	
	switch (stage.state)
	{
		case StageState_Play:
		{		
			//Get song position
			boolean playing = false;
			fixed_t next_scroll;
			
			if (!(stage.flag & STAGE_FLAG_PAUSED))
			{
				if (stage.note_scroll < 0)
				{
					//Play countdown sequence
					Stage_DrawCountdown();
					stage.song_time += timer_dt;
						
					//Update song
					if (stage.song_time >= 0)
					{
						//Song has started
						playing = true;
						Audio_PlayXA_Track(stage.stage_def->track, 0x40, stage.stage_def->channel, false);
							
						//Update song time
						fixed_t audio_time = (fixed_t)Audio_TellXA_Milli() - stage.offset;
						if (audio_time < 0)
							audio_time = 0;
						stage.interp_ms = (audio_time << FIXED_SHIFT) / 1000;
						stage.interp_time = 0;
						stage.song_time = stage.interp_ms;
					}
					else
					{
						//Still scrolling
						playing = false;
					}
						
						//Update scroll
						next_scroll = FIXED_MUL(stage.song_time, stage.step_crochet);
					}
				else if (Audio_PlayingXA())
				{
					//Sync to audio
					fixed_t audio_time_pof = (fixed_t)Audio_TellXA_Milli();
					fixed_t audio_time = (audio_time_pof > 0) ? (audio_time_pof - stage.offset) : 0;

					stage.interp_ms = (audio_time << FIXED_SHIFT) / 1000;
					stage.interp_time = 0;
					stage.song_time = stage.interp_ms;
						
					playing = true;
						
					//Update scroll
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
				}
				else
				{
					//Song has ended
					playing = false;
					stage.song_time += timer_dt;
						
					//Update scroll
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
						
					//Transition to menu or next song
					if (stage.story && stage.stage_def->next_stage != stage.stage_id)
					{
						stage.trans = StageTrans_NextStage;
						Trans_Start();
					}
					else
					{
						//Update score
						if (stage.mode != StageMode_2P)
						{
							if (stage.player_state[0].score > (s32)stage.save.savescore[stage.stage_id][stage.stage_diff] && stage.save.botplay == false)
									stage.save.savescore[stage.stage_id][stage.stage_diff] = stage.player_state[0].score;
						}
						stage.trans = StageTrans_Menu;
						Trans_Start();
					}
				}

			//Clear per-frame flags
			stage.flag &= ~(STAGE_FLAG_JUST_STEP);
			
			RecalcScroll:;
			//Update song scroll and step
			if (next_scroll > stage.note_scroll)
			{
				if (((stage.note_scroll / 12) & FIXED_UAND) != ((next_scroll / 12) & FIXED_UAND))
					stage.flag |= STAGE_FLAG_JUST_STEP;
				stage.note_scroll = next_scroll;
				stage.song_step = (stage.note_scroll >> FIXED_SHIFT);
				if (stage.note_scroll < 0)
					stage.song_step -= 11;
				stage.song_step /= 12;
				stage.song_beat = stage.song_step / 4;
			}
			
			//Update section
			if (stage.note_scroll >= 0)
			{
				//Check if current section has ended
				u16 end = stage.main_chart.cur_section->end;
				if ((stage.note_scroll >> FIXED_SHIFT) >= end)
				{
					//Increment section pointer
					stage.main_chart.cur_section++;
					
					//Update BPM
					u16 next_bpm = stage.main_chart.cur_section->flag & SECTION_FLAG_BPM_MASK;
					Stage_ChangeBPM(next_bpm, end);
					stage.section_base = stage.main_chart.cur_section;
					
					//Recalculate scroll based off new BPM
					next_scroll = ((fixed_t)stage.step_base << FIXED_SHIFT) + FIXED_MUL(stage.song_time - stage.time_base, stage.step_crochet);
					goto RecalcScroll;
				}
			}
		}

			//Go to pause state
			if (stage.flag & STAGE_FLAG_PAUSED)
			{
				Stage_DrawPause();
			}

			if (playing && pad_state.press & PAD_START)
			{
				Audio_PauseXA();
				//Initialize pause variables
				stage.flag |= STAGE_FLAG_PAUSED;
			  stage.pause_scroll = -1;
			  stage.pause_select = 0;
  		}

  		//Psych events
  		Events_StartEvents();

			//Tick stage
			if (stage.back->tick != NULL)
				stage.back->tick(stage.back);
			
			//Handle bump
			if ((stage.flag & STAGE_FLAG_PAUSED) == false)
			{
				if ((stage.bump = FIXED_UNIT + FIXED_MUL(stage.bump - FIXED_UNIT, FIXED_DEC(95,100))) <= FIXED_DEC(1003,1000))
					stage.bump = FIXED_UNIT;
				stage.sbump = FIXED_UNIT + FIXED_MUL(stage.sbump - FIXED_UNIT, FIXED_DEC(85,100));

				if ((stage.character_bump = FIXED_UNIT + FIXED_MUL(stage.character_bump - FIXED_UNIT, FIXED_DEC(95,100))) <= FIXED_DEC(1003,1000))
					stage.character_bump = FIXED_UNIT;
				
				if (playing && (stage.flag & STAGE_FLAG_JUST_STEP))
				{
					//Check if screen should bump
					boolean is_bump_step = (stage.song_step % 16) == 0;
					
					//Bump screen
					if (is_bump_step && stage.save.canbump == true)
					{
						stage.bump += FIXED_DEC(3,100); //0.03
						stage.character_bump += FIXED_DEC(15,1000); //0.015
					}
					
					//Bump health every 4 steps
					if ((stage.song_step % 4) == 0)
						stage.sbump += FIXED_DEC(3,100);
				}
			}
			
			//Scroll camera
			if ((stage.main_chart.cur_section->flag & SECTION_FLAG_OPPFOCUS && stage.opponent != NULL) || (stage.player == NULL))
				Stage_FocusCharacter(stage.opponent, FIXED_DEC(1,1) / 24);
			else
				Stage_FocusCharacter(stage.player, FIXED_DEC(1,1) / 24);

			Stage_ScrollCamera();
			
			switch (stage.mode)
			{
				case StageMode_Normal:
				case StageMode_Swap:
				{
					//Handle player 1 inputs
					Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
					
					for (Note *note = stage.main_chart.cur_note;; note++)
					{
						if (note->pos > (stage.note_scroll >> FIXED_SHIFT))
							break;
						
						//Opponent note hits
						if (playing && ((note->type ^ stage.note_swap) & NOTE_FLAG_OPPONENT) && !(note->type & NOTE_FLAG_HIT))
						{
							//Opponent hits note
							stage.player_state[1].arrow_hitan[note->type % 4] = stage.step_time; //Opponent strum notes light up
							Stage_StartVocal();

							//Make character sing if he exist
							if (stage.player_state[1].character != NULL)
								stage.player_state[1].character->set_anim(stage.player_state[1].character, note_anims[note->type % 4][(note->type & NOTE_FLAG_ALT_ANIM) != 0]);

							note->type |= NOTE_FLAG_HIT;
						}
					}
					break;
				}
				case StageMode_2P:
				{
					//Handle player 1 and 2 inputs
					Stage_ProcessPlayer(&stage.player_state[0], &pad_state, playing);
					Stage_ProcessPlayer(&stage.player_state[1], &pad_state_2, playing);
					break;
				}
			}
			
			//Tick note splashes
			ObjectList_Tick(&stage.objlist_splash);
			
			//Draw stage notes
			Stage_DrawNotes();
			
			//Draw note HUD
			RECT note_src = {0, 0, 32, 32};
			RECT_FIXED note_dst = {0, 0, FIXED_DEC(32,1), FIXED_DEC(32,1)};
			
			for (u8 i = 0; i < 4; i++)
			{
				//BF
				note_dst.x = stage.note_x[i] - FIXED_DEC(16,1);
				note_dst.y = stage.note_y[i] - FIXED_DEC(16,1);

				if (stage.save.downscroll)
				note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawStrum(i, &note_src, &note_dst);
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
				
				//Opponent
				note_dst.x = stage.note_x[(i | 4)] - FIXED_DEC(16,1);
				note_dst.y = stage.note_y[(i | 4)] - FIXED_DEC(16,1);

				if (stage.save.downscroll)
				note_dst.y = -note_dst.y - note_dst.h;
				Stage_DrawStrum(i | 4, &note_src, &note_dst);
				Stage_DrawTex(&stage.tex_hud0, &note_src, &note_dst, stage.bump);
			}

			//Draw timer
			Stage_TimerDraw();
			
			//Draw score, misses and accuracy
			for (int i = 0; i < ((stage.mode >= StageMode_2P) ? 2 : 1); i++)
			{
				PlayerState *this = &stage.player_state[i];
				
				//Calculate accuracy
				if (this->max_accuracy) // Prevent division by zero
					this->accuracy = (this->min_accuracy * 100) / (this->max_accuracy);

				//Check if should update the info
				if (this->refresh_score)
				{
					if (this->score != 0)
						sprintf(this->score_text, "Score: %d0", this->score * stage.max_score / this->max_score);
					else
						strcpy(this->score_text, "Score: ?");
					this->refresh_score = false;
				}

				if (this->refresh_misses)
				{
					if (this->misses != 0)
						sprintf(this->misses_text, "| Misses: %d", this->misses);
					else
						strcpy(this->misses_text, "| Misses: 0");
					this->refresh_misses = false;
				}

				if (this->refresh_accuracy)
				{
					if (this->accuracy != 0)
						sprintf(this->accuracy_text, "| Accuracy: (%d%%)", this->accuracy);
					else
						strcpy(this->accuracy_text, "| Accuracy: ?");
					this->refresh_accuracy = false;
				}

				sprintf(this->info_text, "%s %s %s", 
					this->score_text,
					this->misses_text,
					(stage.mode != StageMode_2P) ? this->accuracy_text : "" //Don't draw accuracy in 2p mode
				);

				//Change text position x depending of the mode
				s16 textposx;

				switch(stage.mode)
				{
					case StageMode_2P: //Multiplayer mode
						if (i == 0) //BF
							textposx = 110;
						else
							textposx = -50; //Opponent
					break;
					default:
						textposx = 40; //Normal and swap mode
					break;
				}
				
				//Display info
				stage.font_cdr.draw(&stage.font_cdr,
						this->info_text,
						textposx,
						(stage.save.downscroll) ? -SCREEN_HEIGHT2 + 18 + (9*2) : SCREEN_HEIGHT2 - 18,
						FontAlign_Center
					);
			}
			
			if (stage.mode < StageMode_2P)
			{
				//Perform health checks
				if (stage.player_state[0].health <= 0)
				{
					//Player has died
					stage.player_state[0].health = 0;
					stage.state = StageState_Dead;
				}
				if (stage.player_state[0].health > 20000)
					stage.player_state[0].health = 20000;
				
				//Draw health heads
				Stage_DrawHealthIcon(stage.player_state[0].health, stage.player_state[0].character,  1);
				Stage_DrawHealthIcon(stage.player_state[0].health, stage.player_state[1].character, -1);
				
				//Draw health bar
				Stage_DrawHealthBar(stage.player_state[1].character->health_b, -1);
				Stage_DrawHealthBar(stage.player_state[0].character->health_b,  1);
			}
			
			//Draw stage foreground
			if (stage.back->draw_fg != NULL)
				stage.back->draw_fg(stage.back);
			
			//Tick foreground objects
			ObjectList_Tick(&stage.objlist_fg);
			
			//Tick characters
			if (stage.player != NULL)
				stage.player->tick(stage.player);
			if (stage.opponent != NULL)
				stage.opponent->tick(stage.opponent);
			
			//Draw stage middle
			if (stage.back->draw_md != NULL)
				stage.back->draw_md(stage.back);
			
			//Tick girlfriend
			if (stage.gf != NULL)
				stage.gf->tick(stage.gf);
			
			//Tick background objects
			ObjectList_Tick(&stage.objlist_bg);
			
			//Draw stage background
			if (stage.back->draw_bg != NULL)
				stage.back->draw_bg(stage.back);
			break;
		}
		case StageState_Dead: //Start BREAK animation and reading extra data from CD
		{
			//Stop music immediately
			Audio_StopXA();
			
			//Unload stage data
			Stage_UnloadChart(&stage.main_chart);
			Stage_UnloadChart(&stage.event_chart);
			
			//Free background
			stage.back->free(stage.back);
			stage.back = NULL;
			
			//Free objects
			ObjectList_Free(&stage.objlist_fg);
			ObjectList_Free(&stage.objlist_bg);
			
			//Free opponent and girlfriend
			Character_Free(stage.opponent);
			stage.opponent = NULL;
			Character_Free(stage.gf);
			stage.gf = NULL;
			
			//Reset stage state
			stage.flag = 0;
			stage.bump = stage.sbump = FIXED_UNIT;
			
			//Change background colour to black
			Gfx_SetClear(0, 0, 0);
		
			Stage_FocusCharacter(stage.player, 0);
			stage.song_time = 0;

			//Load Gameover texture
			Gfx_LoadTex(&stage.tex_gameover, stage.gameover_tim, GFX_LOADTEX_FREE);

			//Play Gameover music
			Audio_PlayXA_Track(XA_GameOver, 0x40, 0, true);
			
			stage.state = StageState_DeadLoop;
			break;
		}
		//Fallthrough
		case StageState_DeadLoop:
		{
			RECT gameover_src = {0, 0, 255, 255};
			RECT_FIXED gameover_dst = {
				FIXED_DEC(-128,1),
				FIXED_DEC(-120,1),
				FIXED_DEC(255,1),
				FIXED_DEC(255,1)
			};
			Stage_DrawTex(&stage.tex_gameover, &gameover_src, &gameover_dst, stage.character_bump);
			break;
		}
	}
}