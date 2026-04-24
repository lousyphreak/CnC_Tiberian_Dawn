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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Part of the TILE Library                 *
 *                                                                         *
 *                    File Name : TILE.H                                   *
 *                                                                         *
 *                   Programmer : Barry W. Green                           *
 *                                                                         *
 *                   Start Date : February 2, 1995                         *
 *                                                                         *
 *                  Last Update : February 2, 1995 [BWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef TILE_H
#define TILE_H

#include <stdint.h>

/*=========================================================================*/
/* The following prototypes are for the file: ICONSET.CPP						*/
/*=========================================================================*/
void * Load_Icon_Set(char const *filename, void *iconsetptr, int32_t buffsize);
void Free_Icon_Set(void const *iconset);
int32_t Get_Icon_Set_Size(void const *iconset);
int32_t Get_Icon_Set_Width(void const *iconset);
int32_t Get_Icon_Set_Height(void const *iconset);
void * Get_Icon_Set_Icondata(void const *iconset);
void * Get_Icon_Set_Trans(void const *iconset);
void * Get_Icon_Set_Remapdata(void const *iconset);
void * Get_Icon_Set_Palettedata(void const *iconset);
int32_t Get_Icon_Set_Count(void const *iconset);
void * Get_Icon_Set_Map(void const *iconset);
#if (0)
/*
** This is the control structure at the start of a loaded icon set.  It must match
** the structure in WWLIB.I!  This structure MUST be a multiple of 16 bytes long.
*/
typedef struct {
	int16_t	Width;			// Width of icons (pixels).
	int16_t	Height;			// Height of icons (pixels).
	int16_t	Count;			// Number of (logical) icons in this set.
//	BOOL	Allocated;		// Was this iconset allocated?
	int16_t	Allocated;		// Was this iconset allocated?
	int32_t	Size;				// Size of entire iconset memory block.
	uint8_t *Icons;			// Offset from buffer start to icon data.
	int32_t	Palettes;		// Offset from buffer start to palette data.
	int32_t	Remaps;			// Offset from buffer start to remap index data.
	int32_t	TransFlag;		// Offset for transparency flag table.
	uint8_t *Map;				// Icon map offset (if present).
} IControl_Type;
#endif	//(0)

#pragma pack(push, 1)
typedef struct {
	int16_t	Width;			// Width of icons (pixels).
	int16_t	Height;			// Height of icons (pixels).
	int16_t	Count;			// Number of (logical) icons in this set.
	int16_t	Allocated;		// Was this iconset allocated?
	int16_t MapWidth;		// Width of map (in icons).
	int16_t MapHeight;		// Height of map (in icons).
	int32_t	Size;				// Size of entire iconset memory block.
	int32_t	Icons;			// Offset from buffer start to icon data.
//	unsigned char * Icons;	// Offset from buffer start to icon data.
	int32_t	Palettes;		// Offset from buffer start to palette data.
	int32_t	Remaps;			// Offset from buffer start to remap index data.
	int32_t	TransFlag;		// Offset for transparency flag table.
	int32_t	ColorMap;		// Offset for color control value table.
	int32_t	Map;				// Icon map offset (if present).
//	unsigned char * Map;				// Icon map offset (if present).
} IControl_Type;
#pragma pack(pop)

static_assert(sizeof(IControl_Type) == 40, "IControl_Type layout mismatch");


#endif //TILE_H
