/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_AUDIO_H
#define PSXF_GUARD_AUDIO_H

#include "psx.h"

typedef u32 SFX;

//XA enumerations
typedef enum
{
	XA_Menu,   //MENU.XA
	XA_Week1A, //WEEK1A.XA
	XA_Week1B, //WEEK1B.XA
	
	XA_Max,
} XA_File;

//Audio functions
void Audio_Init(void);
void Audio_Quit(void);
void Audio_LoadXA(const char* path);
void Audio_PlayXA_Track(u8 volume, u8 channel, boolean loop);
void Audio_PauseXA(void);
void Audio_ResumeXA(void);
void Audio_StopXA(void);
void Audio_ChannelXA(u8 channel);
s32 Audio_TellXA_Sector(void);
s32 Audio_TellXA_Milli(void);
boolean Audio_PlayingXA(void);
void Audio_WaitPlayXA(void);
void Audio_ProcessXA(void);
u16 Audio_GetLength();

void Audio_ClearAlloc(void);
u32 Audio_LoadSFX(const char* path);
void Audio_PlaySFX(SFX addr, u16 volume);

#endif
