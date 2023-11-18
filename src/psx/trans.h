/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "psx.h"

//Transition functions
void Trans_Set(void);
void Trans_Clear(void);
void Trans_Start(void);
boolean Trans_Tick(void);
boolean Trans_Idle(void);
