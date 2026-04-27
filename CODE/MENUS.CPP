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

/* $Header:   F:\projects\c&c\vcs\code\menus.cpv   2.17   16 Oct 1995 16:50:48   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MENUS.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Phil W. Gorrow                                               *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : May 17, 1995 [BRR]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Main_Menu -- Menu processing                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"
#include "SDLINPUT.H"

/***************************************************************************
 * Main_Menu -- Menu processing                                            *
 *                                                                         *
 * INPUT:                                                                  *
 *		none.																						*
 *                                                                         *
 * OUTPUT:                                                                 *
 *		index of item selected, -1 if time out											*
 *                                                                         *
 * WARNINGS:                                                               *
 *		none.																						*
 *                                                                         *
 * HISTORY:                                                                *
 *   05/17/1995 BRR : Created.                                             *
 *=========================================================================*/
int Main_Menu(uint32_t timeout)
{
	enum {
		D_DIALOG_W = 152*2,
		D_DIALOG_H = 136*2,
		D_DIALOG_X = 85*2,
		D_DIALOG_Y = 0,
		D_DIALOG_CX = D_DIALOG_X + (D_DIALOG_W / 2),

		D_START_W = 125*2,
		D_START_H = 9*2,
		D_START_X = 98*2,
		D_START_Y = 35*2,

#ifdef BONUS_MISSIONS
		D_BONUS_W = 125*2,
		D_BONUS_H = 9*2,
		D_BONUS_X = 98*2,
		D_BONUS_Y = 0,
#endif	//BONUS_MISSIONS

		D_INTERNET_W = 125*2,
		D_INTERNET_H = 9*2,
		D_INTERNET_X = 98*2,
		D_INTERNET_Y = 36*2,

		D_LOAD_W = 125*2,
		D_LOAD_H = 9*2,
		D_LOAD_X = 98*2,
		D_LOAD_Y = 53*2,

		D_MULTI_W = 125*2,
		D_MULTI_H = 9*2,
		D_MULTI_X = 98*2,
		D_MULTI_Y = 71*2,

		D_INTRO_W = 125*2,
		D_INTRO_H = 9*2,
		D_INTRO_X = 98*2,
		D_INTRO_Y = 89*2,
#if (GERMAN | FRENCH)
		D_EXIT_W = 83*2,
#else
		D_EXIT_W = 63*2,
#endif
		D_EXIT_H = 9*2,
#if (GERMAN | FRENCH)
		D_EXIT_X = 118*2,
#else
		D_EXIT_X = 128*2,
#endif
		D_EXIT_Y = 111*2,

	};

#ifdef NEWMENU
	int starty = 25*2;
#endif

	enum {
#ifdef NEWMENU
		BUTTON_EXPAND=100*2,
		BUTTON_START,
#ifdef BONUS_MISSIONS
		BUTTON_BONUS,
#endif	//BONUS_MISSIONS
		BUTTON_INTERNET,
#else
		BUTTON_START=100*2,
#endif
		BUTTON_LOAD,
		BUTTON_MULTI,
		BUTTON_INTRO,
		BUTTON_EXIT,
	};

#ifdef NEWMENU
	bool expansions = Expansion_Present();
#endif
	KeyNumType input;								// input from user
	int retval;										// return value
	int curbutton;
#ifdef NEWMENU
#ifdef BONUS_MISSIONS
	TextButtonClass *buttons[8];
#else
	TextButtonClass *buttons[7];
#endif	//BONUS_MISSIONS
#else
	TextButtonClass *buttons[5];
#endif
	uint32_t starttime;

	ControlClass *commands = NULL;				// the button list

#ifdef NEWMENU
#ifdef BONUS_MISSIONS
	int ystep = 13*2;
#else
	int ystep = 15*2;
#endif	//BONUS_MISSIONS

	if (expansions) ystep -= 2*2;
	TextButtonClass expandbtn (BUTTON_EXPAND, TXT_NEW_MISSIONS,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_START_X, starty, D_START_W, D_START_H);
	if (expansions) starty += ystep;

	TextButtonClass startbtn (BUTTON_START, TXT_START_NEW_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_START_X, starty, D_START_W, D_START_H);
	starty += ystep;

#ifdef BONUS_MISSIONS
	TextButtonClass bonusbtn (BUTTON_BONUS, TXT_BONUS_MISSIONS,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_BONUS_X, starty, D_BONUS_W, D_BONUS_H);
	starty += ystep;
#endif	//BONUS_MISSIONS

	TextButtonClass loadbtn (BUTTON_LOAD, TXT_LOAD_MISSION,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_LOAD_X, starty, D_LOAD_W, D_LOAD_H);
	starty += ystep;
#else

	TextButtonClass startbtn (BUTTON_START, TXT_START_NEW_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_START_X, D_START_Y, D_START_W, D_START_H);

	TextButtonClass loadbtn (BUTTON_LOAD, TXT_LOAD_MISSION,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_LOAD_X, D_LOAD_Y, D_LOAD_W, D_LOAD_H);

#endif


#ifdef DEMO
	TextButtonClass multibtn (BUTTON_MULTI, TXT_ORDER_INFO,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_MULTI_X, D_MULTI_Y, D_MULTI_W, D_MULTI_H);
#else

#ifdef NEWMENU
	TextButtonClass multibtn (BUTTON_MULTI, TXT_MULTIPLAYER_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_MULTI_X, starty, D_MULTI_W, D_MULTI_H);
	starty += ystep;

	//TextButtonClass internetbutton(BUTTON_INTERNET, TXT_INTERNET,
	//	TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
	//	D_INTERNET_X, starty, D_INTERNET_W, D_INTERNET_H);
	//starty += ystep;
#else
	TextButtonClass multibtn (BUTTON_MULTI, TXT_MULTIPLAYER_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_MULTI_X, D_MULTI_Y, D_MULTI_W, D_MULTI_H);
#endif
#endif

#ifdef NEWMENU
#ifdef DEMO
	TextButtonClass introbtn (BUTTON_INTRO, TXT_JUST_INTRO,
#else	//DEMO
	TextButtonClass introbtn (BUTTON_INTRO, TXT_INTRO,
#endif	//DEMO
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_INTRO_X, starty, D_INTRO_W, D_INTRO_H);
	starty += ystep;

	TextButtonClass exitbtn (BUTTON_EXIT, TXT_EXIT_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
#if (GERMAN | FRENCH)
		//D_EXIT_X, starty);
		D_EXIT_X, starty, D_EXIT_W, D_EXIT_H);
#else
		D_EXIT_X, starty, D_EXIT_W, D_EXIT_H);
#endif
	starty += ystep;

#else

#ifdef DEMO
	TextButtonClass introbtn (BUTTON_INTRO, TXT_JUST_INTRO,
#else	//DEMO
	TextButtonClass introbtn (BUTTON_INTRO, TXT_INTRO,
#endif	//DEMO
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		D_INTRO_X, D_INTRO_Y, D_INTRO_W, D_INTRO_H);

	TextButtonClass exitbtn (BUTTON_EXIT, TXT_EXIT_GAME,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
#if (GERMAN | FRENCH)
		//D_EXIT_X, D_EXIT_Y);
		D_EXIT_X, D_EXIT_Y, D_EXIT_W, D_EXIT_H);
#else
		D_EXIT_X, D_EXIT_Y, D_EXIT_W, D_EXIT_H);
#endif
#endif

	/*
	**	Initialize
	*/
	Set_Logic_Page(SeenBuff);
	Keyboard::Clear();
	starttime = TickCount.Time();

	/*
	**	Create the list
	*/
	commands = &startbtn;
#ifdef NEWMENU
	if (expansions) {
		expandbtn.Add_Tail(*commands);
	}
#endif
#ifdef BONUS_MISSIONS
	bonusbtn.Add_Tail(*commands);
#endif	//BONUS_MISSIONS


	loadbtn.Add_Tail(*commands);
	multibtn.Add_Tail(*commands);
	introbtn.Add_Tail(*commands);
	exitbtn.Add_Tail(*commands);

	/*
	**	Fill array of button ptrs
	*/
#ifdef NEWMENU
	if (expansions) {
		curbutton = 0;
	} else {
		curbutton = 1;
	}
	int butt = 0;

	buttons[butt++] = &expandbtn;
	buttons[butt++] = &startbtn;
#ifdef BONUS_MISSIONS
	buttons[butt++] = &bonusbtn;
#endif	//BONUS_MISSIONS
	buttons[butt++] = &loadbtn;
	buttons[butt++] = &multibtn;
	buttons[butt++] = &introbtn;
	buttons[butt++] = &exitbtn;
#else
	curbutton = 0;
	buttons[0] = &startbtn;
	buttons[1] = &loadbtn;
	buttons[2] = &multibtn;
	buttons[3] = &introbtn;
	buttons[4] = &exitbtn;
#endif
	buttons[curbutton]->Turn_On();

	Keyboard::Clear();

	Fancy_Text_Print(TXT_NONE, 0, 0, CC_GREEN, TBLACK, TPF_CENTER|TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_NOSHADOW);
	while (Get_Mouse_State() > 0) Show_Mouse();

	/*
	**	Main Processing Loop.
	*/
	bool display = true;
	bool process = true;
	while (process) {

		/*
		** If we have just received input focus again after running in the background then
		** we need to redraw.
		*/
		if (AllSurfaces.SurfacesRestored){
			AllSurfaces.SurfacesRestored=FALSE;
			display=TRUE;
		}

		/*
		**	If timeout expires, bail
		*/
		if (timeout && TickCount.Time() - starttime > timeout) {
			retval = -1;
			process = false;
		}

		/*
		**	Invoke game callback.
		*/
		Call_Back();

		/*
		**	Refresh display if needed.
		*/
		if (display) {

			/*
			**	Load the background picture.
			*/
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);

			/*
			**	Display the title and text overlay for the menu.
			*/
			Set_Logic_Page(HidPage);
			Dialog_Box(D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W, D_DIALOG_H);
			Draw_Caption (TXT_NONE, D_DIALOG_X, D_DIALOG_Y, D_DIALOG_W);
#ifdef VIRGIN_CHEAT_KEYS
#ifdef DEMO
			Version_Number();
			Fancy_Text_Print("Demo%s", D_DIALOG_X+D_DIALOG_W-5*2, D_DIALOG_Y+D_DIALOG_H-10*2, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, VersionText);
#else
			Fancy_Text_Print("V.%d%s", D_DIALOG_X+D_DIALOG_W-5*2, D_DIALOG_Y+D_DIALOG_H-10*2, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
#endif
//			Fancy_Text_Print("V.%d%s%02d", D_DIALOG_X+D_DIALOG_W-5, D_DIALOG_Y+D_DIALOG_H-10, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
#else
#ifdef DEMO
			Version_Number();
			Fancy_Text_Print("Demo%s", D_DIALOG_X+D_DIALOG_W-5*2, D_DIALOG_Y+D_DIALOG_H-10*2, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, VersionText);
#else
			Fancy_Text_Print("V.%d%s", D_DIALOG_X+D_DIALOG_W-5*2, D_DIALOG_Y+D_DIALOG_H-10*2, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText);
#endif
#endif

			/*
			**	Copy the menu to the visible page.
			*/
			Hide_Mouse();
			HidPage.Blit(SeenBuff);
			Show_Mouse();

			Set_Logic_Page(SeenBuff);
			startbtn.Draw_All();
			WWDraw_Flush_Present();
			if (ScreenWidth==320){
				ModeX_Blit (SeenBuff.Get_Graphic_Buffer());
			}
			display = false;
		}

		/*
		**	Get and process player input.
		*/
		Set_Logic_Page(SeenBuff);
		input = commands->Input();
		WWDraw_Flush_Present();
		switch (static_cast<int>(input)) {
#ifdef NEWMENU
			case (BUTTON_EXPAND | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
				process = false;
				break;

			case (BUTTON_INTERNET | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
				process = false;
				break;

#else
#define	BUTTON_EXPAND	BUTTON_START
#endif

			case (BUTTON_START | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
				process = false;
				break;

#ifdef BONUS_MISSIONS
			case (BUTTON_BONUS | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
				process = false;
				break;
#endif	//BONUS_MISSIONS

			case (BUTTON_LOAD | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
#ifdef DEMO
				retval += 1;
#endif	//DEMO
				process = false;
				break;

			case (BUTTON_MULTI | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
#ifdef DEMO
				retval += 1;
#endif	//DEMO
				process = false;
				break;

			case (BUTTON_INTRO | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
#ifdef DEMO
				retval += 1;
#endif	//DEMO
				process = false;
				break;

			case (BUTTON_EXIT | KN_BUTTON):
				retval = (input & 0x7FFF) - BUTTON_EXPAND;
#ifdef DEMO
				retval += 1;
#endif	//DEMO
				process = false;
				break;

			case KN_UP:
				buttons[curbutton]->Turn_Off();
				buttons[curbutton]->Flag_To_Redraw();
				curbutton--;
#ifdef NEWMENU
				if (expansions) {
					if (curbutton < 0) {
						curbutton = 6;
					}
				} else {
					if (curbutton < 1) {
						curbutton = 6;
					}
				}
#else
				if (curbutton < 0) {
					curbutton = 4;
				}
#endif
				buttons[curbutton]->Turn_On();
				buttons[curbutton]->Flag_To_Redraw();
				break;

			case KN_DOWN:
				buttons[curbutton]->Turn_Off();
				buttons[curbutton]->Flag_To_Redraw();
				curbutton++;
#ifdef NEWMENU
				if (curbutton > 6) {
					if (expansions) {
						curbutton = 0;
					} else {
						curbutton = 1;
					}
				}
#else
				if (curbutton > 4) {
					curbutton = 0;
				}
#endif
				buttons[curbutton]->Turn_On();
				buttons[curbutton]->Flag_To_Redraw();
				break;

			case KN_RETURN:
				buttons[curbutton]->IsPressed = true;
				buttons[curbutton]->Draw_Me(true);
				retval = curbutton;
				process = false;
				break;

			default:
				break;
		}

		if (process) {
			SDL_GameInput_Delay(1);
		}
	}
	return(retval);
}
