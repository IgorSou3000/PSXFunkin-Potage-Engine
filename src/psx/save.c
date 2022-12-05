/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//thanks to spicyjpeg for helping me understand this code

#include "save.h"

#include <libmcrd.h>
#include "mem.h"
#include "stage/stage.h"

// Pallete Offset: 0x00000014 to 0x00000033
// Image Offset:   0x00000040 to 0x000000BF


static const u8 saveIconPalette[32] = 
{
  0x00, 0x00, 0x19, 0xE3, 0x08, 0xA1, 0x94, 0xD2, 0xF5, 0x88, 0x5B, 0x85,
  0x0B, 0x99, 0x1F, 0x82, 0x2B, 0x9D, 0xBD, 0xF7, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const u8 saveIconImage[128] = 
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x01,
  0x00, 0x20, 0x22, 0x22, 0x22, 0x22, 0x11, 0x00, 0x00, 0x32, 0x33, 0x33,
  0x33, 0x13, 0x21, 0x00, 0x20, 0x43, 0x44, 0x44, 0x44, 0x11, 0x35, 0x02,
  0x32, 0x44, 0x44, 0x55, 0x15, 0x51, 0x55, 0x23, 0x46, 0x44, 0x55, 0x75,
  0x55, 0x75, 0x55, 0x65, 0x62, 0x54, 0x55, 0x55, 0x77, 0x57, 0x55, 0x28,
  0x12, 0x66, 0x56, 0x55, 0x55, 0x65, 0x66, 0x29, 0x12, 0x11, 0x61, 0x66,
  0x66, 0x16, 0x91, 0x29, 0x20, 0x11, 0x11, 0x11, 0x99, 0x11, 0x99, 0x02,
  0x00, 0x12, 0x11, 0x91, 0x19, 0x99, 0x29, 0x00, 0x00, 0x00, 0x10, 0x91,
  0x91, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

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
  memcpy(file->iconPalette, saveIconPalette, 32);
  memcpy(file->iconImage, saveIconImage, 128);
}

//If save not be founded, enable some options by default
void DefaultSettings()
{
  stage.save.ghost = true; 
  stage.save.showtimer = true;

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
  #ifndef NOSAVE
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
  #ifndef NOSAVE
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
}