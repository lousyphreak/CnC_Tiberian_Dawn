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
 *                 Project Name : Part of the WINDOWS Library              *
 *                                                                         *
 *                    File Name : WINDOWS.H                                *
 *                                                                         *
 *                   Programmer : Barry W. Green                           *
 *                                                                         *
 *                   Start Date : February 16, 1995                        *
 *                                                                         *
 *                  Last Update : February 16, 1995 [BWG]                  *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WINDOWS_H
#define WINDOWS_H

/*=========================================================================*/
/* The following prototypes are for the file: WINHIDE.CPP						*/
/*=========================================================================*/
void Window_Hide_Mouse(int32_t window);
void Window_Show_Mouse(void);

/*=========================================================================*/
/* The following prototypes are for the file: WINDOWS.CPP						*/
/*=========================================================================*/
void Standard_More_Prompt(char const *prompt, int32_t space, int32_t fcolor, int32_t bcolor);
void Set_More_Prompt(char const *prompt, int32_t space, int32_t fcolor, int32_t bcolor);
void Set_More_On(void);
void Set_More_Off(void);
int32_t Change_Window(int32_t windnum);
int32_t Change_New_Window(int32_t windnum);
void New_Window(void);
void Window_Int_Print(int32_t num);
void Window_Print(char const string[], ...);

/*
**	The WindowList[][8] array contains the following elements.  Use these
**	defines when accessing the WindowList.
*/
typedef enum {
	WINDOWX,			// X byte position of left edge.
	WINDOWY,			// Y pixel position of top edge.
	WINDOWWIDTH,	// Width in bytes of the window.
	WINDOWHEIGHT,	// Height in pixels of the window.
	WINDOWFCOL,		// Default foreground color.
	WINDOWBCOL,		// Default background color.
	WINDOWCURSORX,	// Current cursor X position (in rows).
	WINDOWCURSORY,	// Current cursor Y position (in lines).
	WINDOWPADDING=0x1000
} WindowIndexType;

extern int32_t WindowList[][8];
extern int32_t WindowColumns;
extern int32_t WindowLines;
extern int32_t WindowWidth;
extern uint32_t WinB;
extern uint32_t WinC;
extern uint32_t WinX;
extern uint32_t WinY;
extern uint32_t WinCx;
extern uint32_t WinCy;
extern uint32_t WinH;
extern uint32_t WinW;
extern uint32_t Window;

extern int32_t MoreOn;
extern char const * TXT_MoreText;

extern void (*Window_More_Ptr)(char const *, int32_t, int32_t, int32_t);

#endif //WINDOWS_H
