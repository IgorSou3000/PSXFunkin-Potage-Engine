/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//thanks to spicyjpeg for helping me understand this code

#include "save.h"

#include <libmcrd.h>
#include "psx/mem.h"
#include "psx/main.h"
#include "psx/stage.h"
#include "psx/configuration.h"

// Pallete Offset: 0x00000014 to 0x00000033
// Image Offset:   0x00000040 to 0x000000BF

static IO_Data saveIcon_tex;

static void toShiftJIS(u8 *buffer, const char *text)
{
    int pos = 0;
    for (u32 i = 0; i < strlen(text); i++) 
    {
        u8 c = text[i];
        if (c >= '0' && c <= '9') { buffer[pos++] = 0x82; buffer[pos++] = 0x4F + c - '0'; }
        else if (c >= 'A' && c <= 'Z') { buffer[pos++] = 0x82; buffer[pos++] = 0x60 + c - 'A'; }
        else if (c >= 'a' && c <= 'z') { buffer[pos++] = 0x82; buffer[pos++] = 0x81 + c - 'a'; }
        else if (c == '(') { buffer[pos++] = 0x81; buffer[pos++] = 0x69; }
        else if (c == ')') { buffer[pos++] = 0x81; buffer[pos++] = 0x6A; }
        else /* space */ { buffer[pos++] = 0x81; buffer[pos++] = 0x40; }
    }
}

static void InitSaveFile(SaveFile *file, const char *name) 
{
  file->id = 0x4353;
  file->iconDisplayFlag = 0x11;
  file->iconBlockNum = 1;
  toShiftJIS(file->title, name);

  u8* saveIconByte = (u8*)saveIcon_tex;

  u8* saveIconPalette = (u8*)(saveIconByte + 0x14);
  u8* saveIconImage = (u8*)(saveIconByte + 0x40);

  memcpy(file->iconPalette, saveIconPalette, 32);
  memcpy(file->iconImage, saveIconImage, 128);
}

//If save not be founded, enable some options by default
void DefaultSettings()
{
  stage.save.ghost = true; 
  stage.save.showtimer = true;
  stage.save.canbump = true;
  stage.save.splash = true;

  for (int i = 0; i < StageId_Max; i++)
  {
    for (int j = 0; j < StageDiff_Max; j++)
    {
      stage.save.savescore[i][j] = 0;
    }
  }
}

boolean CheckSave(void)
{
  #ifdef SAVE
  int fd = open(SaveTitle, 0x0001);

  if (fd < 0) // file doesnt exist 
  {
    printf("Not Founded Save\n");
    return false;
  }
  else
    printf("Founded Save\n");
  return true;

  #else
    return false;
  #endif
}

boolean ReadSave()
{
  int fd = open(SaveTitle, 0x0001);
  //Check if exist any save
  if (CheckSave() == true)
  {
    SaveFile file;

    //Load Save
    if (read(fd, (void *) &file, sizeof(SaveFile)) == sizeof(SaveFile)) 
      printf("Loading Save....\n");

    else 
    {
      printf("Failed To Load Save!\n");
      return false;
    }
    memcpy((void *) &stage.save, (const void *) file.saveData, sizeof(stage.save));
    close(fd);
    return true;
  }

  return false;
}

void WriteSave()
{ 
  stage.save.debug_mode = false;
  
  #ifdef SAVE
  int fd = open(SaveTitle, 0x0002);

  if (fd < 0) // if save doesnt exist make one
  {
    printf("Creating Save...\n");
    fd =  open(SaveTitle, 0x0202 | (1 << 16));
  }

  SaveFile file;
  InitSaveFile(&file, SaveName);
  memcpy((void *) file.saveData, (const void *) &stage.save, sizeof(stage.save));
  
  if (fd >= 0) 
  {
      if (write(fd, (void *) &file, sizeof(SaveFile)) == sizeof(SaveFile)) 
        printf("Saved!\n");
    else 
      printf("Save Unexpected Error\n");  // if save doesnt exist do a error
    close(fd);
  } 
  else 
    printf("Not Managed To Save %d\n", fd);  // failed to save
  #endif
}

//Initiliaze memory card
void MCRD_Init(void)
{
  //InitPAD(0);
  InitCARD(1);
  StartCARD();
  _bu_init();
  ChangeClearPAD(0);
  saveIcon_tex =  IO_Read("\\SAVE16.TIM;1");
}