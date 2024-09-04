/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifndef PSXF_GUARD_AUDIO_H
#define PSXF_GUARD_AUDIO_H

#include "psx.h"

#include "fixed.h"

typedef u32 sound_t;

//Audio interface
void Audio_Init(void);
void Audio_Quit(void);
void Audio_LoadMusFile(CdlFILE *file);
void Audio_LoadMus(const char *path);
void Audio_PlayMus(boolean loops);
void Audio_StopMus(void);
void Audio_ResumeMus(void);
void Audio_PauseMus(void);
void Audio_SetVolume(u8 i, u16 vol_left, u16 vol_right);
fixed_t Audio_GetTime(void);
u32 Audio_GetLength(void);
boolean Audio_IsPlaying(void);
boolean Audio_IsPaused(void);
void Audio_ProcessMusic(void);

void Audio_ClearAlloc(void);
sound_t Audio_LoadSound(const char* path);
void Audio_PlaySound(sound_t sound, u16 volume);

#endif
