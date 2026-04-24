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

/***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood 32 Bit Library                                      *
 *                                                                                             *
 *                    File Name : MOUSE.H                                                      *
 *                                                                                             *
 *                   Programmer : Philip W. Gorrow                                             *
 *                                                                                             *
 *                   Start Date : 12/12/95                                                     *
 *                                                                                             *
 *                  Last Update : December 12, 1995 [PWG]                                      *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WW_MOUSE_H
#define WW_MOUSE_H

#include <GBUFFER.H>
void  Mouse_Shadow_Buffer(void *thisptr, GraphicViewPortClass *srcdst, void *buffer, int32_t x, int32_t y, int32_t hotx, int32_t hoty, int32_t store);
void  Draw_Mouse(void *thisptr, GraphicViewPortClass *srcdst, int32_t x, int32_t y);
void *  ASM_Set_Mouse_Cursor(void * thisptr, int32_t hotspotx, int32_t hotspoty, void *cursor);

class WWMouseClass {
	public:
		WWMouseClass(GraphicViewPortClass *scr, int32_t mouse_max_width, int32_t mouse_max_height);
		~WWMouseClass();
		void *Set_Cursor(int32_t xhotspot, int32_t yhotspot, void *cursor);
		void Process_Mouse(void);
		void Hide_Mouse(void);
		void Show_Mouse(void);
		void Conditional_Hide_Mouse(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
		void Conditional_Show_Mouse(void);
		int32_t Get_Mouse_State(void);
		int32_t Get_Mouse_X(void);
		int32_t Get_Mouse_Y(void);
		void Get_Mouse_XY(int32_t &x, int32_t &y);
		//
		// The following two routines can be used to render the mouse onto a graphicbuffer
		// other than the hidpage.
		//
		void Draw_Mouse(GraphicViewPortClass *scr);
		void Erase_Mouse(GraphicViewPortClass *scr, int32_t forced = FALSE);

		void Set_Cursor_Clip(void);
		void Clear_Cursor_Clip(void);

	private:
		enum 	{
			CONDHIDE		= 1,
			CONDHIDDEN 	= 2,
		};
		void Low_Hide_Mouse(void);
		void Low_Show_Mouse(int32_t x, int32_t y);

		char						*MouseCursor;	// pointer to the mouse cursor in memory
		int32_t						MouseXHot;		// X hot spot of the current mouse cursor
		int32_t						MouseYHot;		// Y hot spot of the current mouse cursor
		int32_t						CursorWidth;	// width of the mouse cursor in pixels
		int32_t						CursorHeight;	// height of the mouse cursor in pixels

		char						*MouseBuffer;	// pointer to background buffer in memory
		int32_t						MouseBuffX;		// pixel x mouse buffer was preserved at
		int32_t						MouseBuffY;		// pixel y mouse buffer was preserved at
		int32_t						MaxWidth;		// maximum width of mouse background buffer
		int32_t						MaxHeight;		// maximum height of mouse background buffer

		int32_t						MouseCXLeft;	// left x pos if conditional hide mouse in effect
		int32_t						MouseCYUpper;	// upper y pos if conditional hide mouse in effect
		int32_t						MouseCXRight;	// right x pos if conditional hide mouse in effect
		int32_t						MouseCYLower;	// lower y pos if conditional hide mouse in effect
		char						MCFlags;			// conditional hide mouse flags
		char						MCCount;			// nesting count for conditional hide mouse

		GraphicViewPortClass	*Screen;			// pointer to the surface mouse was init'd with
		char *					PrevCursor;		// pointer to previous cursor shape
		int32_t						MouseUpdate;
		int32_t						State;

		char						*EraseBuffer;	// Buffer which holds background to restore to hidden page
		int32_t						EraseBuffX;		// X position of the hidden page background
		int32_t						EraseBuffY;		// Y position of the hidden page background
		int32_t						EraseBuffHotX;	// X position of the hidden page background
		int32_t						EraseBuffHotY;	// Y position of the hidden page background

		int32_t						EraseFlags;		// Records whether mutex has been released

		friend void  Mouse_Shadow_Buffer(void *thisptr, GraphicViewPortClass *srcdst, void *buffer, int32_t x, int32_t y, int32_t hotx, int32_t hoty, int32_t store);
		friend void  Draw_Mouse(void *thisptr, GraphicViewPortClass *srcdst, int32_t x, int32_t y);
		friend void *  ASM_Set_Mouse_Cursor(void *thisptr, int32_t hotspotx, int32_t hotspoty, void *cursor);

};

void Hide_Mouse(void);
void Show_Mouse(void);
void Conditional_Hide_Mouse(int32_t x1, int32_t y1, int32_t x2, int32_t y2);
void Conditional_Show_Mouse(void);
int32_t Get_Mouse_State(void);
void *Set_Mouse_Cursor(int32_t hotx, int32_t hoty, void *cursor);
int32_t Get_Mouse_X(void);
int32_t Get_Mouse_Y(void);

#endif
