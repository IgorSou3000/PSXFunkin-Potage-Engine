/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "character.h"

#include "mem.h"
#include "psx/stage.h"
#include "psx/main.h"
#include "psx/archive.h"
#include "characters/speaker.h"

//Character functions
void Character_Free(Character *this)
{
	//Check if NULL
	if (this == NULL)
		return;
	
	//Free character
	this->free(this);
	Mem_Free(this);
}

void Character_Init(Character *this, fixed_t x, fixed_t y)
{
	//Perform common character initialization
	this->x = x;
	this->y = y;
	
	this->set_anim(this, CharAnim_Idle);
	this->pad_held = 0;
	
	this->sing_end = 0;
}

void Character_DrawCol(Character *this, Gfx_Tex *tex, const CharFrame *cframe, u8 r, u8 g, u8 b)
{
	//Draw character
	fixed_t x = this->x - stage.camera.x - FIXED_MUL(FIXED_DEC(cframe->off[0],1), this->scale);
	fixed_t y = this->y - stage.camera.y - FIXED_MUL(FIXED_DEC(cframe->off[1],1), this->scale);
	
	RECT src = {cframe->src[0], cframe->src[1], cframe->src[2], cframe->src[3]};
	RECT_FIXED dst = {
		x, 
		y, 
		FIXED_MUL(src.w << FIXED_SHIFT, this->scale), 
		FIXED_MUL(src.h << FIXED_SHIFT, this->scale)
	};

	Stage_DrawTexCol(tex, &src, &dst, stage.camera.bzoom, r, g, b);
}

void Character_Draw(Character *this, Gfx_Tex *tex, const CharFrame *cframe)
{
	Character_DrawCol(this, tex, cframe, 0x80, 0x80, 0x80);
}

void Character_CheckStartSing(Character *this)
{
	//Update sing end if singing animation
	if (this->animatable.anim == CharAnim_Left ||
	    this->animatable.anim == CharAnim_LeftAlt ||
	    this->animatable.anim == CharAnim_Down ||
	    this->animatable.anim == CharAnim_DownAlt ||
	    this->animatable.anim == CharAnim_Up ||
	    this->animatable.anim == CharAnim_UpAlt ||
	    this->animatable.anim == CharAnim_Right ||
	    this->animatable.anim == CharAnim_RightAlt ||
	    ((this->spec & CHAR_SPEC_MISSANIM) &&
	    (this->animatable.anim == PlayerAnim_LeftMiss ||
	     this->animatable.anim == PlayerAnim_DownMiss ||
	     this->animatable.anim == PlayerAnim_UpMiss ||
	     this->animatable.anim == PlayerAnim_RightMiss)))
		this->sing_end = stage.note_scroll + (FIXED_DEC(12,1) << 2); //1 beat
}

void Character_CheckEndSing(Character *this)
{
	if ((this->animatable.anim == CharAnim_Left ||
	     this->animatable.anim == CharAnim_LeftAlt ||
	     this->animatable.anim == CharAnim_Down ||
	     this->animatable.anim == CharAnim_DownAlt ||
	     this->animatable.anim == CharAnim_Up ||
	     this->animatable.anim == CharAnim_UpAlt ||
	     this->animatable.anim == CharAnim_Right ||
	     this->animatable.anim == CharAnim_RightAlt ||
	    ((this->spec & CHAR_SPEC_MISSANIM) &&
	    (this->animatable.anim == PlayerAnim_LeftMiss ||
	     this->animatable.anim == PlayerAnim_DownMiss ||
	     this->animatable.anim == PlayerAnim_UpMiss ||
	     this->animatable.anim == PlayerAnim_RightMiss))) &&
	    stage.note_scroll >= this->sing_end)
		this->set_anim(this, CharAnim_Idle);
}

void Character_CheckAnimationUpdate(Character* this)
{
	//Handle animation updates
	if ((this->pad_held & (INPUT_LEFT | INPUT_DOWN | INPUT_UP | INPUT_RIGHT)) == 0 ||
	    (this->animatable.anim != CharAnim_Left &&
	     this->animatable.anim != CharAnim_LeftAlt &&
	     this->animatable.anim != CharAnim_Down &&
	     this->animatable.anim != CharAnim_DownAlt &&
	     this->animatable.anim != CharAnim_Up &&
	     this->animatable.anim != CharAnim_UpAlt &&
	     this->animatable.anim != CharAnim_Right &&
	     this->animatable.anim != CharAnim_RightAlt))
		Character_CheckEndSing(this);
}

void Character_PerformIdle(Character *this)
{
	if (Animatable_Ended(&this->animatable) &&
		(this->animatable.anim != CharAnim_Left &&
		 this->animatable.anim != CharAnim_LeftAlt &&
		 this->animatable.anim != PlayerAnim_LeftMiss &&
		 this->animatable.anim != CharAnim_Down &&
		 this->animatable.anim != CharAnim_DownAlt &&
		 this->animatable.anim != PlayerAnim_DownMiss &&
		 this->animatable.anim != CharAnim_Up &&
		 this->animatable.anim != CharAnim_UpAlt &&
		 this->animatable.anim != PlayerAnim_UpMiss &&
		 this->animatable.anim != CharAnim_Right &&
		 this->animatable.anim != CharAnim_RightAlt &&
		 this->animatable.anim != PlayerAnim_RightMiss) &&
		(stage.song_step & 7) == 0)
			this->set_anim(this, CharAnim_Idle);
}

typedef struct
{
	//Character base structure
	Character character;
	
	//Render data and state
	IO_Data arc_main;
	IO_Data* arc_ptr;
	
	Gfx_Tex tex;
	u8 frame, tex_id;

	IO_Data chr_file;

	CharFrame* char_frames;
	Animation* char_animations;

	Speaker speaker;
} CharacterData;

//Character data functions
static void CharacterData_SetFrame(void *user, u8 frame)
{
	CharacterData *this = (CharacterData*)user;
	
	//Check if this is a new frame
	if (frame != this->frame)
	{
		//Check if new art shall be loaded
		const CharFrame *cframe = &this->char_frames[this->frame = frame];
		if (cframe->tex != this->tex_id)
			Gfx_LoadTex(&this->tex, this->arc_ptr[this->tex_id = cframe->tex], 0);
	}
}

static void CharacterData_Tick(Character *character)
{
	CharacterData *this = (CharacterData*)character;
	
	if (!(character->spec & CHAR_SPEC_GFANIM))
		Character_CheckAnimationUpdate(character);
	
	if (stage.flag & STAGE_FLAG_JUST_STEP)
	{
		if (!(character->spec & CHAR_SPEC_GFANIM))
			Character_PerformIdle(character);

		else
		{
			//Perform dance
			if (stage.note_scroll >= character->sing_end && (stage.song_step % stage.gf_speed) == 0)
			{
				//Switch animation
				if (character->animatable.anim == CharAnim_LeftAlt || character->animatable.anim == CharAnim_Right)
					character->set_anim(character, CharAnim_RightAlt);
				else
					character->set_anim(character, CharAnim_LeftAlt);
					
				//Bump speakers
				Speaker_Bump(&this->speaker);
			}
		}
	}
	
	//Animate and draw character
	Animatable_Animate(&character->animatable, (void*)this, CharacterData_SetFrame);
	Character_Draw(character, &this->tex, &this->char_frames[this->frame]); 

	//Tick speakers
	if (character->spec & CHAR_SPEC_GFANIM)	
		Speaker_Tick(&this->speaker, character->x, character->y);
}

static void CharacterData_SetAnim(Character *character, u8 anim)
{
	CharacterData *this = (CharacterData*)character;
	
	//Set animation
	if (character->spec & CHAR_SPEC_GFANIM)
	{
		if (anim == CharAnim_Left || anim == CharAnim_Down || anim == CharAnim_Up || anim == CharAnim_Right || anim == CharAnim_UpAlt)
			character->sing_end = stage.note_scroll + FIXED_DEC(22,1); //Nearly 2 steps
	}

	Animatable_SetAnim(&character->animatable, anim);

	if (!(character->spec & CHAR_SPEC_GFANIM))
		Character_CheckStartSing(character);
}

static void CharacterData_Free(Character *character)
{
	CharacterData *this = (CharacterData*)character;
	
	//Free art
	Mem_Free(this->arc_ptr);
	Mem_Free(this->arc_main);
	Mem_Free(this->chr_file);
}

Character *CharacterData_New(Character* character, const char* chr_path, fixed_t x, fixed_t y)
{
	//Allocate boyfriend object
	character = Mem_Alloc(sizeof(CharacterData));
	if (character == NULL)
	{
		sprintf(error_msg, "[%s] Failed to allocate character object", chr_path);
		ErrorLock();
		return NULL;
	}

	CharacterData *this = (CharacterData*)character;
	
	this->chr_file = IO_Read(chr_path);

	//Initialize character
	character->tick = CharacterData_Tick;
	character->set_anim = CharacterData_SetAnim;
	character->free = CharacterData_Free;

	u8* chr_byte = (u8*)this->chr_file;
	character->file_header = (CharFileHeader*)(chr_byte);
	character->file = (CharFile*)(chr_byte + sizeof(CharFileHeader));

	this->char_frames = (CharFrame*)(chr_byte + character->file_header->frame_address);
	this->char_animations = (Animation*)(chr_byte + character->file_header->animation_address);

	this->arc_ptr = Mem_Alloc(sizeof(IO_Data) * character->file_header->texture_paths_size);
	
	Animatable_Init(&character->animatable, this->char_animations);
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
	this->arc_main = IO_Read(character->file->archive_path);

	char* paths = (char*)(chr_byte + sizeof(CharFile) + sizeof(CharFileHeader));

	IO_Data *arc_ptr = this->arc_ptr;
	for (; *paths != NULL; paths += 12)
		*arc_ptr++ = Archive_Find(this->arc_main, paths);
	
	//Initialize render state
	this->tex_id = this->frame = 0xFF;

	//Initialize speaker
	if (character->spec & CHAR_SPEC_GFANIM)
		Speaker_Init(&this->speaker);
	
	return (Character*)this;
}
