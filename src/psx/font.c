/*
  This Source Code Form is subject to the terms of the Mozilla Public
  License, v. 2.0. If a copy of the MPL was not distributed with this
  file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#include "font.h"

#include "timer.h"
#include "stage/stage.h"

#include <string.h>

static Gfx_Tex font_tex;

static void Font_DrawTex(struct FontData *this, RECT* src, s32 x, s32 y, u8 r, u8 g, u8 b, Gfx_Tex* tex)
{
	//Draw a stage font (it bump)
	if (this->is_stage)
	{
		RECT_FIXED dst = {x * FIXED_UNIT, y * FIXED_UNIT, src->w * FIXED_UNIT, src->h * FIXED_UNIT};
		Stage_DrawTexCol(tex, src, &dst, stage.bump, r, g, b);
	}
	else
	{
		RECT dst = {x, y, src->w, src->h};
		Gfx_DrawTexCol(tex, src, &dst, r, g, b);
	}
}

//Font_Bold
s32 Font_Bold_GetWidth(struct FontData *this, const char *text)
{
	(void)this;
	return strlen(text) * 13;
}

void Font_Bold_DrawCol(struct FontData *this, const char *text, s32 x, s32 y, FontAlign align, u8 r, u8 g, u8 b)
{
	//Offset position based off alignment
	switch (align)
	{
		case FontAlign_Left:
			break;
		case FontAlign_Center:
			x -= Font_Bold_GetWidth(this, text) >> 1;
			break;
		case FontAlign_Right:
			x -= Font_Bold_GetWidth(this, text);
			break;
	}
	
	//Get animation offsets
	u32 v0 = ((animf_count / 2 % 2) != 0);
	
	//Draw string character by character
	u8 c;
	while ((c = *text++) != '\0')
	{
		//Draw character
		if ((c -= 'A') <= 'z' - 'A') //Lower-case will show inverted colours
		{
			RECT src = {((c % 0x8) * 28) + v0 * 14, (c / 8) * 16, 14, 16};
			Font_DrawTex(this, &src, x, y, r, g, b, &font_tex);
		}
		x += 13;
	}
}

//Font_Arial
#include "font_arialmap.h"

s32 Font_Arial_GetWidth(struct FontData *this, const char *text)
{
	(void)this;
	
	//Draw string width character by character
	s32 width = 0;
	
	u8 c;
	while ((c = *text++) != '\0')
	{
		//Shift and validate character
		if ((c -= 0x20) >= 0x60)
			continue;
		
		//Add width
		width += font_arialmap[c].gw;
	}
	
	return width;
}

void Font_Arial_DrawCol(struct FontData *this, const char *text, s32 x, s32 y, FontAlign align, u8 r, u8 g, u8 b)
{
	//Offset position based off alignment
	switch (align)
	{
		case FontAlign_Left:
			break;
		case FontAlign_Center:
			x -= Font_Arial_GetWidth(this, text) >> 1;
			break;
		case FontAlign_Right:
			x -= Font_Arial_GetWidth(this, text);
			break;
	}
	
	//Draw string character by character
	u8 c;
	while ((c = *text++) != '\0')
	{
		//Shift and validate character
		if ((c -= 0x20) >= 0x60)
			continue;
		
		//Draw character
		RECT src = {font_arialmap[c].ix, 130 + font_arialmap[c].iy, font_arialmap[c].iw, font_arialmap[c].ih};
		Font_DrawTex(this, &src, x, y, r, g, b, &font_tex);
		
		//Increment X
		x += font_arialmap[c].gw;
	}
}

//Font_CDR
#include "font_cdrmap.h"
s32 Font_CDR_GetWidth(struct FontData *this, const char *text)
{
	(void)this;
	return strlen(text) * 7;
}

void Font_CDR_DrawCol(struct FontData *this, const char *text, s32 x, s32 y, FontAlign align, u8 r, u8 g, u8 b)
{
	//Offset position based off alignment
	switch (align)
	{
		case FontAlign_Left:
			break;
		case FontAlign_Center:
			x -= Font_CDR_GetWidth(this, text) >> 1;
			break;
		case FontAlign_Right:
			x -= Font_CDR_GetWidth(this, text);
			break;
	}
	
	//Draw string character by character
	u8 c;
	while ((c = *text++) != '\0')
	{
		//Draw character
		//Shift and validate character
		if ((c -= 0x20) >= 0x60)
			continue;

		RECT src = {font_cdrmap[c].charX, font_cdrmap[c].charY + 198, font_cdrmap[c].charW, font_cdrmap[c].charL};

		Font_DrawTex(this, &src, x, y, r, g, b, &font_tex);

		//Increment X
		x += (font_cdrmap[c].charW - 1);
	}
}

//Common font functions
void Font_Draw(struct FontData *this, const char *text, s32 x, s32 y, FontAlign align)
{
	this->draw_col(this, text, x, y, align, 0x80, 0x80, 0x80);
}

//Font functions
void Font_Init(void)
{
	Gfx_LoadTex(&font_tex, IO_Read("\\FONT\\FONT1.TIM;1"), GFX_LOADTEX_FREE);
}
void FontData_Load(FontData *this, Font font, boolean is_stage)
{
	//Load the given font
	switch (font)
	{
		case Font_Bold:
			//Set functions
			this->get_width = Font_Bold_GetWidth;
			this->draw_col = Font_Bold_DrawCol;
			break;
		case Font_Arial:
			//Set functions
			this->get_width = Font_Arial_GetWidth;
			this->draw_col = Font_Arial_DrawCol;
			break;
	case Font_CDR:
			//Set functions
			this->get_width = Font_CDR_GetWidth;
			this->draw_col = Font_CDR_DrawCol;
			break;
	}
	this->draw = Font_Draw;
	this->is_stage = is_stage;
}
