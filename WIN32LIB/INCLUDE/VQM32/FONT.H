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

#ifndef VQMFONT_H
#include <stdint.h>
#define VQMFONT_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*---------------------------------------------------------------------------
*
* FILE
*     font.h
*
* DESCRIPTION
*     Font definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     March 9, 1995
*
****************************************************************************/
/* Font: A Westwood style font.
 *
 * Size       - Size of font.
 * CompMethod - Compression method of font. (0 = none)
 * NumBlks    - Number of data blocks.
 * InfoBlk    - Offset to font information block.
 * OffsetBlk  - Offset to character offset block.
 * WidthBlk   - Offset to character width block.
 * DataBlk    - Offset to character data block.
 * HeightBlk  - Offset to character height block.
 */
typedef struct _Font {
	uint16_t Size;
	uint8_t CompMethod;
	uint8_t NumBlks;
	uint16_t InfoBlk;
	uint16_t OffsetBlk;
	uint16_t WidthBlk;
	uint16_t DataBlk;
	uint16_t HeightBlk;
} Font;

typedef struct _FontInfo {
	int32_t huh;
	uint8_t MaxHeight;
	uint8_t MaxWidth;
} FontInfo;


extern void const *FontPtr;
extern int FontXSpacing;
extern int FontYSpacing;


extern char FontWidth;
extern char FontHeight;
extern char *FontWidthBlockPtr;

/* Function prototypes. */
void *Load_Font(char const *name);
void *Set_Font(void const *font);
uint32_t String_Pixel_Width(char const *string);

int32_t Char_Pixel_Width(char chr);

#endif /* VQMFONT_H */
