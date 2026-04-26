/*
**	Command & Conquer(tm)
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

/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : INTERPAL.CPP                                                 *
 *                                                                                             *
 *                   Programmer : Steve Tall                                                   *
 *                                                                                             *
 *                   Start Date : December 7th 1995                                            *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Overview:                                                                                   *
 *  This module contains functions to allow use of old 320x200 animations on a 640x400 screen  *
 *                                                                                             *
 * Functions:                                                                                  *
 *  Read_Interpolation_Palette -- reads an interpolation palette table from disk               *
 *  Write_Interpolation_Palette -- writes an interpolation palette to disk                     *
 *  Create_Palette_Interpolation_Table -- build the palette interpolation table                *
 *  Increase_Palette_Luminance -- increase the contrast of a palette                           *
 *  Interpolate_2X_Scale -- Stretch a 320x200 graphic buffer into 640x400                      *
 *                                                                                             *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"

BOOL	InterpolationPaletteChanged = FALSE;
extern "C" {
extern void Asm_Interpolate (unsigned char* src_ptr ,
						 						unsigned char* dest_ptr ,
												int				lines ,
												int				src_width ,
												int				dest_width);

extern void Asm_Interpolate_Line_Double (unsigned char* src_ptr ,
						 						unsigned char* dest_ptr ,
												int				lines ,
												int				src_width ,
												int				dest_width);

extern void Asm_Interpolate_Line_Interpolate (unsigned char* src_ptr ,
						 						unsigned char* dest_ptr ,
												int				lines ,
												int				src_width ,
												int				dest_width);

}

extern "C"{
	unsigned char PaletteInterpolationTable[SIZE_OF_PALETTE][SIZE_OF_PALETTE];
	unsigned char *InterpolationPalette;
}



/***********************************************************************************************
 * Read_Interpolatioin_Palette -- reads an interpolation palette table from disk               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    name of palette file                                                              *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    12/12/95 12:15PM ST : Created                                                            *
 *=============================================================================================*/

void Read_Interpolation_Palette (char const *palette_file_name)
{
	CCFileClass palette_file(palette_file_name);

	if (palette_file.Is_Available()){
		palette_file.Open(READ);
		palette_file.Read(&PaletteInterpolationTable[0][0],256*256);
		palette_file.Close();
		InterpolationPaletteChanged = FALSE;
	}
}


/***********************************************************************************************
 * Write_Interpolatioin_Palette -- writes an interpolation palette table to disk               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    name of palette file                                                              *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    12/12/95 12:15PM ST : Created                                                            *
 *=============================================================================================*/

void Write_Interpolation_Palette (char const *palette_file_name)
{
	CCFileClass palette_file(palette_file_name);

	if (!palette_file.Is_Available()){
		palette_file.Open(WRITE);
		palette_file.Write(&PaletteInterpolationTable[0][0],256*256);
		palette_file.Close();
	}
}





/***************************************************************************
 * CREATE_PALETTE_INTERPOLATION_TABLE                                      *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/06/1995  MG : Created.                                             *
 *=========================================================================*/
void Create_Palette_Interpolation_Table( void )
{

	Asm_Create_Palette_Interpolation_Table();

	InterpolationPaletteChanged = FALSE;
	return;

}









/***********************************************************************************************
 * Increase_Palette_Luminance -- increase contrast of colours in a palette                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to palette                                                                    *
 *           percentage increase of red                                                        *
 *           percentage increase of green                                                      *
 *           percentage increase of blue                                                       *
 *           cap value for colours                                                             *
 *                                                                                             *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    12/12/95 12:16PM ST : Created                                                            *
 *=============================================================================================*/

void Increase_Palette_Luminance (unsigned char *palette , int red_percentage , int green_percentage , int blue_percentage ,int cap)
{

	unsigned	int	red;
	unsigned	int	green;
	unsigned	int	blue;
	for (int i=0 ; i<SIZE_OF_PALETTE*3 ; i+=3){

		red 	= (unsigned)*(palette+i);
		green = (unsigned)*(palette+i+1);
		blue	= (unsigned)*(palette+i+2);

		red 	+= red*red_percentage/100;
		green	+= green*green_percentage/100;
		blue	+= blue*blue_percentage/100;

		red 	= MIN (cap,red);
		green	= MIN (cap,green);
		blue	= MIN (cap,blue);

		*(palette+i)	=(unsigned char)red;
		*(palette+i+1)	=(unsigned char)green;
		*(palette+i+2)	=(unsigned char)blue;

	}

}












int	CopyType	=0;

/***************************************************************************
 * INTERPOLATE_2X_SCALE                                                    *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   12/06/1995  MG : Created.                                             *
 *=========================================================================*/
void Interpolate_2X_Scale( GraphicBufferClass *source, GraphicViewPortClass *dest ,char const *palette_file_name)
{
	unsigned char	*src_ptr;
	unsigned char	*dest_ptr;
	int	src_width;
	int	dest_width;
//	int	width_counter;
	BOOL	source_locked = FALSE;
	BOOL	dest_locked = FALSE;


	/*
	**If a palette table exists on disk then read it in otherwise create it
	*/
	if (InterpolationPaletteChanged){
		if (palette_file_name){
			Read_Interpolation_Palette(palette_file_name);
		}
		if (InterpolationPaletteChanged){
			Create_Palette_Interpolation_Table();
		}
	}

	/*
	** Write the palette table to disk so we dont have to create it again next time
	*/
	if (palette_file_name){
		Write_Interpolation_Palette(palette_file_name);
	}
	if ( dest == &SeenBuff ) Hide_Mouse();

	Wait_Blit();
	/*
	** Lock video surfaces if requred
	*/
	if (source->Get_IsVideoSurface()){
		if (!source->Lock()){
			if (dest == &SeenBuff) Show_Mouse();
			return;
		}
		source_locked = TRUE;
	}
	if (dest->Get_IsVideoSurface()){
		if (!dest->Lock()) {
			if (source_locked){
				source->Unlock();
			}
			if (dest == &SeenBuff) Show_Mouse();
			return;
		}
		dest_locked = TRUE;
	}


	//
	// Get pointers to the source and destination buffers.
	//
	src_ptr = (unsigned char *) source->Get_Offset();
	dest_ptr = (unsigned char *) dest->Get_Offset();

	//
	// Get width of source and dest buffers.
	//
	src_width = source->Get_Width();
	dest_width = 2*(dest->Get_Width() + dest->Get_XAdd() + dest->Get_Pitch());

	/*
	** Call the appropriate assembly language copy routine
	*/
	switch (CopyType){
		case 0:
			Asm_Interpolate ( src_ptr , dest_ptr , source->Get_Height() , src_width , dest_width);
			break;

		case 1:
			Asm_Interpolate_Line_Double( src_ptr , dest_ptr , source->Get_Height() , src_width , dest_width);
			break;

		case 2:
			Asm_Interpolate_Line_Interpolate( src_ptr , dest_ptr , source->Get_Height() , src_width , dest_width);
			break;
	}

	if (source_locked) source->Unlock();
	if (dest_locked) dest->Unlock();
	if (dest == &SeenBuff) Show_Mouse();

}

