/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "picospeaker.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "stage/stage.h"
#include "psx/main.h"
#include "psx/random.h"

#include "speaker.h"

//Pico Speaker character structure
enum
{
	PicoSpeaker_ArcMain_LeftA0,
	PicoSpeaker_ArcMain_LeftA1,
	PicoSpeaker_ArcMain_LeftA2,
	PicoSpeaker_ArcMain_LeftA3,
	PicoSpeaker_ArcMain_LeftA4,
	PicoSpeaker_ArcMain_LeftB0,
	PicoSpeaker_ArcMain_LeftB1,
	PicoSpeaker_ArcMain_LeftB2,
	PicoSpeaker_ArcMain_RightA0,
	PicoSpeaker_ArcMain_RightA1,
	PicoSpeaker_ArcMain_RightA2,
	PicoSpeaker_ArcMain_RightA3,
	PicoSpeaker_ArcMain_RightA4,
	PicoSpeaker_ArcMain_RightB0,
	PicoSpeaker_ArcMain_RightB1,
	PicoSpeaker_ArcMain_RightB2,
	
	PicoSpeaker_Arc_Max,
};

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data arc_ptr[PicoSpeaker_Arc_Max];
	
	Gfx_Tex tex;
	u8 frame, tex_id;

	//Chart stuff
	IO_Data chart_data;
	Note* notes;

	//Speaker
	Speaker speaker;
} Char_PicoSpeaker;

//Pico Speaker character definitions
static const CharFrame char_picospeaker_frame[] = {
	{PicoSpeaker_ArcMain_LeftA0,{  0,  0,180,108},{  2 + 35,  2 + 92}}, //0 LeftA 1
	{PicoSpeaker_ArcMain_LeftA0,{  0,109,180,108},{  6 + 35,  1 + 92}}, //1 LeftA 2
	{PicoSpeaker_ArcMain_LeftA1,{  0,  0,248,143},{ 23 + 35, 25 + 92}}, //2 LeftA 3
	{PicoSpeaker_ArcMain_LeftA2,{  0,  0,255,164},{  7 + 35, 24 + 92}}, //3 LeftA 4
	{PicoSpeaker_ArcMain_LeftA3,{  0,  0,140,110},{  2 + 35,  2 + 92}},	//4 LeftA 5
	{PicoSpeaker_ArcMain_LeftA3,{  0,125,142,110},{  2 + 35,  5 + 92}},	//5 LeftA 6
	{PicoSpeaker_ArcMain_LeftA4,{  0,  0,148,119},{  4 + 35, 14 + 92}},	//6 LeftA 7

	{PicoSpeaker_ArcMain_LeftB0,{  0,  0,180, 94},{ 11 + 35, -13 + 92}}, //7 LeftB 1
	{PicoSpeaker_ArcMain_LeftB0,{  0, 96,180, 94},{  9 + 35, -12 + 92}}, //8 LeftB 2
	{PicoSpeaker_ArcMain_LeftB1,{  0,  0,255,110},{  9 + 35, -11 + 92}}, //9 LeftB 3
	{PicoSpeaker_ArcMain_LeftB1,{  0,105,126,110},{  4 + 35,   4 + 92}}, //10 LeftB 4
	{PicoSpeaker_ArcMain_LeftB1,{128,105,126,110},{  3 + 35,   4 + 92}}, //11 LeftB 5
	{PicoSpeaker_ArcMain_LeftB2,{  0,  0,136,112},{ 12 + 35,   4 + 92}}, //12 LeftB 6

	{PicoSpeaker_ArcMain_RightA0,{  0,  0,176, 98},{ 74 + 35, -10 + 92}}, //13 RightA 1
	{PicoSpeaker_ArcMain_RightA0,{  0, 96,176, 98},{ 73 + 35, -10 + 92}}, //14 RightA 2
	{PicoSpeaker_ArcMain_RightA1,{  0,  0,214,115},{107 + 35, -11 + 92}}, //15 RightA 3
	{PicoSpeaker_ArcMain_RightA2,{  0,  0,250,192},{149 + 35,  12 + 92}}, //16 RightA 4
	{PicoSpeaker_ArcMain_RightA3,{  0,  0,144, 91},{ 34 + 35, -14 + 92}}, //17 RightA 5
	{PicoSpeaker_ArcMain_RightA3,{  0, 90,144, 91},{ 35 + 35, -15 + 92}}, //18 RightA 6
	{PicoSpeaker_ArcMain_RightA4,{  0,  0,156,104},{ 46 + 35,  -6 + 92}}, //19 RightA 7

	{PicoSpeaker_ArcMain_RightB0,{  0,  0,190,102},{ 92 + 35,  -5 + 92}}, //20 RightB 1
	{PicoSpeaker_ArcMain_RightB0,{  0,102,190,102},{ 94 + 35, -11 + 92}}, //21 RightB 2
	{PicoSpeaker_ArcMain_RightB1,{  0,  0,232,132},{134 + 35,  13 + 92}}, //22 RightB 3
	{PicoSpeaker_ArcMain_RightB1,{  0,135,232,114},{ 80 + 35,   9 + 92}}, //23 RightB 4
	{PicoSpeaker_ArcMain_RightB2,{  0,  0,194,114},{ 86 + 35,   7 + 92}}, //24 RightB 5
	{PicoSpeaker_ArcMain_RightB2,{  0,122,194,114},{ 86 + 35,   8 + 92}}, //25 RightB 6
};

static const Animation char_picospeaker_anim[CharAnim_Max] = {
	{1, (const u8[]){ASCR_CHGANI, CharAnim_Left}}, //CharAnim_Idle
	{1, (const u8[]){ 0,  1,  2,  3,  4,  5,  6, ASCR_BACK, 3}},  //CharAnim_Left
	{1, (const u8[]){ 7,  8,  9, 10, 11, 12, ASCR_BACK, 3}},   		//CharAnim_LeftAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},       							//CharAnim_Down
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   							//CharAnim_DownAlt
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},         							//CharAnim_Up
	{0, (const u8[]){ASCR_CHGANI, CharAnim_Idle}},   							//CharAnim_UpAlt
	{1, (const u8[]){13, 14, 15, 16, 17, 18, 19, ASCR_BACK, 3}},  //CharAnim_Right
	{1, (const u8[]){20, 21, 22, 23, 24, 25, ASCR_BACK, 3}},   		//CharAnim_RightAlt
};

//Pico Speaker character functions
void Char_PicoSpeaker_SetFrame(void *user, u8 frame)
{
	Char_PicoSpeaker *this = (Char_PicoSpeaker*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &char_picospeaker_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_PicoSpeaker_Tick(Character *character)
{
	Char_PicoSpeaker *this = (Char_PicoSpeaker*)character;

	//Handle pico notes
	for (Note *note = this->notes;; note++)
	{
		if (note->pos > (stage.note_scroll / FIXED_UNIT))
			break;
						
		//Pico note hits
		if (!(note->type & NOTE_FLAG_HIT))
		{
			//Play alt animation randomly
			u8 altanim = RandomRange(0, 1);
			note->type |= NOTE_FLAG_HIT;

			//Start with the the number 1 for work perfectly
			character->set_anim(character, ((note->type % 0x4) * 2 + altanim) + 1);

			//Bump speakers
			Speaker_Bump(&this->speaker);
		}
	}
	
	//Animate and draw
	Animatable_Animate(&character->animatable, (void*)this, Char_PicoSpeaker_SetFrame);
	Character_Draw(character, &this->tex, &char_picospeaker_frame[this->frame]);

	//Tick speakers
	Speaker_Tick(&this->speaker, character->x, character->y);
}

void Char_PicoSpeaker_SetAnim(Character *character, u8 anim)
{
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_PicoSpeaker_Free(Character *character)
{
	Char_PicoSpeaker *this = (Char_PicoSpeaker*)character;

	//Free chart
	Mem_Free(this->chart_data);
	
	//Free art
	Mem_Free(this->arc_main);
}

Character *Char_PicoSpeaker_New(fixed_t x, fixed_t y)
{
	//Allocate picospeaker object
	Char_PicoSpeaker *this = Mem_Alloc(sizeof(Char_PicoSpeaker));
	if (this == NULL)
	{
		sprintf(error_msg, "[Char_PicoSpeaker_New] Failed to allocate picospeaker object");
		ErrorLock();
		return NULL;
	}
	
	//Initialize character
	this->character.tick = Char_PicoSpeaker_Tick;
	this->character.set_anim = Char_PicoSpeaker_SetAnim;
	this->character.free = Char_PicoSpeaker_Free;
	
	Animatable_Init(&this->character.animatable, char_picospeaker_anim);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	this->character.spec = 0;
	
	//Health Icon
	this->character.health_i = 4;

	//Health Bar
	this->character.health_b = 0xFFB7D753;

	//Character scale
	this->character.scale = FIXED_DEC(1,1);
	
	this->character.focus_x = FIXED_DEC(65,1);
	this->character.focus_y = FIXED_DEC(-65,1);
	this->character.focus_zoom = FIXED_DEC(1,1);

	//Load pico chart
	this->chart_data = IO_Read("\\WEEK7\\7.3P.CHT;1");

	u8* chart_byte = (u8*)this->chart_data;

	u16 section_size = *((u16*)(chart_byte + 4)); //Get the section size (skip the speed bytes (4 bytes))
	
	//Directly use notes pointers
	this->notes = (Note*)(chart_byte + section_size);
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\PICOW7.ARC;1");
	
	const char **pathp = (const char *[]){
		"lefta0.tim", //Pico_ArcMain_LeftA0
		"lefta1.tim", //Pico_ArcMain_LeftA1
		"lefta2.tim", //Pico_ArcMain_LeftA2
		"lefta3.tim", //Pico_ArcMain_LeftA3
		"lefta4.tim", //Pico_ArcMain_LeftA4
		"leftb0.tim", //Pico_ArcMain_LeftB0
		"leftb1.tim", //Pico_ArcMain_LeftB1
		"leftb2.tim", //Pico_ArcMain_LeftB2
		"righta0.tim", //Pico_ArcMain_RightA0
		"righta1.tim", //Pico_ArcMain_RightA1
		"righta2.tim", //Pico_ArcMain_RightA2
		"righta3.tim", //Pico_ArcMain_RightA3
		"righta4.tim", //Pico_ArcMain_RightA4
		"rightb0.tim", //Pico_ArcMain_RightA0
		"rightb1.tim", //Pico_ArcMain_RightB1
		"rightb2.tim", //Pico_ArcMain_RightB2
		NULL
	};
	IO_Data *arc_ptr = this->arc_ptr;
	for (; *pathp != NULL; pathp++)
		*arc_ptr++ = Archive_Find(this->arc_main, *pathp);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;

	//Initialize speaker
	Speaker_Init(&this->speaker);
	
	return (Character*)this;
}
