/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

/*
  If you finish your port, Remove the //
  This will disable some stuffs like debug mode option
*/

//#define RELEASE_MODE //Enable this when you finish your port

//HBASCUS-scusid somename
#define SaveTitle "bu00:BASCUS-54021funkin"
//What will be displayed in memory card
#define SaveName  "PSXFunkinPotageEngine"

#define SAVE //Enable this if you want the save system

//Game loop
typedef enum
{
	GameLoop_Menu,
	GameLoop_Stage,
} GameLoop;

extern GameLoop gameloop;

//Error handler
extern char error_msg[0x200];
void ErrorLock();
