/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bf.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "stage/stage.h"
#include "psx/random.h"
#include "psx/main.h"

//Boyfriend player types
enum
{
	BF_ArcMain_BF0,
	BF_ArcMain_BF1,
	BF_ArcMain_BF2,
	BF_ArcMain_BF3,
	BF_ArcMain_BF4,
	BF_ArcMain_BF5,
	BF_ArcMain_BF6,
	BF_ArcMain_Dead0, //BREAK
	BF_ArcMain_Dead1, //Mic Drop
	BF_ArcMain_Dead2, //Mic Drop
	
	BF_ArcMain_Max,
};

enum
{
	BF_ArcDead_Dead3, //Twitch
	BF_ArcDead_Dead4, //Confirm
	BF_ArcDead_Dead5, //Confirm
	
	BF_ArcDead_Max,
};

#define BF_Arc_Max BF_ArcMain_Max

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main, arc_Dead;
	CdlFILE file_Dead_arc; //Dead.arc file position
	IO_Data arc_ptr[BF_Arc_Max];
	
	Gfx_Tex tex, tex_retry;
	u8 frame, tex_id;
} Char_BF;

//Boyfriend player definitions
static const CharFrame char_bf_frame[] = {
	{BF_ArcMain_BF0, {  0,   0, 102,  99}, { 53,  92}}, //0 idle 1
	{BF_ArcMain_BF0, {103,   0, 102,  99}, { 53,  92}}, //1 idle 2
	{BF_ArcMain_BF0, {  0, 100, 102, 101}, { 53,  94}}, //2 idle 3
	{BF_ArcMain_BF0, {103, 100, 103, 104}, { 53,  97}}, //3 idle 4
	{BF_ArcMain_BF1, {  0,   0, 103, 104}, { 53,  97}}, //4 idle 5
	
	{BF_ArcMain_BF1, {104,   0,  96, 102}, { 56,  95}}, //5 left 1
	{BF_ArcMain_BF1, {  0, 105,  94, 102}, { 54,  95}}, //6 left 2
	
	{BF_ArcMain_BF1, { 95, 103,  94,  89}, { 52,  82}}, //7 down 1
	{BF_ArcMain_BF2, {  0,   0,  94,  90}, { 52,  83}}, //8 down 2
	
	{BF_ArcMain_BF2, { 95,   0,  93, 112}, { 41, 104}}, //9 up 1
	{BF_ArcMain_BF2, {  0,  91,  94, 111}, { 42, 103}}, //10 up 2
	
	{BF_ArcMain_BF2, { 95, 113, 102, 102}, { 41,  95}}, //11 right 1
	{BF_ArcMain_BF3, {  0,   0, 102, 102}, { 41,  95}}, //12 right 2
	
	{BF_ArcMain_BF3, {103,   0,  99, 105}, { 54,  98}}, //13 peace 1
	{BF_ArcMain_BF3, {  0, 103, 104, 103}, { 54,  96}}, //14 peace 2
	{BF_ArcMain_BF3, {105, 106, 104, 104}, { 54,  97}}, //15 peace 3
	
	{BF_ArcMain_BF4, {  0,   0, 128, 128}, { 53,  92}}, //16 sweat 1
	{BF_ArcMain_BF4, {128,   0, 128, 128}, { 53,  93}}, //17 sweat 2
	{BF_ArcMain_BF4, {  0, 128, 128, 128}, { 53,  98}}, //18 sweat 3
	{BF_ArcMain_BF4, {128, 128, 128, 128}, { 53,  98}}, //19 sweat 4
	
	{BF_ArcMain_BF5, {  0,   0,  93, 108}, { 52, 101}}, //20 left miss 1
	{BF_ArcMain_BF5, { 94,   0,  93, 108}, { 52, 101}}, //21 left miss 2
	
	{BF_ArcMain_BF5, {  0, 109,  95,  98}, { 50,  90}}, //22 down miss 1
	{BF_ArcMain_BF5, { 96, 109,  95,  97}, { 50,  89}}, //23 down miss 2
	
	{BF_ArcMain_BF6, {  0,   0,  90, 107}, { 44,  99}}, //24 up miss 1
	{BF_ArcMain_BF6, { 91,   0,  89, 108}, { 44, 100}}, //25 up miss 2
	
	{BF_ArcMain_BF6, {  0, 108,  99, 108}, { 42, 101}}, //26 right miss 1
	{BF_ArcMain_BF6, {100, 109, 101, 108}, { 43, 101}}, //27 right miss 2

	{BF_ArcMain_Dead0, {  0,  0,114,114},{ 59,102}}, //28 dead0 1
	{BF_ArcMain_Dead0, {116,  0,114,114},{ 60,101}}, //29 dead0 2
	{BF_ArcMain_Dead0, {  0,116,114,114},{ 60,104}}, //30 dead0 3
	{BF_ArcMain_Dead0, {116,116,114,114},{ 63,104}}, //31 dead0 4

	{BF_ArcMain_Dead1, {  0,  0,114,114},{ 59,101}}, //32 dead1 1
	{BF_ArcMain_Dead1, {116,  0,114,114},{ 60,101}}, //33 dead1 2
	{BF_ArcMain_Dead1, {  0,122,114,114},{ 62,100}}, //34 dead1 3
	{BF_ArcMain_Dead1, {122,122,114,114},{ 61,100}}, //35 dead1 4

	{BF_ArcMain_Dead2, {  0,  0,114,114},{ 59,101}}, //36 dead2 1
	{BF_ArcMain_Dead2, {116,  0,114,114},{ 60,101}}, //37 dead2 2
	{BF_ArcMain_Dead2, {  0,122,114,114},{ 62,100}}, //38 dead2 3
	{BF_ArcMain_Dead2, {122,122,114,114},{ 61,100}}, //39 dead2 4

	{BF_ArcDead_Dead3, {  0,  0,114,114},{ 59,101}}, //40 dead3 1
	{BF_ArcDead_Dead3, {116,  0,114,114},{ 60,101}}, //41 dead3 2
	{BF_ArcDead_Dead3, {  0,122,114,114},{ 62,100}}, //42 dead3 3
	{BF_ArcDead_Dead3, {122,122,114,114},{ 61,100}}, //43 dead3 4

	{BF_ArcDead_Dead4, {  0,  0,114,118},{ 61,104}}, //44 dead4 1
	{BF_ArcDead_Dead4, {122,  0,114,118},{ 60,104}}, //45 dead4 2
	{BF_ArcDead_Dead4, {  0,124,114,118},{ 60,104}}, //46 dead4 3
	{BF_ArcDead_Dead4, {122,124,114,118},{ 59,104}}, //47 dead4 4

	{BF_ArcDead_Dead5, {  0,  0,114,118},{ 61,104}}, //48 dead5 1
	{BF_ArcDead_Dead5, {122,  0,114,118},{ 60,104}}, //49 dead5 2
	{BF_ArcDead_Dead5, {  0,124,114,118},{ 60,104}}, //50 dead5 3
	{BF_ArcDead_Dead5, {122,124,114,118},{ 59,104}}, //51 dead5 4
};

static const Animation char_bf_anim[PlayerAnim_Max] = {
	{2, (const u8[]){ 0,  1,  2,  3,  4, ASCR_BACK, 1}}, //CharAnim_Idle
	{2, (const u8[]){ 5,  6, ASCR_BACK, 1}},             //CharAnim_Left
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_LeftAlt
	{2, (const u8[]){ 7,  8, ASCR_BACK, 1}},             //CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_DownAlt
	{2, (const u8[]){ 9, 10, ASCR_BACK, 1}},             //CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_UpAlt
	{2, (const u8[]){11, 12, ASCR_BACK, 1}},             //CharAnim_Right
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       //CharAnim_RightAlt
	
	{1, (const u8[]){ 5, 20, 20, 21, ASCR_BACK, 1}},     //PlayerAnim_LeftMiss
	{1, (const u8[]){ 7, 22, 22, 23, ASCR_BACK, 1}},     //PlayerAnim_DownMiss
	{1, (const u8[]){ 9, 24, 24, 25, ASCR_BACK, 1}},     //PlayerAnim_UpMiss
	{1, (const u8[]){11, 26, 26, 27, ASCR_BACK, 1}},     //PlayerAnim_RightMiss
	
	{2, (const u8[]){13, 14, 15, ASCR_BACK, 1}},         //PlayerAnim_Peace
	{2, (const u8[]){16, 17, 18, 19, ASCR_REPEAT}},      //PlayerAnim_Sweat
	
	{4, (const u8[]){28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 39, 39, ASCR_BACK, 1}}, //PlayerAnim_FirstDead
	{2, (const u8[]){40, 41, 42, 43, 43, 43, 43, 43, 43, 43, 43, ASCR_REPEAT}},  //PlayerAnim_DeadLoop     
	{2, (const u8[]){44, 45, 46, 47, 48, 49, 50, 51, 51, 51, 51, 51, 51, 51, 51, 51, ASCR_BACK, 1}},  //PlayerAnim_DeadConfirm                                                 //PlayerAnim_DeadLoop
};

//Boyfriend player functions
void Char_BF_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_bf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BF_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
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
	Animatable_Animate(&character->animatable, (void*)this, Char_BF_SetFrame);
	Character_Draw(character, &this->tex, &char_bf_frame[this->frame]);
}

void Char_BF_SetAnim(Character *character, u8 anim)
{
	Char_BF *this = (Char_BF*)character;
	
	//Perform animation checks
	switch (anim)
	{
		case PlayerAnim_FirstDead:
			//Begin reading Dead.arc and adjust focus
			this->arc_Dead = IO_AsyncReadFile(&this->file_Dead_arc);
			character->focus_x = FIXED_DEC(0,1);
			character->focus_y = FIXED_DEC(-40,1);
			character->focus_zoom = FIXED_DEC(125,100);
			break;
		case PlayerAnim_DeadLoop:
			//Unload main.arc
			Mem_Free(this->arc_main);
			this->arc_main = this->arc_Dead;
			this->arc_Dead = NULL;
			
			//Find Dead.arc files
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

void Char_BF_Free(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Free art
	Mem_Free(this->arc_main);
	Mem_Free(this->arc_Dead);
}

Character *Char_BF_New(fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	Char_BF *this = Mem_Alloc(sizeof(Char_BF));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_BF_New] Failed to allocate boyfriend object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_BF_Tick;
	this->character.set_anim = Char_BF_SetAnim;
	this->character.free = Char_BF_Free;
	
	Animatable_Init(&this->character.animatable, char_bf_anim);
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
	this->character.focus_y = (stage.stage_id == StageId_1_4) ? FIXED_DEC(-85,1) : FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BF.ARC;1");
	this->arc_Dead = NULL;
	IO_FindFile(&this->file_Dead_arc, "\\CHAR\\BFDEAD.ARC;1");
	
	const char **pathp = (const char *[]){
		"bf0.tim",   //BF_ArcMain_BF0
		"bf1.tim",   //BF_ArcMain_BF1
		"bf2.tim",   //BF_ArcMain_BF2
		"bf3.tim",   //BF_ArcMain_BF3
		"bf4.tim",   //BF_ArcMain_BF4
		"bf5.tim",   //BF_ArcMain_BF5
		"bf6.tim",   //BF_ArcMain_BF6
		"dead0.tim", //BF_ArcMain_Dead0
		"dead1.tim", //BF_ArcMain_Dead1
		"dead2.tim", //BF_ArcMain_Dead2
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
