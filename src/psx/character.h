/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_CHARACTER_H
#define PSXF_GUARD_CHARACTER_H

#include "io.h"
#include "gfx.h"

#include "fixed.h"
#include "animation.h"

//Character flags
#define CHAR_FLAGS_IS_PLAYER 		(1 << 0)	//Character is the player
#define CHAR_FLAGS_MISS_ANIM 		(1 << 1)	//Has miss animations
#define CHAR_FLAGS_GF_DANCE   	(1 << 2)	//Has girlfriend dance
#define CHAR_FLAGS_SPOOKY_DANCE (1 << 3)	//Has spooky month dance

//Character enums
typedef enum
{
	CharAnim_Idle,
	CharAnim_Left,  CharAnim_LeftAlt,
	CharAnim_Down,  CharAnim_DownAlt,
	CharAnim_Up,    CharAnim_UpAlt,
	CharAnim_Right, CharAnim_RightAlt,
	CharAnim_Special1, CharAnim_Special2, CharAnim_Special3,
	
	CharAnim_Max //Max standard/shared animation
} CharAnim;

//Character structures
typedef struct
{
	u32 frame_address;
	u32 animation_address;
	u32 texture_paths_size;

} CharFileHeader;

typedef struct
{
	u8 flags;
	u8 health_i;
	u32 health_b;

	fixed_t focus_x;
	fixed_t	focus_y;
	fixed_t focus_zoom;

	fixed_t scale;

	char archive_path[32];

} CharFile;

typedef struct
{
	u16 tex;
	u16 src[4];
	s16 off[2];
} CharFrame;

typedef struct Character
{
	//Character functions
	void (*tick)(struct Character*);
	void (*set_anim)(struct Character*, u8);
	void (*free)(struct Character*);
	
	//Position
	fixed_t x, y;

	//Character file
	CharFileHeader* file_header;
	CharFile* file;
	
	//Character information
	u8 flags;
	u8 health_i;
	u32 health_b;

	fixed_t focus_x; 
	fixed_t focus_y;
	fixed_t focus_zoom;

	fixed_t scale; //Scale character
	
	//Animation state
	Animatable animatable;
	fixed_t sing_end;
	u16 pad_held;
} Character;

//Character functions
void Character_Free(Character *this);
void Character_Init(Character *this, fixed_t x, fixed_t y);
void Character_DrawCol(Character *this, Gfx_Tex *tex, const CharFrame *cframe, u8 r, u8 g, u8 b);
void Character_Draw(Character *this, Gfx_Tex *tex, const CharFrame *cframe);

void Character_CheckStartSing(Character *this);
void Character_CheckEndSing(Character *this, u8 idle_animation);
void Character_CheckAnimationUpdate(Character* this, u8 idle_animation);
void Character_PerformIdle(Character *this, boolean is_animatable_done, u8 step_speed, u8 idle_animation);

Character *CharacterData_New(Character* character, const char* chr_path, fixed_t x, fixed_t y);

#endif
