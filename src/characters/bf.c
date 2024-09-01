/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "bf.h"

#include "psx/mem.h"
#include "psx/archive.h"
#include "psx/stage.h"
#include "psx/random.h"
#include "psx/main.h"

//Boyfriend player types
typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data* arc_ptr;
	
	Gfx_Tex tex;
	u8 frame, tex_id;

	CharFrame* bf_frame;
	Animation* bf_animation;
} Char_BF;

//Boyfriend player functions
void Char_BF_SetFrame(void *user, u8 frame)
{
	Char_BF *this = (Char_BF*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &this->bf_frame[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

void Char_BF_Tick(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	Character_CheckAnimationUpdate(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
		Character_PerformIdle(character);
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, Char_BF_SetFrame);
	Character_Draw(character, &this->tex, &this->bf_frame[this->frame]); 
}

void Char_BF_SetAnim(Character *character, u8 anim)
{
	Char_BF *this = (Char_BF*)character;
	
	//Set animation
	Animatable_SetAnim(&character->animatable, anim);
	Character_CheckStartSing(character);
}

void Char_BF_Free(Character *character)
{
	Char_BF *this = (Char_BF*)character;
	
	//Free art
	Mem_Free(this->arc_ptr);
	Mem_Free(this->arc_main);
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

	Character* character = &this->character;
	
	//Initialize character
	character->tick = Char_BF_Tick;
	character->set_anim = Char_BF_SetAnim;
	character->free = Char_BF_Free;

	IO_Data chr_file = IO_Read("\\CHAR\\BF.CHR;1");

	u8* chr_byte = (u8*)chr_file;
	character->file_header = (CharFileHeader*)(chr_byte);
	character->file = (CharFile*)(chr_byte + sizeof(CharFileHeader));

	this->bf_frame = (CharFrame*)(chr_byte + character->file_header->frame_address);
	this->bf_animation = (Animation*)(chr_byte + character->file_header->animation_address);

	this->arc_ptr = Mem_Alloc(sizeof(IO_Data) * character->file_header->texture_paths_size);
	
	Animatable_Init(&character->animatable, this->bf_animation);
	Character_Init((Character*)this, x, y);
	
	//Set character information
	character->spec = character->file->flags;

	//Health
	character->health_i = character->file->health_i;
	character->health_b = character->file->health_b;

	//Character scale
	character->scale = character->file->scale;
	
	character->focus_x = character->file->focus_x;
	character->focus_y = character->file->focus_y;
	character->focus_zoom = character->file->focus_zoom;
	
	//Load art
	this->arc_main = IO_Read("\\CHAR\\BF.ARC;1");

	//Load gameover texture
	sprintf(stage.gameover_path, "\\CHAR\\BFDEAD.TIM;1");
	stage.gameover_tim = IO_Read(stage.gameover_path);

	char* paths = (char*)(chr_byte + sizeof(CharFile) + sizeof(CharFileHeader));

	IO_Data *arc_ptr = this->arc_ptr;
	for (; *paths != NULL; paths += 12)
		*arc_ptr++ = Archive_Find(this->arc_main, paths);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;
	
	return (Character*)this;
}
