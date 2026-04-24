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
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : BUFFGLBL.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : January 10, 1995                         *
 *                                                                         *
 *                  Last Update : January 10, 1995   [PWG]                 *
 *                                                                         *
 * This module holds the global fixup tables for the 8-bit buffer class.   *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "GBUFFER.H"
/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/*=========================================================================*/
/* Globals required by GraphicBufferClass for function pointers.  These		*/
/*   pointers will be set to the proper function when set mode is called.	*/
/*=========================================================================*/
int32_t (*GVPC_Blit_to_VVPC_Func)(void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
int32_t (*GVPC_Scale_To_VVPC)(	void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, char *);

#ifdef not_any_more_it_doesnt
/*=========================================================================*/
/* Globals required by VideoBufferClass for function pointers.  These		*/
/*   pointers will be set to the proper function when set mode is called.	*/
/*=========================================================================*/
void (*VVPC_Clear_Func)(void *, uint8_t);
int32_t (*VVPC_To_Buffer_Func)(void *,int32_t x, int32_t y, int32_t w, int32_t h, void *buff, int32_t size);
void (*VVPC_Put_Pixel_Func)(void *,int32_t x, int32_t y, uint8_t color);
int32_t  (*VVPC_Get_Pixel_Func)(void *, int32_t x, int32_t y);
int32_t (*VVPC_Buffer_To_Page)(int32_t x, int32_t y, int32_t w, int32_t h, void *Buffer, void *view);
int32_t (*VVPC_Blit_to_GVPC_Func)(void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
int32_t (*VVPC_Blit_to_VVPC_Func)(void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
int32_t (*VVPC_Scale_To_GVPC)(	void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, char *);
int32_t (*VVPC_Scale_To_VVPC)(	void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, char *);
int32_t (*VVPC_Print_Func)(		void *, const char *, int32_t, int32_t, int32_t, int32_t);
void (*VVPC_Draw_Stamp)(void *, void *, int32_t, int32_t, int32_t, void *);
int32_t (*VVPC_Size_Of_Region)(void *, int32_t, int32_t);

#endif	//not_any_more_it_doesnt

/*=========================================================================*/
/* We need to keep a pointer to the logic page hanging around somewhere		*/
/*=========================================================================*/
GraphicViewPortClass	*LogicPage;

int32_t IconCacheAllowed = TRUE;

/*
** Pointer to a function we will call if we detect loss of focus
*/
void (*Gbuffer_Focus_Loss_Function)(void) = NULL;
