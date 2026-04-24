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

#ifndef VQMGRAPHICS_H
#include <stdint.h>
#define VQMGRAPHICS_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     graphics.h
*
* DESCRIPTION
*     Graphic rendering and manipulation definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     April 27, 1995
*
****************************************************************************/
/* Prototypes */


void Eor_Region(int32_t sx, int32_t sy, int32_t dx, int32_t dy, int32_t color);
void Fill_Rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t color);


#endif /* VQMGRAPHICS_H */
