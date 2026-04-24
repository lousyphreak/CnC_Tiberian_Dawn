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
 **   C O N F I D E N T I A L --- W E S T W O O D    S T U D I O S        **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : WWLIB32                                  *
 *                                                                         *
 *                    File Name : SHAPE.H                                  *
 *                                                                         *
 *                   Programmer : Bill Randolph                            *
 *                                                                         *
 *                   Start Date : May 25, 1994                             *
 *                                                                         *
 *                  Last Update : September 14, 1994   [IML]               *
 *                                                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#ifndef SHAPE_H
#define SHAPE_H

#include <cstddef>
#include <cstdint>

#ifndef GBUFFER_H
#include "GBUFFER.H"
#endif
/*
*********************************** Types ***********************************
*/
/*
--------------------------- Shape creation flags ----------------------------
*/
typedef enum {
	MAKESHAPE_NORMAL	 = 0x0000,	// 256-color compressed shape
	MAKESHAPE_COMPACT  = 0x0001,	// 16-color shape (with built-in color table)
	MAKESHAPE_NOCOMP	 = 0x0002,	// Uncompressed shape
	MAKESHAPE_VARIABLE =	0x0004	// <16-color shape
} MakeShapeFlags_Type;

/*---------------------------------------------------------------------------
Shape drawing flags:
- The low byte is for coordinate transformations.
- The high byte is for drawing effects.
---------------------------------------------------------------------------*/
typedef enum {
	SHAPE_NORMAL 			= 0x0000,		// Standard shape
	SHAPE_HORZ_REV 		= 0x0001,		// Flipped horizontally
	SHAPE_VERT_REV 		= 0x0002,		// Flipped vertically
	SHAPE_SCALING 			= 0x0004,		// Scaled (WORD scale_x, WORD scale_y)
	SHAPE_VIEWPORT_REL 	= 0x0010,		// Coords are window-relative
	SHAPE_WIN_REL 			= 0x0010,		// Coordinates are window relative instead of absolute.
	SHAPE_CENTER 			= 0x0020,		// Coords are based on shape's center pt
	SHAPE_FADING 			= 0x0100,		// Fading effect (void * fading_table,
													//  WORD fading_num)
	SHAPE_PREDATOR 		= 0x0200,		// Transparent warping effect
	SHAPE_COMPACT 			= 0x0400,		// Never use this bit
	SHAPE_PRIORITY 		= 0x0800,		// Use priority system when drawing
	SHAPE_GHOST				= 0x1000,		// Shape is drawn ghosted
	SHAPE_SHADOW			= 0x2000,
	SHAPE_PARTIAL  		= 0x4000,
	SHAPE_COLOR 			= 0x8000			// Remap the shape's colors
													//  (void * color_table)
} ShapeFlags_Type;

/*
------------------------------- Shape header --------------------------------
*/
#pragma pack(push, 1)
typedef struct {
	uint16_t		ShapeType;			// 0 = normal, 1 = 16 colors,
										//  2 = uncompressed, 4 = 	<16 colors
	uint8_t		Height;				// Height of the shape in scan lines
	uint16_t		Width;				// Width of the shape in bytes
	uint8_t		OriginalHeight;	// Original height of shape in scan lines
	uint16_t		ShapeSize;			// Size of the shape, including header
	uint16_t		DataLength;			// Size of the uncompressed shape (just data)
	uint8_t		Colortable[16];	// Optional color table for compact shape
} Shape_Type;
#pragma pack(pop)
static_assert(sizeof(Shape_Type) == 26, "Shape_Type must stay 26 bytes");

/*
------------------------------- Shape block ---------------------------------
*/
#pragma pack(push, 1)
typedef struct {
	uint16_t		NumShapes;			// number of shapes in the block
	int32_t		Offsets[];			// array of offsets to shape data
										//  (offsets within the shape block, with
										//  0 being the first offset value, not the
										//  start of the shape block)
} ShapeBlock_Type;
#pragma pack(pop)
static_assert(offsetof(ShapeBlock_Type, Offsets) == 2, "ShapeBlock_Type offsets must start immediately after NumShapes");


/*
******************************** Prototypes *********************************
*/

/*
-------------------------------- prioinit.c ---------------------------------
*/

extern void  *MaskPage;
extern void  *BackGroundPage;
extern int32_t  _ShapeBufferSize;
extern char  *_ShapeBuffer;


void  Init_Priority_System (GraphicBufferClass *mask,
											GraphicBufferClass *back);


/*
-------------------------------- drawshp.asm --------------------------------
*/

int32_t Draw_Shape(GraphicViewPortClass *gvp, void const *shape, int32_t x, int32_t y, int32_t flags, ...);

/*
---------------------------------- shape.c ----------------------------------
*/
int16_t  Get_Shape_Data(void const *shape, int32_t data);
int32_t  Extract_Shape_Count(void const *buffer);
void *  Extract_Shape(void const *buffer, int32_t shape);
int32_t  Restore_Shape_Height(void *shape);
int32_t  Set_Shape_Height(void const *shape, int32_t newheight);

int32_t  Get_Shape_Width(void const *shape);
int32_t  Get_Shape_Height(void const *shape);
int32_t  Get_Shape_Original_Height(void const *shape);
int32_t  Get_Shape_Uncomp_Size(void const *shape);


/*
------------------------------- setshape.asm --------------------------------
*/
void  Set_Shape_Buffer(void const *buffer, int32_t size);
/*
------------------------------- shapeinf.asm --------------------------------
*/
int32_t  Get_Shape_Flags(void const *shape);
int32_t   Get_Shape_Size(void const *shape);
int32_t   Get_Shape_Scaled_Width(void const *shape, int32_t scale);
int32_t   Get_Shape_Scaled_Height(void const *shape, int32_t scale);

#endif // SHAPE_H

/****************************** End of shape.h *****************************/
