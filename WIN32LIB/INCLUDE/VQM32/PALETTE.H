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

#ifndef VQMPALETTE_H
#include <stdint.h>
#define VQMPALETTE_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     Palette.h (32-Bit protected mode)
*
* DESCRIPTION
*     Palette definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     Febuary 3, 1995
*
****************************************************************************/
/* Prototypes */


void SetPalette(uint8_t *palette,int32_t numbytes,uint32_t slowpal);
void ReadPalette(void *palette);
void SetDAC(int32_t color, int32_t red, int32_t green, int32_t blue);
void TranslatePalette(void *pal24, void *pal15, int32_t numbytes);


void SortPalette(uint8_t *pal, int32_t numcolors);

#endif /* VQMPALETTE_H */
