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

#ifndef VQMTEXT_H
#define VQMTEXT_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     text.h
*
* DESCRIPTION
*     Text printing definitions. (32-Bit protected mode)
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     March 13, 1995
*
****************************************************************************/
#include <stdint.h>

/* Prototypes */


int32_t Text_Print(char *string, int32_t x, int32_t y, int32_t fcol, int32_t bcol);
void Draw_Char(int32_t character, int32_t x, int32_t y);
void Set_Font_Palette_Range(void const *palette, int32_t start, int32_t end);


#endif /* VQMTEXT_H */
