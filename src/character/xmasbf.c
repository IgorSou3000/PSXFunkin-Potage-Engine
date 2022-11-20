/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "xmasbf.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "stage/stage.h"
#include "psx/random.h"
#include "psx/main.h"

//Boyfriend player types
enum
{
	XmasBF_ArcMain_XmasBF0,
	XmasBF_ArcMain_XmasBF1,
	XmasBF_ArcMain_XmasBF2,
	XmasBF_ArcMain_XmasBF3,
	XmasBF_ArcMain_XmasBF4,
	XmasBF_ArcMain_XmasBF5,
	XmasBF_ArcMain_Dead0, //BREAK
	XmasBF_ArcMain_Dead1, //Mic Drop
	XmasBF_ArcMain_Dead2, //Mic Drop
	
	XmasBF_ArcMain_Max,
};

enum
{
	XmasBF_ArcDead_Dead3, //Twitch
	XmasBF_ArcDead_Dead4, //Confirm
	XmasBF_ArcDead_Dead5, //Confirm
	
	XmasBF_ArcDead_Max,
};

#define XmasBF_Arc_Max XmasBF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_dead;
	CdlFILE file_dead_arc; //dead.arc file position
	IO_Data arc_ptr[XmasBF_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;

} Char_XmasBF;

//Boyfriend player definitions
static const CharFrame char_xmasbf_frame[] = {
	{XmasBF_ArcMain_XmasBF0, {  0,   0, 102,  99}, { 53,  92}}, //0 idle 1
	{XmasBF_ArcMain_XmasBF0, {103,   0, 102,  99}, { 53,  92}}, //1 idle 2
	{XmasBF_ArcMain_XmasBF0, {  0, 100, 102, 101}, { 53,  94}}, //2 idle 3
	{XmasBF_ArcMain_XmasBF0, {103, 100, 103, 104}, { 53,  97}}, //3 idle 4
	{XmasBF_ArcMain_XmasBF1, {  0,   0, 103, 104}, { 53,  97}}, //4 idle 5
	
	{XmasBF_ArcMain_XmasBF1, {104,   0,  96, 102}, { 56,  95}}, //5 left 1
	{XmasBF_ArcMain_XmasBF1, {  0, 105,  94, 102}, { 54,  95}}, //6 left 2
	
	{XmasBF_ArcMain_XmasBF1, { 95, 103,  94,  89}, { 52,  82}}, //7 down 1
	{XmasBF_ArcMain_XmasBF2, {  0,   0,  94,  90}, { 52,  83}}, //8 down 2
	
	{XmasBF_ArcMain_XmasBF2, { 95,   0,  93, 112}, { 41, 104}}, //9 up 1
	{XmasBF_ArcMain_XmasBF2, {  0,  91,  94, 111}, { 42, 103}}, //10 up 2
	
	{XmasBF_ArcMain_XmasBF2, { 95, 113, 102, 102}, { 41,  95}}, //11 right 1
	{XmasBF_ArcMain_XmasBF3, {  0,   0, 102, 102}, { 41,  95}}, //12 right 2
	
	{XmasBF_ArcMain_XmasBF4, {  0,   0,  93, 108}, { 52, 101}}, //13 left miss 1
	{XmasBF_ArcMain_XmasBF4, { 94,   0,  93, 108}, { 52, 101}}, //14 left miss 2
	
	{XmasBF_ArcMain_XmasBF4, {  0, 109,  95,  98}, { 50,  90}}, //15 down miss 1
	{XmasBF_ArcMain_XmasBF4, { 96, 109,  95,  97}, { 50,  89}}, //16 down miss 2
	
	{XmasBF_ArcMain_XmasBF5, {  0,   0,  90, 107}, { 44,  99}}, //17 up miss 1
	{XmasBF_ArcMain_XmasBF5, { 91,   0,  89, 108}, { 44, 100}}, //18 up miss 2
	
	{XmasBF_ArcMain_XmasBF5, {  0, 108,  99, 108}, { 42, 101}}, //19 right miss 1
	{XmasBF_ArcMain_XmasBF5, {100, 109, 101, 108}, { 43, 101}}, //20 right miss 2

	{XmasBF_ArcMain_Dead0, {  0,  0,114,114},{ 59,102}}, //21 dead0 1
	{XmasBF_ArcMain_Dead0, {116,  0,114,114},{ 60,101}}, //22 dead0 2
	{XmasBF_ArcMain_Dead0, {  0,116,114,114},{ 60,104}}, //23 dead0 3
	{XmasBF_ArcMain_Dead0, {116,116,114,114},{ 63,104}}, //24 dead0 4

	{XmasBF_ArcMain_Dead1, {  0,  0,114,114},{ 59,101}}, //25 dead1 1
	{XmasBF_ArcMain_Dead1, {116,  0,114,114},{ 60,101}}, //26 dead1 2
	{XmasBF_ArcMain_Dead1, {  0,122,114,114},{ 62,100}}, //27 dead1 3
	{XmasBF_ArcMain_Dead1, {122,122,114,114},{ 61,100}}, //28 dead1 4

	{XmasBF_ArcMain_Dead2, {  0,  0,114,114},{ 59,101}}, //29 dead2 1
	{XmasBF_ArcMain_Dead2, {116,  0,114,114},{ 60,101}}, //30 dead2 2
	{XmasBF_ArcMain_Dead2, {  0,122,114,114},{ 62,100}}, //31 dead2 3
	{XmasBF_ArcMain_Dead2, {122,122,114,114},{ 61,100}}, //32 dead2 4

	{XmasBF_ArcDead_Dead3, {  0,  0,114,114},{ 59,101}}, //33 dead3 1
	{XmasBF_ArcDead_Dead3, {116,  0,114,114},{ 60,101}}, //34 dead3 2
	{XmasBF_ArcDead_Dead3, {  0,122,114,114},{ 62,100}}, //35 dead3 3
	{XmasBF_ArcDead_Dead3, {122,122,114,114},{ 61,100}}, //36 dead3 4

	{XmasBF_ArcDead_Dead4, {  0,  0,114,118},{ 61,104}}, //37 dead4 1
	{XmasBF_ArcDead_Dead4, {122,  0,114,118},{ 60,104}}, //38 dead4 2
	{XmasBF_ArcDead_Dead4, {  0,124,114,118},{ 60,104}}, //39 dead4 3
	{XmasBF_ArcDead_Dead4, {122,124,114,118},{ 59,104}}, //40 dead4 4

	{XmasBF_ArcDead_Dead5, {  0,  0,114,118},{ 61,104}}, //41 dead5 1
	{XmasBF_ArcDead_Dead5, {122,  0,114,118},{ 60,104}}, //42 dead5 2
	{XmasBF_ArcDead_Dead5, {  0,124,114,118},{ 60,104}}, //43 dead5 3
	{XmasBF_ArcDead_Dead5, {122,124,114,118},{ 59,104}}, //44 dead5 4
};

static const Animation char_xmasbf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 5, 13, 13, 14, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 7, 15, 15, 16, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 9, 17, 17, 18, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){11, 19, 19, 20, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //PlayerAnim_Peace
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},      //PlayerAnim_Sweat
	
	{4, (const u8[]){21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 32, 32, ASCR_BACK, 1}}, //PlayerAnim_FirstDead
	{2, (const u8[]){33, 34, 35, 36, 36, 36, 36, 36, 36, 36, 36, ASCR_REPEAT}},  //PlayerAnim_DeadLoop     
	{2, (const u8[]){37, 38, 39, 40, 41, 42, 43, 44, 44, 44, 44, 44, 44, 44, 44, 44, ASCR_BACK, 1}},  //PlayerAnim_DeadConfirm
};

//Boyfriend player functions
void Char_XmasBF_SetFrame(void *user, u8 frame)
{
	Char_XmasBF *this = (Char_XmasBF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_xmasbf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_XmasBF_Tick(Character *character)
{
	Char_XmasBF *this = (Char_XmasBF*)character;
	
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
			(stage.song_step & 0x7) == 0)
			character->set_anim(character, CharAnim_Idle);
		
		//Stage specific animations
		if (stage.note_scroll >= 0)
		{
			switch (stage.stage_id)
			{
				case StageId_1_4: //Tutorial peace
					if (stage.song_step > 64 && stage.song_step < 192 && (stage.song_step & 0x3F) == 60)
						character->set_anim(character, PlayerAnim_Peace);
					break;
				case StageId_1_1: //Bopeebo peace
					if ((stage.song_step & 0x1F) == 28)
						character->set_anim(character, PlayerAnim_Peace);
					break;
				default:
					break;
			}
		}
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_XmasBF_SetFrame);
	Character_Draw(character, &this->tex, &char_xmasbf_frame[this->frame]);
}

void Char_XmasBF_SetAnim(Character *character, u8 anim)
{
	Char_XmasBF *this = (Char_XmasBF*)character;
	
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
				"dead4.tim", //BF_ArcDead_Dead4
				"dead5.tim", //BF_ArcDead_Dead5
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

void Char_XmasBF_Free(Character *character)
{
	Char_XmasBF *this = (Char_XmasBF*)character;
	
	//Free art
	Mem_Free(this->arc_main);
	Mem_Free(this->arc_dead);
}

Character *Char_XmasBF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_XmasBF *this = Mem_Alloc(sizeof(Char_XmasBF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_XmasBF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_XmasBF_Tick;
	this->character.set_anim = Char_XmasBF_SetAnim;
	this->character.free = Char_XmasBF_Free;
	
	Animatable_Init(&this->character.animatable, char_xmasbf_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = CHAR_SPEC_MISSANIM;
	
	//Health Icon
	this->character.health_i = 0;

	//Health Bar
	this->character.health_b = 0xFF28B0D1;

	//Character scale
	this->character.scale = FIXED_DEC(1,1);
	
	this->character.focus_x = FIXED_DEC(-50,1);
	this->character.focus_y = FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\XMASBF.ARC;1");
	this->arc_dead = NULL;
	IO_FindFile(&this->file_dead_arc, "\\CHAR\\BFDEAD.ARC;1");
	
	const char **pathp = (const char *[]){
		"bf0.tim",   //XmasBF_ArcMain_XmasBF0
		"bf1.tim",   //XmasBF_ArcMain_XmasBF1
		"bf2.tim",   //XmasBF_ArcMain_XmasBF2
		"bf3.tim",   //XmasBF_ArcMain_XmasBF3
		"bf4.tim",   //XmasBF_ArcMain_XmasBF4
		"bf5.tim",   //XmasBF_ArcMain_XmasBF5
		"dead0.tim", //XmasBF_ArcMain_Dead0
		"dead1.tim", //XmasBF_ArcMain_Dead1
		"dead2.tim", //XmasBF_ArcMain_Dead2
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
