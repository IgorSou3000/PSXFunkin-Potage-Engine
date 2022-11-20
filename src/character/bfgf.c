/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bfgf.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "stage/stage.h"
#include "psx/random.h"
#include "psx/main.h"

//Boyfriend and Girlfriend player types
enum
{
	BFGF_ArcMain_Idle0,
	BFGF_ArcMain_Idle1,
	BFGF_ArcMain_Idle2,
	BFGF_ArcMain_Left,
	BFGF_ArcMain_Down,
	BFGF_ArcMain_Up,
	BFGF_ArcMain_Right,
	
	BFGF_ArcMain_Max,
};

enum
{
	BFGF_ArcDead_Dead1, //Mic Drop
	BFGF_ArcDead_Dead2, //Twitch
	BFGF_ArcDead_Retry, //Retry prompt
	
	BFGF_ArcDead_Max,
};

#define BFGF_Arc_Max BFGF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[BFGF_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
} Char_BFGF;

//Boyfriend and Girlfriend player definitions
static const CharFrame char_bfgf_frame[] = {
  {BFGF_ArcMain_Idle0,{  0,  0,228,120},{ 80 + 12, 70 + 42}}, //0 idle 1
	{BFGF_ArcMain_Idle0,{  0,119,228,120},{ 79 + 12, 60 + 42}}, //1 idle 2
	{BFGF_ArcMain_Idle1,{  0,  0,220,119},{ 80 + 12, 70 + 42}}, //2 idle 3
	{BFGF_ArcMain_Idle1,{  0,118,220,119},{ 79 + 12, 61 + 42}}, //3 idle 4
	{BFGF_ArcMain_Idle2,{  0,  0,220,119},{ 80 + 12, 70 + 42}}, //4 idle 5
	{BFGF_ArcMain_Idle2,{  0,118,220,119},{ 79 + 12, 61 + 42}}, //5 idle 6
	
	{BFGF_ArcMain_Left,{  0,  0,220,120},{ 80 + 12, 70 + 42}}, //6 left 1
	{BFGF_ArcMain_Left,{  0,120,220,120},{ 79 + 12, 63 + 42}}, //7 left 2
	
	{BFGF_ArcMain_Down,{  0,  0,220,120},{ 76 + 12, 59 + 42}}, //8 down 1
	{BFGF_ArcMain_Down,{  0,120,220,120},{ 75 + 12, 57 + 42}}, //9 down 2
	
	{BFGF_ArcMain_Up,{  0,  0,220,120},{ 78 + 12, 65 + 42}}, //10 up 1
	{BFGF_ArcMain_Up,{  0,120,220,120},{ 75 + 12, 68 + 42}}, //11 up 1
	
	{BFGF_ArcMain_Right,{  0,  0,220,120},{ 75 + 12, 68 + 42}}, //12 right 1
	{BFGF_ArcMain_Right,{  0,120,220,120},{ 69 + 12, 72 + 42}}, //13 right 2
};

static const Animation char_bfgf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4,  5, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 6,  7, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 8,  9, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){10, 11, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){12, 13, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 5, 20, 20, 21, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 7, 22, 22, 23, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 9, 24, 24, 25, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){11, 26, 26, 27, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},         //PlayerAnim_Peace
	{2, (const u8[]){16, 17, 18, 19, ASCR_REPEAT}},      //PlayerAnim_Sweat
	
	{5, (const u8[]){23, 24, 25, 26, 26, 26, 26, 26, 26, 26, ASCR_CHGANI, PlayerAnim_DeadLoop}}, //PlayerAnim_FirstDead
	{5, (const u8[]){26, ASCR_REPEAT}},   
};

//Boyfriend and Girlfriend player functions
void Char_BFGF_SetFrame(void *user, u8 frame)
{
	Char_BFGF *this = (Char_BFGF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bfgf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BFGF_Tick(Character *character)
{
	Char_BFGF *this = (Char_BFGF*)character;
	
	//Handle animation updates
	if ((character->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (character->animatable.anim != CharAnim_Left &&
	     character->animatable.anim != CharAnim_LeftAlt &&
	     character->animatable.anim != CharAnim_Down &&
	     character->animatable.anim != CharAnim_DownAlt &&
	     character->animatable.anim != CharAnim_Up &&
	     character->animatable.anim != CharAnim_UpAlt &&
	     character->animatable.anim != CharAnim_Right &&
	     character->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		//Perform idle dance
		if (Animatable_Ended(&character->animatable) &&
			(character->animatable.anim != CharAnim_Left &&
		     character->animatable.anim != CharAnim_LeftAlt &&
		     character->animatable.anim != PlayerAnim_LeftMiss &&
		     character->animatable.anim != CharAnim_Down &&
		     character->animatable.anim != CharAnim_DownAlt &&
		     character->animatable.anim != PlayerAnim_DownMiss &&
		     character->animatable.anim != CharAnim_Up &&
		     character->animatable.anim != CharAnim_UpAlt &&
		     character->animatable.anim != PlayerAnim_UpMiss &&
		     character->animatable.anim != CharAnim_Right &&
		     character->animatable.anim != CharAnim_RightAlt &&
		     character->animatable.anim != PlayerAnim_RightMiss) &&
			(stage.song_step % 8) == 0)
			character->set_anim(character, CharAnim_Idle);
	}

	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BFGF_SetFrame);
	Character_Draw(character, &this->tex, &char_bfgf_frame[this->frame]);
}

void Char_BFGF_SetAnim(Character *character, u8 anim)
{
	Char_BFGF *this = (Char_BFGF*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_FirstDead:
			//Begin reading dead.arc and adjust focus
			this->arc_dead = IO_AsyncReadFile(&this->file_dead_arc);
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_DeadLoop:
			//Unload main.arc
			Mem_Free(this->arc_main);
			this->arc_main = this->arc_dead;
			this->arc_dead = NULL;
			
			//Find dead.arc files
			const char **pathp = (const char *[]){
				"dead3.tim", //BF_ArcDead_Dead3
				NULL
			};
			IO_Data *arc_ptr = this->arc_ptr;
			for (; *pathp != NULL; pathp++)
				*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
			break;
	}
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BFGF_Free(Character *character)
{
	Char_BFGF *this = (Char_BFGF*)character;
	
	//Free art
	Mem_Free(this->arc_main);
	Mem_Free(this->arc_dead);
}

Character *Char_BFGF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend and girlfriend object
	Char_BFGF *this = Mem_Alloc(sizeof(Char_BFGF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BFGF_New] Failed to allocate boyfriend and girlfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BFGF_Tick;
	this->character.set_anim = Char_BFGF_SetAnim;
	this->character.free = Char_BFGF_Free;
	
	Animatable_Init(&this->character.animatable, char_bfgf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	//Health Icon
	this->character.health_i = 0;

	//Health Bar
	this->character.health_b = 0xFF28B0D1;

	//Character scale
	this->character.scale = FIXED_DEC(1,1);
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = (stage.stage_id == StageId_1_4) ? FIXED_DEC(-85,1) : FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BFGF.ARC;1");
	this->arc_dead = NULL;
	IO_FindFile(&this->file_dead_arc, "\\CHAR\\BFDEAD.ARC;1");
	
	const char **pathp = (const char *[]){
		"idle0.tim", //BFGF_ArcMain_Idle0
		"idle1.tim", //BFGF_ArcMain_Idle1
		"idle2.tim", //BFGF_ArcMain_Idle2
		"left.tim",  //BFGF_ArcMain_Left
		"down.tim",  //BFGF_ArcMain_Down
		"up.tim",    //BFGF_ArcMain_Up
		"right.tim", //BFGF_ArcMain_Right
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
