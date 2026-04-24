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
 *                 Project Name : Westwood Library                         *
 *                                                                         *
 *                    File Name : GETSHAPE.CPP                             *
 *                                                                         *
 *                   Programmer : Joe L. Bostic                            *
 *                                                                         *
 *                   Start Date : April 5, 1992                            *
 *                                                                         *
 *                  Last Update : May 25, 1994   [BR]                      *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Get_Shape_Size -- Fetch the size of the shape in memory.              *
 *   Get_Shape_Uncomp_Size -- gets shape's uncompressed size in bytes		*
 *   Get_Shape_Data -- retrieves a shape's special prefix data					*
 *   Extract_Shape_Count -- returns # of shapes in the given shape block	*
 *   Extract_Shape -- Gets pointer to shape in given shape block				*
 *   Get_Shape_Width -- gets shape width in pixels									*
 *   Get_Shape_Height -- gets shape height in pixels								*
 *   Set_Shape_Height -- modifies shape's height									*
 *   Restore_Shape_Height -- restores a shape to its original height			*
 *   Get_Shape_Original_Height -- gets shape's unmodified height				*
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
/*
********************************* Includes **********************************
*/
#include <cstring>

#include <WWSTD.H>
#include	<SHAPE.H>
namespace {

template <typename T>
T Read_Unaligned(void const* data)
{
	T value;
	std::memcpy(&value, data, sizeof(value));
	return value;
}

template <typename T>
void Write_Unaligned(void* data, T value)
{
	std::memcpy(data, &value, sizeof(value));
}

}

char * _ShapeBuffer = 0;
int32_t _ShapeBufferSize = 0;

void  Set_Shape_Buffer(void const *buffer, int32_t size)
{
	_ShapeBuffer = (char *)buffer;
	_ShapeBufferSize = size;
}


/***************************************************************************
 * Get_Shape_Size -- Fetch the size of the shape in memory.                *
 *                                                                         *
 * The shape size returned includes both the shape header & its data.		*
 *																									*
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape's size in memory																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t  Get_Shape_Size(void const *shape)
{
	/*
	------------------------- Return if NULL pointer -------------------------
	*/
	if (!shape) 
		return(0);

	/*
	-------------------------- Returns shape's size --------------------------
	*/
	return Read_Unaligned<uint16_t>((char const*)shape + offsetof(Shape_Type, ShapeSize));

}	/* end of Get_Shape_Size */


/***************************************************************************
 * Get_Shape_Uncomp_Size -- gets shape's uncompressed size in bytes			*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 *                                                                         *
 * OUTPUT:                                                                 *
 *	shape's size in bytes when uncompressed											*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t Get_Shape_Uncomp_Size(void const *shape)
{
	return Read_Unaligned<uint16_t>((char const*)shape + offsetof(Shape_Type, DataLength));

}	/* end of Get_Shape_Uncomp_Size */


/***************************************************************************
 * Get_Shape_Data -- retrieves a shape's special prefix data					*
 *																									*
 * MAKESHPS.EXE can store special data values along with a shape.  These	*
 * values are inserted in the shape table >before< the shape's header.		*
 * So, this routine uses the 'data' parameter as a negative index from		*
 * the given shape pointer.																*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to shape																*
 * data		index of WORD data value to get											*
 *                                                                         *
 * OUTPUT:                                                                 *
 * data value																					*
 *                                                                         *
 * WARNINGS:                                                               *
 *	The shape pointer must be a pointer into a shape table created by			*
 * MAKESHPS.EXE; it >cannot< be a pointer to shape returned by Make_Shape!	*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int16_t  Get_Shape_Data(void const *shape, int16_t data)
{
	int16_t retval;

	retval = Read_Unaligned<int16_t>((char const*)shape - ((data+1) * sizeof(int16_t)));

	return (retval);

}	/* end of Get_Shape_Data */


/***************************************************************************
 * Extract_Shape_Count -- returns # of shapes in the given shape block		*
 *                                                                         *
 * The # of shapes in a shape block is the first WORD in the block, so		*
 * this is the value returned.															*
 *																									*
 * INPUT:                                                                  *
 * buffer	pointer to shape block, created with MAKESHPS.EXE					*
 *                                                                         *
 * OUTPUT:                                                                 *
 * # shapes in the block																	*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t  Extract_Shape_Count(void const *buffer)
{
	return Read_Unaligned<uint16_t>(buffer);

}	/* end of Extract_Shape_Count */


/***************************************************************************
 * Extract_Shape -- Gets pointer to shape in given shape block					*
 *                                                                         *
 * INPUT:                                                                  *
 * buffer	pointer to shape block, created with MAKESHPS.EXE					*
 * shape		index of shape to get														*
 *                                                                         *
 * OUTPUT:                                                                 *
 * pointer to shape in the shape block													*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
void *  Extract_Shape(void const *buffer, int32_t shape)
{
	int32_t numshapes;		// Number of shapes
	int32_t offset;			// Offset of shape data, from start of block
	char *bytebuf = (char*) buffer;

	/*
	----------------------- Return if invalid argument -----------------------
	*/
	if (!buffer)
		return(NULL);

	numshapes = Read_Unaligned<uint16_t>(buffer);
	if (shape < 0 || shape >= numshapes)
		return(NULL);

	offset = Read_Unaligned<int32_t>(bytebuf + offsetof(ShapeBlock_Type, Offsets) + (shape * sizeof(int32_t)));

	return(bytebuf + 2 + offset);

}	/* end of Extract_Shape */


/***************************************************************************
 * Get_Shape_Width -- gets shape width in pixels									*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 *	shape width in pixels																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t Get_Shape_Width(void const *shape)
{
	return Read_Unaligned<uint16_t>((char const*)shape + offsetof(Shape_Type, Width));

}	/* end of Get_Shape_Width */


/***************************************************************************
 * Get_Shape_Height -- gets shape height in pixels									*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape height in pixels																	*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t Get_Shape_Height(void const *shape)
{
	return Read_Unaligned<uint8_t>((char const*)shape + offsetof(Shape_Type, Height));

}	/* end of Get_Shape_Height */


/***************************************************************************
 * Set_Shape_Height -- modifies shape's height										*
 *                                                                         *
 * The new height must be shorter than the original height.  This effect	*
 * chops off the lower portion of the shape, like it's sinking into the		*
 * ground.																						*
 *																									*
 * INPUT:                                                                  *
 * shape			pointer to a shape														*
 * newheight	new shape height															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * old shape height																			*
 *                                                                         *
 * WARNINGS:                                                               *
 * none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t  Set_Shape_Height(void const *shape, int16_t newheight)
{
	int16_t oldheight;

	oldheight = Read_Unaligned<uint8_t>((char const*)shape + offsetof(Shape_Type, Height));
	Write_Unaligned<uint8_t>((char*)shape + offsetof(Shape_Type, Height), (uint8_t)newheight);

	return(oldheight);

}	/* end of Set_Shape_Height */


/***************************************************************************
 * Restore_Shape_Height -- restores a shape to its original height			*
 *                                                                         *
 * INPUT:                                                                  *
 * shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * old shape height																			*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t  Restore_Shape_Height(void *shape)
{
	int16_t oldheight;

	oldheight = Read_Unaligned<uint8_t>((char const*)shape + offsetof(Shape_Type, Height));
	Write_Unaligned<uint8_t>((char*)shape + offsetof(Shape_Type, Height),
		Read_Unaligned<uint8_t>((char const*)shape + offsetof(Shape_Type, OriginalHeight)));

	return(oldheight);

}	/* end of Restore_Shape_Height */


/***************************************************************************
 * Get_Shape_Original_Height -- gets shape's unmodified height					*
 *                                                                         *
 * INPUT:                                                                  *
 *	shape		pointer to a shape															*
 *                                                                         *
 * OUTPUT:                                                                 *
 * shape's unmodified height																*
 *                                                                         *
 * WARNINGS:                                                               *
 *	none																							*
 *                                                                         *
 * HISTORY:                                                                *
 *   06/09/1992 JLB : Created.                                             *
 *   08/19/1993 SKB : Split drawshp.asm into several modules.              *
 *   05/25/1994 BR : Converted to 32-bit                                   *
 *=========================================================================*/
int32_t Get_Shape_Original_Height(void const *shape)
{
	return Read_Unaligned<uint8_t>((char const*)shape + offsetof(Shape_Type, OriginalHeight));

}	/* end of Get_Shape_Original_Height */


/************************* end of getshape.cpp *****************************/
