/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "psx/fixed.h"

typedef struct
{
	//Pause state
	boolean is_paused;
	u8 select;
  fixed_t scroll;
} Pause;

extern Pause pause;

boolean Pause_IsPaused();
void Pause_Tick();