/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_DEBUG_H
#define PSXF_GUARD_DEBUG_H

#include "psx/io.h"
#include "psx/gfx.h"
#include "psx/fixed.h"

#ifdef DEBUG_MODE
//Debug definitions
typedef struct
{
	u8 mode, next_mode;
	u8 select;
	RECT_FIXED ogpositions[10];
	RECT_FIXED positions[10];
	char tex_names[15][10];
} Debug;

extern Debug debug;
#endif

void Debug_Load(void);
void Debug_MoveTexture(RECT_FIXED* src, u8 select, const char* name, fixed_t camera_x, fixed_t camera_y);
void Debug_Tick(void);

#endif
