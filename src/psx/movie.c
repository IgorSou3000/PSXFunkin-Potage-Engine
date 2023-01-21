/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

//This code relies upon strplay.c, which was created by Lameguy64 of Meido-Tek Productions.

/*
  Bilious leave some basic info about this code throughout the file, so it shouldn't be too hard to understand.
  Keep in mind this is a rewrite of the old movie.c, so things are a little different.

  This code assumes that any str file you give it is 320x240, so keep that in mind.
*/

#include "movie.h"

#include "strplay.h"
#include "stage/stage.h"

#define DEBUG 1

static const struct {
  const char* path; //path of the movie
  int frameCount; //frame count (you can get in MC32)
  StageId id; //Which Stage play the movie
}strmovies[] = {
  {"\\STR\\UGH.STR;1", 180, StageId_Max},
};

//Prepare to do movie
void Movie_Prep()
{
  // Reset and initialize stuff
  printf("Resetting for Movie\n");
  ResetCallback();
  CdInit();
  ResetGraph(0);
  SetGraphDebug(0);
}

void CheckMovies(void)
{
  //Check if have some movie to play
  for (u8 i = 0; i < COUNT_OF(strmovies); i++)
  {
    //Play only in story mode
    if (strmovies[i].id == stage.stage_id && stage.story)
      Movie_Play(strmovies[i].path, strmovies[i].frameCount);
  }
}

void Movie_Play(const char *path, u32 length) //Play the movie, path is the location of the str file, and length is the frame count
{
  printf("[Movie_Play] starting file %s with %d frames\n", path, length);
  Movie_Prep();
  PlayStr(320, 240, 0, 0, path, length);
}
