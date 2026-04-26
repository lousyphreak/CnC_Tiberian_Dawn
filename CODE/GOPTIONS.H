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

/* $Header:   F:\projects\c&c\vcs\code\goptions.h_v   2.19   16 Oct 1995 16:46:26   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : OPTIONS.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : June 8, 1994                                                 *
 *                                                                                             *
 *                  Last Update : June 8, 1994   [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef GOPTIONS_H
#define GOPTIONS_H

#include "OPTIONS.H"
#include "GADGET.H"


class GameOptionsClass : public OptionsClass {
		enum GameOptionsButtonEnum {
			BUTTON_LOAD=1,
			BUTTON_SAVE,
			BUTTON_DELETE,
			BUTTON_GAME,
			BUTTON_QUIT,
			BUTTON_RESUME,
			BUTTON_RESTATE,

			BUTTON_COUNT,
		};

		enum GameOptionsEnum {
		};

	public:
		GameOptionsClass(void): OptionsClass () { };
		void  Adjust_Variables_For_Resolution(void);
		void Process(void);

	private:
		int	OptionWidth;
		int 	OptionHeight;
		int 	OptionX;
		int 	OptionY;
		int 	ButtonWidth;
		int 	OButtonHeight;
		int 	CaptionYPos;
		int 	ButtonY;
		int 	Border1Len;
		int 	Border2Len;
		int 	ButtonResumeY;
};

#endif
