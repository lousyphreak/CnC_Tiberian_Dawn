/*
**	Command & Conquer Red Alert(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VQMMONO_H
#include <stdint.h>
#define VQMMONO_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     mono.h
*
* DESCRIPTION
*     Mono screen definitions. (32-Bit protected mode)
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     Feburary 8, 1995
*
****************************************************************************/

/* Prototypes */


void Mono_Enable(void);
void Mono_Disable(void);
void Mono_Set_Cursor(int32_t x, int32_t y);
void Mono_Clear_Screen(void);
void Mono_Scroll(int32_t lines);
void Mono_Put_Char(int32_t character, int32_t attrib);
void Mono_Draw_Rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t attrib,
		int32_t thick);

void Mono_Text_Print(void const *text, int32_t x, int32_t y, int32_t attrib);
void Mono_Print(void const *text);
int16_t Mono_View_Page(int32_t page);
int16_t Mono_X(void);
int16_t Mono_Y(void);


#endif /* VQMMONO_H */
