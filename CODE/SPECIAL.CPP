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

/* $Header:   F:\projects\c&c\vcs\code\special.cpv   1.4   16 Oct 1995 16:50:06   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : SPECIAL.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 05/27/95                                                     *
 *                                                                                             *
 *                  Last Update : May 27, 1995 [JLB]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#include "SDLINPUT.H"

#define	OPTION_WIDTH	236
#define	OPTION_HEIGHT	162
#define	OPTION_X			((320 - OPTION_WIDTH) / 2)
#define	OPTION_Y			(200 - OPTION_HEIGHT) / 2

void Special_Dialog(void)
{
	SpecialClass oldspecial = Special;
	GadgetClass * buttons = NULL;
	static struct {
		int Description;
		int Setting;
		CheckBoxClass * Button;
	} _options[] = {
//		{TXT_DEFENDER_ADVANTAGE, 0, 0},
		{TXT_SEPARATE_HELIPAD, 0, 0},
		{TXT_VISIBLE_TARGET, 0, 0},
		{TXT_TREE_TARGET, 0, 0},
		{TXT_MCV_DEPLOY, 0, 0},
		{TXT_SMART_DEFENCE, 0, 0},
		{TXT_THREE_POINT, 0, 0},
//		{TXT_TIBERIUM_GROWTH, 0, 0},
//		{TXT_TIBERIUM_SPREAD, 0, 0},
		{TXT_TIBERIUM_FAST, 0, 0},
		{TXT_ROAD_PIECES, 0, 0},
		{TXT_SCATTER, 0, 0},
		{TXT_SHOW_NAMES, 0, 0},
	};

	TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD|TPF_NOSHADOW, OPTION_X+5, OPTION_Y+OPTION_HEIGHT-15);
	TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD|TPF_NOSHADOW, OPTION_X+OPTION_WIDTH-50, OPTION_Y+OPTION_HEIGHT-15);
	buttons = &ok;
	cancel.Add(*buttons);

	const int option_count = static_cast<int>(sizeof(_options)/sizeof(_options[0]));
	for (int index = 0; index < option_count; index++) {
		_options[index].Button = new CheckBoxClass(100+index, OPTION_X+7, OPTION_Y+20+(index*10));
		if (_options[index].Button) {
			_options[index].Button->Add(*buttons);

			bool value = false;
			switch (_options[index].Description) {
				case TXT_SEPARATE_HELIPAD:
					value = Special.IsSeparate;
					break;

				case TXT_SHOW_NAMES:
					value = Special.IsNamed;
					break;

				case TXT_DEFENDER_ADVANTAGE:
					value = Special.IsDefenderAdvantage;
					break;

				case TXT_VISIBLE_TARGET:
					value = Special.IsVisibleTarget;
					break;

				case TXT_TREE_TARGET:
					value = Special.IsTreeTarget;
					break;

				case TXT_MCV_DEPLOY:
					value = Special.IsMCVDeploy;
					break;

				case TXT_SMART_DEFENCE:
					value = Special.IsSmartDefense;
					break;

				case TXT_THREE_POINT:
					value = Special.IsThreePoint;
					break;

				case TXT_TIBERIUM_GROWTH:
					value = Special.IsTGrowth;
					break;

				case TXT_TIBERIUM_SPREAD:
					value = Special.IsTSpread;
					break;

				case TXT_TIBERIUM_FAST:
					value = Special.IsTFast;
					break;

				case TXT_ROAD_PIECES:
					value = Special.IsRoad;
					break;

				case TXT_SCATTER:
					value = Special.IsScatter;
					break;
			}

			_options[index].Setting = value;
			if (value) {
				_options[index].Button->Turn_On();
			} else {
				_options[index].Button->Turn_Off();
			}
		}
	}

	Map.Override_Mouse_Shape(MOUSE_NORMAL);
	Set_Logic_Page(SeenBuff);
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

		if (GameToPlay == GAME_NORMAL) {
			Call_Back();
		} else {
			if (Main_Loop()) {
				process = false;
			}
		}

		if (display) {
			display = false;

			Hide_Mouse();
			Dialog_Box(OPTION_X, OPTION_Y, OPTION_WIDTH, OPTION_HEIGHT);
			Draw_Caption(TXT_SPECIAL_OPTIONS, OPTION_X, OPTION_Y, OPTION_WIDTH);

			for (int option_index = 0; option_index < option_count; option_index++) {
				Fancy_Text_Print(_options[option_index].Description, _options[option_index].Button->X+10, _options[option_index].Button->Y, CC_GREEN, TBLACK, TPF_6PT_GRAD|TPF_USE_GRAD_PAL|TPF_NOSHADOW);
			}
			buttons->Draw_All();
			Show_Mouse();
			WWDraw_Flush_Present();
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = buttons->Input();
		WWDraw_Flush_Present();
		switch (static_cast<int>(input)) {
			case KN_ESC:
			case 200|KN_BUTTON:
				process = false;
				for (int option_index = 0; option_index < option_count; option_index++) {
					switch (_options[option_index].Description) {
						case TXT_SEPARATE_HELIPAD:
							oldspecial.IsSeparate = _options[option_index].Setting;
							break;

						case TXT_SHOW_NAMES:
							oldspecial.IsNamed = _options[option_index].Setting;
							break;

						case TXT_DEFENDER_ADVANTAGE:
							oldspecial.IsDefenderAdvantage = _options[option_index].Setting;
							break;

						case TXT_VISIBLE_TARGET:
							oldspecial.IsVisibleTarget = _options[option_index].Setting;
							break;

						case TXT_TREE_TARGET:
							oldspecial.IsTreeTarget = _options[option_index].Setting;
							break;

						case TXT_MCV_DEPLOY:
							oldspecial.IsMCVDeploy = _options[option_index].Setting;
							break;

						case TXT_SMART_DEFENCE:
							oldspecial.IsSmartDefense = _options[option_index].Setting;
							break;

						case TXT_THREE_POINT:
							oldspecial.IsThreePoint = _options[option_index].Setting;
							break;

						case TXT_TIBERIUM_GROWTH:
							oldspecial.IsTGrowth = _options[option_index].Setting;
							break;

						case TXT_TIBERIUM_SPREAD:
							oldspecial.IsTSpread = _options[option_index].Setting;
							break;

						case TXT_TIBERIUM_FAST:
							oldspecial.IsTFast = _options[option_index].Setting;
							break;

						case TXT_ROAD_PIECES:
							oldspecial.IsRoad = _options[option_index].Setting;
							break;

						case TXT_SCATTER:
							oldspecial.IsScatter = _options[option_index].Setting;
							break;
					}
				}
				OutList.Add(EventClass(oldspecial));
				break;

			case 201|KN_BUTTON:
				process = false;
				break;

			case KN_NONE:
				break;

			default:
				int option_index = (input & ~KN_BUTTON) - 100;
				if (option_index >= 0 && option_index < option_count) {
					_options[option_index].Setting = (_options[option_index].Setting == false);
					if (_options[option_index].Setting) {
						_options[option_index].Button->Turn_On();
					} else {
						_options[option_index].Button->Turn_Off();
					}
				}
				break;
		}

		if (process) {
			SDL_GameInput_Delay(1);
		}
	}

	Map.Revert_Mouse_Shape();
	HiddenPage.Clear();
	Map.Flag_To_Redraw(true);
	Map.Render();
}


