/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "splash.h"

#include "psx/mem.h"
#include "psx/timer.h"
#include "psx/random.h"
#include "psx/mutil.h"

//Splash object functions
boolean Obj_Splash_Tick(Object *obj)
{
	Obj_Splash *this = (Obj_Splash*)obj;
	
	//Move
	this->x += this->xsp;
	this->y += this->ysp;
	this->xsp = this->xsp * 5 / 6;
	this->ysp = this->ysp * 5 / 6;
	
	//Scale down
	fixed_t scale = FIXED_MUL(FIXED_UNIT - FIXED_MUL(this->size, this->size), FIXED_DEC(8,10));
	this->size += FIXED_UNIT / 25;
	
	//Draw plubbie
	RECT plub_src = {120 + (this->colour << 2), 224, 4, 4};
	RECT_FIXED plub_dst = {
		this->x - (scale << 2),
		this->y - (scale << 2),
		scale << 3,
		scale << 3
	};
	
	Stage_DrawTex(&stage.tex_hud0, &plub_src, &plub_dst, stage.bump);
	
	return this->size >= FIXED_UNIT;
}

void Obj_Splash_Free(Object *obj)
{
	(void)obj;
}

Obj_Splash *Obj_Splash_New(fixed_t x, fixed_t y, u8 colour)
{
	//Allocate new object
	Obj_Splash *this = (Obj_Splash*)Mem_Alloc(sizeof(Obj_Splash));
	if (this == NULL)
		return NULL;
	
	//Set object functions
	this->obj.tick = Obj_Splash_Tick;
	this->obj.free = Obj_Splash_Free;
	
	//Initialize position
	u8 angle = Random8();
	fixed_t speed = RandomRange(FIXED_DEC(35,10), FIXED_DEC(45,10));
	this->xsp = ((this->cos = MUtil_Cos(angle)) * speed) >> 8;
	this->ysp = ((this->sin = MUtil_Sin(angle)) * speed) >> 8;
	this->size = 0;
	
	this->x = x + this->xsp;
	this->y = y + this->ysp;
	
	this->colour = colour;
	
	return this;
}
