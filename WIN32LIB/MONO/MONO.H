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

/* $Header:   F:\projects\c&c\vcs\code\monoc.h_v   2.16   06 Sep 1995 16:29:02   JOE_BOSTIC  $ */
#include <cstdint>
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               *** 
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MONO.H                                                       *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : July 2, 1994                                                 *
 *                                                                                             *
 *                  Last Update : July 2, 1994   [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef MONOC_H
#define MONOC_H


class MonoClass {
		/*
		**	This is a private structure that is used to control which characters
		**	are used when a box is drawn. Line drawing on the monochrome screen is
		**	really made up of characters. This specifies which characters to use.
		*/
		typedef struct {
			uint8_t	UpperLeft;
			uint8_t	TopEdge;
			uint8_t	UpperRight;
			uint8_t	RightEdge;
			uint8_t	BottomRight;
			uint8_t	BottomEdge;
			uint8_t	BottomLeft;
			uint8_t	LeftEdge;
		} BoxDataType;

		/*
		**	Each cell is constructed of the actual character that is displayed and the
		**	attribute to use. This character pair is located at every position on the
		**	display (80 x 25). Since this cell pair can be represented by a "short"
		**	integer, certain speed optimizations are taken in the monochrome drawing
		**	code.
		*/
		typedef struct {
			uint8_t Character;			// Character to display.
			uint8_t Attribute;			// Attribute.
		} CellType;

		/*
		**	These private constants are used in the various monochrome operations.
		*/
		enum MonoClassPortEnums {
			CONTROL_PORT=0x03B4,			// CRTC control register.
			DATA_PORT=0x03B5,				// CRTC data register.
			COLUMNS=80,						// Number of columns.
			LINES=25,						// Number of lines.
			SIZE_OF_PAGE=LINES*COLUMNS*sizeof(CellType),	// Entire page size.
			DEFAULT_ATTRIBUTE=0x02		// Normal white on black color attribute.
		};

	public:
		enum MonoClassPageEnums {
			MAX_MONO_PAGES=16,	// Maximum RAM pages on mono card.
			SEGMENT=0xB000  		// Monochrome screen segment.
		};

		/*
		**	These are the various box styles that may be used.
		*/
		typedef enum BoxStyleType {
			SINGLE,				// Single thickness.
			DOUBLE_HORZ,		// Double thick on the horizontal axis.
			DOUBLE_VERT,		// Double thick on the vertical axis.
			DOUBLE,				// Double thickness.

			COUNT
		} BoxStyleType;

		MonoClass(void);
		~MonoClass(void);

		static void Enable(void) {Enabled = 1;};
		static void Disable(void) {Enabled = 0;};
		static int32_t Is_Enabled(void) {return Enabled;};
		static MonoClass * Get_Current(void) {return PageUsage[0];};

		void Draw_Box(int32_t x, int32_t y, int32_t w, int32_t h, char attrib=DEFAULT_ATTRIBUTE, BoxStyleType thick=SINGLE);
		void Set_Default_Attribute(char attrib) {Attrib = attrib;};
		void Clear(void);
		void Set_Cursor(int32_t x, int32_t y);
		void Print(char const *text);
		void Print(int32_t text);
		void Printf(char const *text, ...);
		void Printf(int32_t text, ...);
		void Text_Print(char const *text, int32_t x, int32_t y, char attrib=DEFAULT_ATTRIBUTE);
		void Text_Print(int32_t text, int32_t x, int32_t y, char attrib=DEFAULT_ATTRIBUTE);
		void View(void);
		int32_t Get_X(void) const {return X;};
		int32_t Get_Y(void) const {return Y;};

		/*
		**	Handles deep copies for the mono class objects. This performs what is essentially
		**	a screen copy.
		*/
		MonoClass & operator = (MonoClass const & );

		/*
		**	This merely makes a duplicate of the mono object into a newly created mono
		**	object.
		*/
		MonoClass (MonoClass const &);

	private:
		char	X;					// Cursor X position.
		char	Y;					// Cursor Y position.
		char	Attrib;			// Normal attribute to use if none specified.
		char	Page;				// The current page to write to.

		/*
		**	Helper functions to help with display operations.
		*/
		int32_t Offset(int32_t x=0, int32_t y=0) const {return (SIZE_OF_PAGE*Page) + sizeof(CellType)*(x + (y*COLUMNS));};
		void Scroll(int32_t lines);
		uint8_t * Mono_Bytes(int32_t x=0, int32_t y=0) const {return MonoSegment + Offset(x, y);}
		void Store_Cell(CellType &cell, int32_t x, int32_t y) {
			*reinterpret_cast<CellType *>(Mono_Bytes(x, y)) = cell;
		};

		/*
		**	This is the segment/selector of the monochrome screen.
		*/
		static uint8_t * MonoSegment;

		/*
		** This the the arrays of characters used for drawing boxes.
		*/
		static BoxDataType const CharData[4];

		/*
		**	This array contains pointers to the monochrome objects that are assigned
		**	to each of the monochrome pages. As the monochrome pages are made visible,
		**	they can be shuffled around between the actual locations. The first entry
		**	in this table is the one that is visible.
		*/
		static MonoClass * PageUsage[MAX_MONO_PAGES];

		/*
		**	If this is true, then monochrome output is allowed. It defaults to false
		**	so that monochrome output must be explicitly enabled.
		*/
		static int32_t Enabled;
};

void Mono_Set_Cursor(int32_t x, int32_t y);
int32_t Mono_Printf(char const *string, ...);
void Mono_Clear_Screen(void);
void Mono_Text_Print(void const *text, int32_t x, int32_t y, int32_t attrib);
void Mono_Draw_Rect(int32_t x, int32_t y, int32_t w, int32_t h, int32_t attrib, int32_t thick);
void Mono_Print(void const *text);
int32_t Mono_X(void);
int32_t Mono_Y(void);

#endif
