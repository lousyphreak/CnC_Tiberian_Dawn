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
 *                 Project Name : Westwood 32 Bit Library						*
 *                                                                         *
 *                    File Name : GBUFFER.H                                *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : May 26, 1994                             *
 *                                                                         *
 *                  Last Update : October 9, 1995   []                     *
 *                                                                         *
 ***************************************************************************
 *                                                                         *
 *	This module contains the definition for the graphic buffer class.  The	*
 * primary functionality of the graphic buffer class is handled by inline	*
 * functions that make a call through function pointers to the correct		*
 * routine.  This has two benefits:														*
 *																									*
 *																									*
 *		1) C++ name mangling is not a big deal since the function pointers	*
 *		   point to functions in standard C format.									*
 *		2) The function pointers can be changed when the active backend		*
 *       changes.  This keeps the higher-level buffer API independent		*
 *			of the underlying SDL/software implementation details.				*
 *																									*
 * In the basic library, these functions point to stub routines which just	*
 * return.  This makes a product that just uses a graphic buffer take the	*
 * minimum amount of code space.  Backend-specific implementations can		*
 * override the necessary stubs without changing the GraphicBuffer API.	*
 *																									*
 * In addition, there are helpful inline function calls for parameter		*
 * ellimination.  This header file gives the defintion for all					*
 * GraphicViewPort and GraphicBuffer classes.  										*
 *																									*
 * Terminology:																				*
 *																									*
 *	Buffer Class - A class which consists of a pointer to an allocated		*
 *		buffer and the size of the buffer that was allocated.						*
 *																									*
 *	Graphic ViewPort - The Graphic ViewPort defines a window into a 			*
 *		Graphic Buffer.  This means that although a Graphic Buffer 				*
 *		represents linear memory, this may not be true with a Graphic			*
 *		Viewport.  All low level functions that act directly on a graphic		*
 *		viewport are included within this class.  This includes but is not	*
 *		limited to most of the functions which can act on a Video Viewport	*
 *		Video Buffer.																			*
 *																									*
 * Graphic Buffer - A Graphic Buffer is an instance of an allocated buffer	*
 *		used to represent a rectangular region of graphics memory.				*
 *		The HidBuff	and BackBuff are excellent examples of a Graphic Buffer.	*
 *																									*
 * Below is a tree which shows the relationship of the VideoBuffer and 		*
 * Buffer classes to the GraphicBuffer class:										*
 *																									*
 *	  BUFFER.H				 GBUFFER.H			  BUFFER.H				 VBUFFER.H	*
 *  ----------          ----------         ----------          ----------	*
 * |  Buffer  |        | Graphic  |       |  Buffer  |        |  Video   |	*
 * |  Class   |        | ViewPort |       |  Class   |        | ViewPort |	*
 *  ----------          ----------         ----------          ----------	*
 *            \        /                             \        /				*
 *             \      /                               \      /					*
 *            ----------                             ----------				*
 *           |  Graphic |                           |  Video   |				*
 *           |  Buffer  |                           |  Buffer  |				*
 *            ----------                             ---------- 				*
 *	  			  GBUFFER.H			                       VBUFFER.H					*
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   GBC::GraphicBufferClass -- inline constructor for GraphicBufferClass  *
 *   GVPC::Remap -- Short form to remap an entire graphic view port        *
 *   GVPC::Get_XPos -- Returns x offset for a graphic viewport class       *
 *   GVPC::Get_Ypos -- Return y offset in a GraphicViewPortClass           *
 *   VVPC::Get_XPos -- Get the x pos of the VP on the Video                *
 *   VVPC::Get_YPos -- Get the y pos of the VP on the video                *
 *   GBC::Get_Graphic_Buffer -- Get the graphic buffer of the VP.          *
 *   GVPC::Draw_Line -- Stub function to draw line in Graphic Viewport Class*
 *   GVPC::Fill_Rect -- Stub function to fill rectangle in a GVPC          *
 *   GVPC::Remap -- Stub function to remap a GVPC                          *
 *   GVPC::Print -- stub func to print a text string                       *
 *   GVPC::Print -- Stub function to print an integer                      *
 *   GVPC::Print -- Stub function to print a short to a graphic viewport   *
 *   GVPC::Print -- stub function to print a long on a graphic view port   *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


#include "sdl_draw.h"


#ifndef GBUFFER_H
#define GBUFFER_H

/*=========================================================================*/
/* If we have not already loaded the standard library header, than we can	*/
/*		load it.																					*/
/*=========================================================================*/

#ifndef WWSTD_H
#include "WWSTD.H"
#endif

#ifndef DRAWBUFF_H
#include "DRAWBUFF.H"
#endif

#ifndef BUFFER_H
#include "BUFFER.H"
#endif

#ifndef WINDOWS_H
#include "WW_WIN.H"
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "ICONCACH.H"


#ifndef FUNCTION_H

#ifndef BITMAPCLASS
#define BITMAPCLASS
class BitmapClass
{
	public:
		BitmapClass(int32_t w, int32_t h, uint8_t * data) :
			Width(w), Height(h), Data(data) {};

		int32_t Width;
		int32_t Height;
		uint8_t * Data;
};

class TPoint2D
{
	public:
		TPoint2D(int32_t xx, int32_t yy) : x(xx), y(yy) {};
		TPoint2D(void) : x(0), y(0) {};

		int32_t x;
		int32_t y;
};
#endif

#endif


//////////////////////////////////////////////////////////////////////////
//
// Defines for direct draw
//
//
extern	WWDraw*	VideoDrawObject;	//pointer to direct draw object
extern	RAWindow*		MainWindow;			//handle to programs main window

/*
** Pointer to function to call if we detect a focus loss
*/
extern	void (*Gbuffer_Focus_Loss_Function)(void);

enum GBC_Enum {
	GBC_NONE				= 0,
	GBC_VIDEOMEM		= 1,
	GBC_VISIBLE			= 2,
};

#define	NOT_LOCKED		NULL

/*=========================================================================*/
/* Define the screen width and height to make portability to other modules	*/
/*		easier.																					*/
/*=========================================================================*/
#define	DEFAULT_SCREEN_WIDTH		320
#define	DEFAULT_SCREEN_HEIGHT	200

/*=========================================================================*/
/* Let the compiler know that a GraphicBufferClass exists so that it can	*/
/*		keep a pointer to it in a VideoViewPortClass.								*/
/*=========================================================================*/
class GraphicViewPortClass;
class GraphicBufferClass;
class VideoViewPortClass;
class VideoBufferClass;

GraphicViewPortClass *Set_Logic_Page(GraphicViewPortClass *ptr);
GraphicViewPortClass *Set_Logic_Page(GraphicViewPortClass &ptr);

/*=========================================================================*/
/* GraphicViewPortClass - Holds viewport information on a viewport which	*/
/*		has been attached to a GraphicBuffer.  A viewport is effectively a	*/
/*		rectangular subset of the full buffer which is used for clipping and	*/
/*		the like.																				*/
/*																									*/
/*			char	  	*Buffer	-		is the offset to view port buffer			*/
/*			int	  	Width		-		is the width of view port						*/
/*			int	  	Height	-		is the height of view port						*/
/*			int	  	XAdd		-		is add value to go from the end of a line	*/
/*											to the beginning of the next line			*/
/*			int		XPos;		- 		x offset into its associated VideoBuffer	*/
/*			int		YPos;		-		y offset into its associated VideoBuffer	*/
/*=========================================================================*/
class GraphicViewPortClass {
	public:

		/*===================================================================*/
		/* Define the base constructor and destructors for the class			*/
		/*===================================================================*/
		GraphicViewPortClass(GraphicBufferClass* graphic_buff, int32_t x, int32_t y, int32_t w, int32_t h);
		GraphicViewPortClass();
		~GraphicViewPortClass();

		/*===================================================================*/
		/* define functions to get at the private data members					*/
		/*===================================================================*/
		intptr_t	Get_Offset(void) const;
		int32_t	Get_Height(void) const;
		int32_t	Get_Width(void) const;
		int32_t	Get_XAdd(void) const;
		int32_t	Get_XPos(void) const;
		int32_t	Get_YPos(void) const;
		int32_t	Get_Pitch(void) const;
		inline int32_t	Get_IsVideoSurface(void);
		GraphicBufferClass	*Get_Graphic_Buffer(void);

		/*===================================================================*/
		/* Define a function which allows us to change a video viewport on	*/
		/*		the fly.																			*/
		/*===================================================================*/
		int32_t Change(int32_t x, int32_t y, int32_t w, int32_t h);

		/*===================================================================*/
		/* Define the set of common graphic functions that are supported by	*/
		/*		both Graphic ViewPorts and VideoViewPorts.							*/
		/*===================================================================*/
		int32_t	Size_Of_Region(int32_t w, int32_t h);
		void	Put_Pixel(int32_t x, int32_t y, uint8_t color);
		int32_t	Get_Pixel(int32_t x, int32_t y);
		void	Clear(uint8_t color = 0);
		int32_t	To_Buffer(int32_t x, int32_t y, int32_t w, int32_t h, void *buff, int32_t size);
		int32_t	To_Buffer(int32_t x, int32_t y, int32_t w, int32_t h, BufferClass *buff);
		int32_t	To_Buffer(BufferClass *buff);
		HRESULT	Blit(	GraphicViewPortClass& dest, int32_t x_pixel, int32_t y_pixel, int32_t dx_pixel,
						int32_t dy_pixel, int32_t pixel_width, int32_t pixel_height, int32_t trans = FALSE);
		HRESULT	Blit(	GraphicViewPortClass& dest, int32_t dx, int32_t dy, int32_t trans = FALSE);
		HRESULT	Blit(	GraphicViewPortClass& dest, int32_t trans = FALSE);
		HRESULT	Blit(	VideoViewPortClass& dest, int32_t x_pixel, int32_t y_pixel, int32_t dx_pixel,
						int32_t dy_pixel, int32_t pixel_width, int32_t pixel_height, int32_t trans = FALSE);
		HRESULT	Blit(	VideoViewPortClass& dest, int32_t dx, int32_t dy, int32_t trans = FALSE);
		HRESULT	Blit(	VideoViewPortClass& dest, int32_t trans = FALSE);
		int32_t	Scale(	GraphicViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
							int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, int32_t trans = FALSE, char *remap = NULL);
		int32_t	Scale(	GraphicViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
							int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, char *remap);
		int32_t	Scale(	GraphicViewPortClass &dest, int32_t trans = FALSE, char *remap = NULL);
		int32_t	Scale(	GraphicViewPortClass &dest, char *remap);
		int32_t	Scale(	VideoViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
							int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, int32_t trans = FALSE, char *remap = NULL);
		int32_t	Scale(	VideoViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
							int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, char *remap);
		int32_t	Scale(	VideoViewPortClass &dest, int32_t trans = FALSE, char *remap = NULL);
		int32_t	Scale(	VideoViewPortClass &dest, char *remap);
		uint32_t	Print(char const *string, int32_t x_pixel, int32_t y_pixel, int32_t fcolor, int32_t bcolor);
		uint32_t	Print(int16_t num, int32_t x_pixel, int32_t y_pixel, int32_t fcol, int32_t bcol);
		uint32_t	Print(int32_t num, int32_t x_pixel, int32_t y_pixel, int32_t fcol, int32_t bcol);

		/*===================================================================*/
		/* Define the list of graphic functions which work only with a 		*/
		/*		graphic buffer.																*/
		/*===================================================================*/
		void Draw_Line(int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color);
		void Draw_Rect(int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color);
		void Fill_Rect(int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color);
		void Fill_Quad(void *span_buff, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
							int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t color);
		void Remap(int32_t sx, int32_t sy, int32_t width, int32_t height, void *remap);
		void Remap(void *remap);
		void Draw_Stamp(void const *icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const *remap);
		void Draw_Stamp(void const *icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const *remap, int32_t clip_window);

//	This doesnt seem to exist anywhere?? - Steve T 9/26/95 6:05PM
//		VOID Grey_Out_Region(int x, int y, int width, int height, int color);

		//
		// New members to lock and unlock the direct draw video memory
		//
		inline int32_t 	Lock ();
		inline int32_t 	Unlock();
		inline int32_t		Get_LockCount();

		// Member to blit using the SDL draw hardware path
		HRESULT Video_Linear_Blit_To_Linear ( GraphicViewPortClass &dest, int32_t source_x, int32_t source_y, int32_t dest_x, int32_t dest_y, int32_t width	, int32_t height, int32_t mask );

		/*===================================================================*/
		/* Define functions to attach the viewport to a graphicbuffer			*/
		/*===================================================================*/
		void Attach(GraphicBufferClass *graphic_buff, int32_t x, int32_t y, int32_t w, int32_t h);
		void Attach(GraphicBufferClass *video_buff, int32_t w, int32_t h);

	protected:

		/*===================================================================*/
		/* Define the data used by a GraphicViewPortClass							*/
		/*===================================================================*/
		intptr_t				Offset;			// offset to graphic page
		int32_t						Width;			// width of graphic page
		int32_t						Height;			// height of graphic page
		int32_t						XAdd;			// xadd for graphic page (0)
		int32_t						XPos;			// x offset in relation to graphicbuff
		int32_t						YPos;			// y offset in relation to graphicbuff
		int32_t				Pitch;			//Distance from one line to the next
		GraphicBufferClass		*GraphicBuff;	// related graphic buff
		int32_t					IsVideoSurface;	//Flag to let us know if it is a direct draw surface
		int32_t						LockCount;		// Count for stacking locks if non-zero the buffer
};                                              //   is a locked DD surface

/*=========================================================================*/
/* GraphicBufferClass - A GraphicBuffer refers to an actual instance of an	*/
/*		allocated buffer.  The GraphicBuffer may be drawn to directly 			*/
/*		becuase it inherits a ViewPort which represents its physcial size.	*/
/*																									*/
/*			BYTE	  	*Buffer	-		is the offset to graphic buffer				*/
/*			int	  	Width		-		is the width of graphic buffer				*/
/*			int	  	Height	-		is the height of graphic buffer				*/
/*			int	  	XAdd		-		is the xadd of graphic buffer					*/
/*			int		XPos;		- 		will be 0 because it is graphicbuff			*/
/*			int		YPos;		-		will be 0 because it is graphicbuff			*/
/*			long	Pitch		-		modulo of buffer for reading and writing																						*/
/*			BOOL	IsVideoSurface - 		flag if its a direct draw surface																						*/
/*=========================================================================*/
class GraphicBufferClass : public GraphicViewPortClass, public BufferClass {

	public:
		GraphicBufferClass(int32_t w, int32_t h, GBC_Enum flags);
		GraphicBufferClass(int32_t w, int32_t h,	void *buffer, int32_t size);
		GraphicBufferClass(int32_t w, int32_t h, void *buffer = 0);
		GraphicBufferClass(void);
		~GraphicBufferClass();

		void Video_Init(GBC_Enum flags);
		void Init(int32_t w, int32_t h, void *buffer, int32_t size, GBC_Enum flags);
		void Un_Init(void);
		void Attach_Surface (GraphicBufferClass * attach_buffer);
		int32_t Lock(void);
		int32_t Unlock(void);

		void Scale_Rotate(BitmapClass &bmp,TPoint2D const &pt,int32_t scale,uint8_t angle);

		// Member to get a pointer to a direct draw surface
		WWSurface* Get_Video_Surface ( void );

	protected:
		WWSurface*	VideoSurfacePtr;		//Pointer to the related video surface
		WWLockData	VideoLockData;

};



inline int32_t GraphicViewPortClass::Get_LockCount(void)
{
	return (LockCount);
}



/***********************************************************************************************
 * GVPC::Get_IsVideoSurface -- provide read access to the IsVideoSurface flag                      *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   IsVideoSurface                                                                      *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/29/95 1:02PM ST : Created                                                             *
 *=============================================================================================*/
inline int32_t GraphicViewPortClass::Get_IsVideoSurface(void)
{
	return (IsVideoSurface);
}



/***********************************************************************************************
 * GBC::Get_Video_Surface -- returns a pointer to the buffer direct draw surface                  *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   ptr to direct draw surface                                                        *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/29/95 9:43AM ST : Created                                                              *
 *=============================================================================================*/
inline WWSurface* GraphicBufferClass::Get_Video_Surface ( void )
{
	return ( VideoSurfacePtr );

}



/***********************************************************************************************
 * GVPC::Lock -- lock the graphics buffer for reading or writing                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully locked                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-19-95 12:33pm ST : Created                                                             *
 *   10/09/1995     : Moved actually functionality to GraphicBuffer                            *
 *=============================================================================================*/
inline int32_t GraphicViewPortClass::Lock(void)
{
	int32_t lock = GraphicBuff->Lock();
	if ( !lock ) return(FALSE);

	if (this != GraphicBuff) {
		Attach(GraphicBuff, XPos, YPos,  Width, Height);
	}
	return(TRUE);
}

/***********************************************************************************************
 * GVPC::Unlock -- unlock the video buffer                                                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if surface was successfully unlocked                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09-19-95 02:20pm ST : Created                                                             *
 *   10/09/1995     : Moved actually functionality to GraphicBuffer                            *
 *=============================================================================================*/
inline int32_t GraphicViewPortClass::Unlock(void)
{
	int32_t unlock = GraphicBuff->Unlock();
	if (!unlock) return(FALSE);
	if (this != GraphicBuff && IsVideoSurface && !GraphicBuff->LockCount) {
		Offset = 0;
	}
	return(TRUE);
}


/***************************************************************************
 * GVPC::GET_OFFSET -- Get offset for virtual view port class instance     *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     long the offset for the virtual viewport instance           *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline intptr_t GraphicViewPortClass::Get_Offset(void) const
{
	return(Offset);
}

/***************************************************************************
 * GVPC::GET_HEIGHT -- Gets the height of a virtual viewport instance      *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     WORD the height of the virtual viewport instance            *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t GraphicViewPortClass::Get_Height(void) const
{
	return(Height);
}

/***************************************************************************
 * GVPC::GET_WIDTH -- Get the width of a virtual viewport instance			*
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     WORD the width of the virtual viewport instance             *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t GraphicViewPortClass::Get_Width(void) const
{
	return(Width);
}


/***************************************************************************
 * GVPC::GET_XADD -- Get the X add offset for virtual viewport instance    *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     WORD the xadd for a virtual viewport instance               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/07/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t GraphicViewPortClass::Get_XAdd(void) const
{
	return(XAdd);
}
/***************************************************************************
 * GVPC::GET_XPOS -- Get the x pos of the VP on the Video                  *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     WORD the x offset to VideoBufferClass								*
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline int32_t GraphicViewPortClass::Get_XPos(void) const
{
	return(XPos);
}


/***************************************************************************
 * GVPC::GET_YPOS -- Get the y pos of the VP on the video                  *
 *                                                                         *
 * INPUT:		none                                                        *
 *                                                                         *
 * OUTPUT:     WORD the x offset to VideoBufferClass								*
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline int32_t GraphicViewPortClass::Get_YPos(void) const
{
	return(YPos);
}

/***************************************************************************
 * GVPC::GET_GRAPHIC_BUFFER -- Get the graphic buffer of the VP.            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * HISTORY:                                                                *
 *   08/22/1994 SKB : Created.                                             *
 *=========================================================================*/
inline GraphicBufferClass *GraphicViewPortClass::Get_Graphic_Buffer(void)
{
	return (GraphicBuff);
}

/***************************************************************************
 * GVPC::SIZE_OF_REGION -- stub to call curr graphic mode Size_Of_Region	*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   03/01/1995 BWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Size_Of_Region(int32_t w, int32_t h)
{
	return Buffer_Size_Of_Region(this, w, h);
}


/***************************************************************************
 * GVPC::PUT_PIXEL -- stub to call curr graphic mode Put_Pixel					*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline void	GraphicViewPortClass::Put_Pixel(int32_t x, int32_t y, uint8_t color)
{

	if (Lock()){
		Buffer_Put_Pixel(this, x, y, color);
	}
	Unlock();


}

/***************************************************************************
 * GVPC::GET_PIXEL -- stub to call curr graphic mode Get_Pixel          	*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Get_Pixel(int32_t x, int32_t y)
{
	int32_t		return_code=0;

	if (Lock()){
		return_code=(Buffer_Get_Pixel(this, x, y));
	}
	Unlock();
	return(return_code);

}

/***************************************************************************
 * GVPC::CLEAR -- stub to call curr graphic mode Clear	                  *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline void	GraphicViewPortClass::Clear(uint8_t color)
{
	if (Lock()){
		Buffer_Clear(this, color);
	}
	Unlock();

}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 1 to call curr graphic mode To_Buffer				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::To_Buffer(int32_t x, int32_t y, int32_t w, int32_t h, void *buff, int32_t size)
{
	int32_t	return_code=0;
	if (Lock()){
		return_code = (Buffer_To_Buffer(this, x, y, w, h, buff, size));
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 2 to call curr graphic mode To_Buffer 				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::To_Buffer(int32_t x, int32_t y, int32_t w, int32_t h, BufferClass *buff)
{
	int32_t	return_code=0;
	if (Lock()){
		return_code = (Buffer_To_Buffer(this, x, y, w, h, buff->Get_Buffer(), buff->Get_Size()));
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::TO_BUFFER -- stub 3 to call curr graphic mode To_Buffer 				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::To_Buffer(BufferClass *buff)
{
	int32_t	return_code=0;
	if (Lock()){
		return_code = (Buffer_To_Buffer(this, 0, 0, Width, Height, buff->Get_Buffer(), buff->Get_Size()));
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::BLIT -- stub 1 to call curr graphic mode Blit to GVPC					*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline HRESULT	GraphicViewPortClass::Blit(	GraphicViewPortClass& dest, int32_t x_pixel, int32_t y_pixel, int32_t dx_pixel,
				int32_t dy_pixel, int32_t pixel_width, int32_t pixel_height, int32_t trans)
{
	HRESULT		return_code=0;

	if ( IsVideoSurface && dest.IsVideoSurface ){
	return(Video_Linear_Blit_To_Linear( dest, XPos+x_pixel, YPos+y_pixel
								, dest.Get_XPos()+dx_pixel, dest.Get_YPos()+dy_pixel
								, pixel_width, pixel_height, trans));
	} else {

		if (Lock()){
			if (dest.Lock()){
				return_code=(Linear_Blit_To_Linear(this, &dest, x_pixel, y_pixel
														, dx_pixel, dy_pixel
														, pixel_width, pixel_height, trans));
			}
			dest.Unlock();
		}
		Unlock();
	}

	return ( return_code );
}

/***************************************************************************
 * GVPC::BLIT -- Stub 2 to call curr graphic mode Blit to GVPC					*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline HRESULT	GraphicViewPortClass::Blit(	GraphicViewPortClass& dest, int32_t dx, int32_t dy, int32_t trans)
{
	HRESULT		return_code=0;


	if ( IsVideoSurface && dest.IsVideoSurface ){
	return(Video_Linear_Blit_To_Linear( dest, XPos, YPos
								, dest.Get_XPos()+dx, dest.Get_YPos()+dy
								, Width, Height, trans));
	} else {

		if (Lock()){
			if (dest.Lock()){
				return_code=(Linear_Blit_To_Linear(this, &dest, 0, 0
														, dx, dy
														, Width, Height, trans));
			}
			dest.Unlock();
		}
		Unlock();
	}

	return (return_code);

}

/***************************************************************************
 * GVPC::BLIT -- stub 3 to call curr graphic mode Blit to GVPC					*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline HRESULT	GraphicViewPortClass::Blit(	GraphicViewPortClass& dest, int32_t trans)
{
	HRESULT		return_code=0;


	if ( IsVideoSurface && dest.IsVideoSurface ){
	return(Video_Linear_Blit_To_Linear( dest, XPos, YPos
								, dest.Get_XPos(), dest.Get_YPos()
								, MAX( Width, dest.Get_Width())
								, MAX( Height, dest.Get_Height())
								, trans));
	} else {

		if (Lock()){
			if (dest.Lock()){
				return_code=(Linear_Blit_To_Linear(this, &dest, 0, 0
														, 0, 0
														, Width, Height, trans));
			}
			dest.Unlock();
		}
		Unlock();
	}

	return (return_code);

}


/***************************************************************************
 * GVPC::SCALE -- stub 1 to call curr graphic mode Scale to GVPC				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Scale(	GraphicViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
					int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, int32_t trans, char *remap)
{
	int32_t	return_code=0;
	if (Lock()){
		if (dest.Lock()){
			return_code = (Linear_Scale_To_Linear(this, &dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h, trans, remap));
		}
		dest.Unlock();
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::SCALE -- stub 2 to call curr graphic mode Scale to GVPC				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Scale(	GraphicViewPortClass &dest, int32_t src_x, int32_t src_y, int32_t dst_x,
					int32_t dst_y, int32_t src_w, int32_t src_h, int32_t dst_w, int32_t dst_h, char *remap)
{
	int32_t	return_code=0;
	if (Lock()){
		if (dest.Lock()){
			return_code = (Linear_Scale_To_Linear(this, &dest, src_x, src_y, dst_x, dst_y, src_w, src_h, dst_w, dst_h, FALSE, remap));
		}
		dest.Unlock();
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::SCALE -- stub 3 to call curr graphic mode Scale to GVPC				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Scale(	GraphicViewPortClass &dest, int32_t trans, char *remap)
{
	int32_t	return_code=0;
	if (Lock()){
		if (dest.Lock()){
			return_code = (Linear_Scale_To_Linear(this,	&dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(), dest.Get_Height(), trans, remap));
		}
		dest.Unlock();
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::SCALE -- stub 4 to call curr graphic mode Scale to GVPC				*
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/06/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t	GraphicViewPortClass::Scale(	GraphicViewPortClass &dest, char *remap)
{
	int32_t	return_code=0;
	if (Lock()){
		if (dest.Lock()){
			return_code = (Linear_Scale_To_Linear(this, &dest, 0, 0, 0, 0, Width, Height, dest.Get_Width(), dest.Get_Height(), FALSE, remap));
		}
		dest.Unlock();
	}
	Unlock();
	return ( return_code );
}
/***************************************************************************
 * GVPC::PRINT -- stub func to print a text string                         *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/17/1995 PWG : Created.                                             *
 *=========================================================================*/
inline uint32_t	GraphicViewPortClass::Print(char const *str, int32_t x, int32_t y, int32_t fcol, int32_t bcol)
{
	uint32_t	return_code=0;
	if (Lock()){
		return_code = (Buffer_Print(this, str, x, y, fcol, bcol));
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::PRINT -- Stub function to print an integer                        *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
inline uint32_t	GraphicViewPortClass::Print(int32_t num, int32_t x, int32_t y, int32_t fcol, int32_t bcol)
{
	char str[17];

	uint32_t	return_code=0;
	if (Lock()){
		sprintf(str, "%d", num);
		return_code = Buffer_Print(this, str, x, y, fcol, bcol);
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::PRINT -- Stub function to print a short to a graphic viewport     *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
inline uint32_t	GraphicViewPortClass::Print(int16_t num, int32_t x, int32_t y, int32_t fcol, int32_t bcol)
{
	char str[17];

	uint32_t	return_code=0;
	if (Lock()){
		sprintf(str, "%d", num);
		return_code = Buffer_Print(this, str, x, y, fcol, bcol);
	}
	Unlock();
	return ( return_code );
}

/***************************************************************************
 * GVPC::PRINT -- stub function to print a long on a graphic view port     *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
/***************************************************************************
 * GVPC::DRAW_STAMP -- stub function to draw a tile on a graphic view port *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *=========================================================================*/
inline void GraphicViewPortClass::Draw_Stamp(void const *icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const *remap)
{
	if (Lock()){
		Buffer_Draw_Stamp(this, icondata, icon, x_pixel, y_pixel, remap);
	}
	Unlock();
}



/***************************************************************************
 * GVPC::DRAW_STAMP -- stub function to draw a tile on a graphic view port *
 *                     This version clips the tile to a window             *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *    07/31/1995 BWG : Created.                                            *
 *=========================================================================*/
extern int32_t IconCacheAllowed;
inline void GraphicViewPortClass::Draw_Stamp(void const * icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const * remap, int32_t clip_window)
{
	int32_t	cache_index=-1;

   int32_t drewit = 0;
	if (IconCacheAllowed){
		if (IsVideoSurface){
			if (!remap){
				cache_index = Is_Icon_Cached(icondata,icon);
			}

			if (cache_index != -1){
				if (CachedIcons[cache_index].Get_Is_Cached()  ){
					CachedIcons[cache_index].Draw_It (GraphicBuff->Get_Video_Surface() , x_pixel, y_pixel,
																WindowList[clip_window][WINDOWX] + XPos,
																WindowList[clip_window][WINDOWY] +YPos,
																WindowList[clip_window][WINDOWWIDTH],
																WindowList[clip_window][WINDOWHEIGHT]);
					CachedIconsDrawn++;
               drewit = 1;
				}
			}
		}
	}


   if (drewit == 0) {
		if (Lock()){
			UnCachedIconsDrawn++;
			Buffer_Draw_Stamp_Clip(this, icondata, icon, x_pixel, y_pixel, remap, WindowList[clip_window][WINDOWX], WindowList[clip_window][WINDOWY], WindowList[clip_window][WINDOWWIDTH], WindowList[clip_window][WINDOWHEIGHT]);
		}
   }
	Unlock();
}


/***************************************************************************
 * GVPC::DRAW_LINE -- Stub function to draw line in Graphic Viewport Class *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
inline void GraphicViewPortClass::Draw_Line(int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color)
{
	if (Lock()){
		Buffer_Draw_Line(this, sx, sy, dx, dy, color);
	}
	Unlock();
}

/***************************************************************************
 * GVPC::FILL_RECT -- Stub function to fill rectangle in a GVPC            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
inline void GraphicViewPortClass::Fill_Rect(int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color)
{
	if (  IsVideoSurface
			&& ( (dx-sx) * (dy-sy) >= (32*32) )
			&& GraphicBuff->Get_Video_Surface()->CanBlit()){
		RECT	dest_rectangle;

		dest_rectangle.left	=sx+XPos;
		dest_rectangle.top	=sy+YPos;
		dest_rectangle.right	=dx+XPos;
		dest_rectangle.bottom=dy+YPos;

		if (dest_rectangle.left<XPos){
			dest_rectangle.left=XPos;
		}

		if (dest_rectangle.right >= Width + XPos){
			dest_rectangle.right = Width +XPos -1;
		}

		if (dest_rectangle.top<YPos){
			dest_rectangle.top=YPos;
		}

		if (dest_rectangle.bottom >= Height + YPos){
			dest_rectangle.bottom = Height + YPos -1;
		}

		if (dest_rectangle.left >= dest_rectangle.right)  return;
		if (dest_rectangle.top  >= dest_rectangle.bottom) return;

		dest_rectangle.right++;
		dest_rectangle.bottom++;

		GraphicBuff->Get_Video_Surface()->FillRect(&dest_rectangle, color);
	} else {
		if (Lock()){
			Buffer_Fill_Rect(this, sx, sy, dx, dy, color);
			Unlock();
		}
	}
}


/***************************************************************************
 * GVPC::REMAP -- Stub function to remap a GVPC                            *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/16/1995 PWG : Created.                                             *
 *=========================================================================*/
inline void GraphicViewPortClass::Remap(int32_t sx, int32_t sy, int32_t width, int32_t height, void *remap)
{
	if (Lock()){
		Buffer_Remap(this, sx, sy, width, height, remap);
	}
	Unlock();
}


inline void GraphicViewPortClass::Fill_Quad(void *span_buff, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
							int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t color)
{
	if (Lock()){
		Buffer_Fill_Quad(this, span_buff, x0, y0, x1, y1, x2, y2, x3, y3, color);
	}
	Unlock();
}

/***************************************************************************
 * GVPC::REMAP -- Short form to remap an entire graphic view port          *
 *                                                                         *
 * INPUT:		BYTE * to the remap table to use										*
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline void GraphicViewPortClass::Remap(void *remap)
{
	if (Lock()){
		Buffer_Remap(this, 0, 0, Width, Height, remap);
	}
	Unlock();
}

inline int32_t GraphicViewPortClass::Get_Pitch(void) const
{
	return(Pitch);
}
/*=========================================================================*/
/* The following BufferClass functions are defined here because they act	*/
/*		on graphic viewports.																*/
/*=========================================================================*/


/***************************************************************************
 * BUFFER_TO_PAGE -- Generic 'c' callable form of Buffer_To_Page           *
 *                                                                         *
 * INPUT: 																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   01/12/1995 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t Buffer_To_Page(int32_t x, int32_t y, int32_t w, int32_t h, void *Buffer, GraphicViewPortClass &view)
{
	int32_t	return_code=0;
	if (view.Lock()){
		return_code = (Buffer_To_Page(x, y, w, h, Buffer, &view));
	}
	view.Unlock();
	return ( return_code );
}

/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h 		*
 *                                                                         *
 * INPUT:		int	width		- the width of copy region							*
 *					int	height	- the height of copy region						*
 *					GVPC&	dest		- virtual viewport to copy to						*
 *                                                                         *
 * OUTPUT:		none                                                        *
 *																									*
 * WARNINGS:	x and y position are the upper left corner of the dest		*
 *						viewport																	*
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(int32_t w, int32_t h, GraphicViewPortClass &view)
{
	int32_t	return_code=0;
	if (view.Lock()){
		return_code = (Buffer_To_Page(0, 0, w, h, Buffer, &view));
	}
	view.Unlock();
	return ( return_code );
}
/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable w, h 		*
 *                                                                         *
 * INPUT:		GVPC&	dest		- virtual viewport to copy to						*
 *                                                                         *
 * OUTPUT:		none                                                        *
 *																									*
 * WARNINGS:	x and y position are the upper left corner of the dest		*
 *						viewport.  width and height are assumed to be the			*
 *						viewport's width and height.										*
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(GraphicViewPortClass &view)
{
	int32_t	return_code=0;
	if (view.Lock()){
		return_code = (Buffer_To_Page(0, 0, view.Get_Width(), view.Get_Height(), Buffer, &view));
	}
	view.Unlock();
	return ( return_code );
}
/***************************************************************************
 * BC::TO_PAGE -- Copys a buffer class to a page with definable x, y, w, h *
 *                                                                         *
 * INPUT:	int	x			- x pixel on viewport to copy from					*
 *				int	y			- y pixel on viewport to copy from					*
 *				int	width		- the width of copy region								*
 *				int	height	- the height of copy region							*
 *				GVPC&	dest		- virtual viewport to copy to							*
 *                                                                         *
 * OUTPUT:	none                                                           *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/01/1994 PWG : Created.                                             *
 *=========================================================================*/
inline int32_t BufferClass::To_Page(int32_t x, int32_t y, int32_t w, int32_t h, GraphicViewPortClass &view)
{
	int32_t	return_code=0;
	if (view.Lock()){
		return_code = (Buffer_To_Page(x, y, w, h, Buffer, &view));
	}
	view.Unlock();
	return ( return_code );
}


#endif
