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

/* $Header:   F:\projects\c&c\vcs\code\debug.cpv   2.17   16 Oct 1995 16:49:18   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : DEBUG.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : September 10, 1993                                           *
 *                                                                                             *
 *                  Last Update : July 5, 1994   [JLB]                                         *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Self_Regulate -- Regulates the logic timer to result in smooth animation.                 *
 *   Debug_Key -- Debug mode keyboard processing.                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"
#include	<stdarg.h>
#include <FILEPCX.H>
#ifdef CHEAT_KEYS

extern bool ScreenRecording;

/***********************************************************************************************
 * Debug_Key -- Debug mode keyboard processing.                                                *
 *                                                                                             *
 *    If debugging is enabled, then this routine will be called for every keystroke that the   *
 *    game doesn't recognize. These extra keys usually perform some debugging function.        *
 *                                                                                             *
 * INPUT:   input -- The key code that was pressed.                                            *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/07/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
void Debug_Key(unsigned input)
{
	static int map_x = -1;
	static int map_y = -1;
	static int map_width = -1;
	static int map_height = -1;

	if (!input || input & KN_BUTTON) return;

	/*
	**	Processing of normal keystrokes.
	*/
	if (Debug_Flag) {

		switch (input) {

			case KN_L:
				extern int NetMonoMode,NewMonoMode;
				if (NetMonoMode)
					NetMonoMode = 0;
				else
					NetMonoMode = 1;
				NewMonoMode = 1;
				break;

			/*
			** Start saving off screens
			*/
			case (int)KN_K|(int)KN_CTRL_BIT:
				ScreenRecording = true;
				break;

			case KN_K:
				/*
				** time to create a screen shot using the PCX code (if it works)
				*/
				{
					GraphicBufferClass temp_page(	SeenBuff.Get_Width(),
															SeenBuff.Get_Height(),
															NULL,
															SeenBuff.Get_Width() * SeenBuff.Get_Height());
					char filename[30];

					SeenBuff.Blit(temp_page);
					for (int lp = 0; lp < 99; lp ++) {
						if (lp < 10) {
							sprintf(filename, "scrsht0%d.pcx", lp);
						} else {
							sprintf(filename, "scrsht%d.pcx", lp);
						}
						if (access(filename, F_OK) == -1)
							break;
					}

					Write_PCX_File(filename, temp_page, (unsigned char *)CurrentPalette);
					//Map.Place_Random_Crate();
				}
				break;

			case KN_P:
				Keyboard::Clear();
				while (!Keyboard::Check()) {
					Self_Regulate();
					Sound_Callback();
				}
				Keyboard::Clear();
				break;

			case KN_O:
				{
					AircraftClass * air = new AircraftClass(AIRCRAFT_ORCA, PlayerPtr->Class->House);
					if (air) {
						air->Altitude = 0;
						air->Unlimbo(Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()), DIR_N);
					}
				}
				break;

			case (int)KN_B|(int)KN_ALT_BIT:
				{
					Debug_Instant_Build ^= 1;
				}
				break;
			case KN_B:
				{
					AircraftClass * air = new AircraftClass(AIRCRAFT_HELICOPTER, PlayerPtr->Class->House);
					if (air) {
						air->Altitude = 0;
						air->Unlimbo(Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()), DIR_N);
					}
				}
				break;

			case KN_T:
				{
					AircraftClass * air = new AircraftClass(AIRCRAFT_TRANSPORT, PlayerPtr->Class->House);
					if (air) {
						air->Altitude = 0;
						air->Unlimbo(Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()), DIR_N);
					}
				}
				break;

			case KN_GRAVE:
				new AnimClass(ANIM_ART_EXP1, Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
				Explosion_Damage(Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()), 250, NULL, WARHEAD_HE);
				break;

			case KN_Z:
//				new AnimClass(ANIM_LZ_SMOKE, Map.Pixel_To_Coord(Get_Mouse_X(), Get_Mouse_Y()));
				GDI_Ending();
				break;

			case KN_C:
				Debug_Cheat = (Debug_Cheat == false);
				PlayerPtr->IsRecalcNeeded = true;
				PlayerPtr->Add_Nuke_Piece();
				PlayerPtr->Add_Nuke_Piece();
				PlayerPtr->Add_Nuke_Piece();

				/*
				**	This placement might affect any prerequisite requirements for construction
				**	lists. Update the buildable options accordingly.
				*/
				if (!ScenarioInit) {
					Map.Recalc();
					for (int index = 0; index < Buildings.Count(); index++) {
						Buildings.Ptr(index)->Update_Buildables();
					}
				}
				break;

			case (int)KN_Z|(int)KN_ALT_BIT:
				if (map_x == -1) {
					map_x = Map.MapCellX;
					map_y = Map.MapCellY;
					map_width = Map.MapCellWidth;
					map_height = Map.MapCellHeight;
					Map.MapCellX = 1;
					Map.MapCellY = 1;
					Map.MapCellWidth = 62;
					Map.MapCellHeight = 62;
				} else {
					Map.MapCellX = map_x;
					Map.MapCellY = map_y;
					Map.MapCellWidth = map_width;
					Map.MapCellHeight = map_height;
					map_x = -1;
					map_y = -1;
					map_width = -1;
					map_height = -1;
				}
				break;

			case KN_R:
				if (CurrentObject.Count()) {
					((TechnoClass *)CurrentObject[0])->IsCloakable = true;
				}
				break;

			case KN_M:
				if (Debug_Flag) {
					if (MonoClass::Is_Enabled()) {
						MonoClass::Disable();
					} else {
						MonoClass::Enable();
					}
				}
				break;

			case (int)KN_W|(int)KN_ALT_BIT:
				PlayerPtr->Flag_To_Win();
				break;

			case (int)KN_L|(int)KN_ALT_BIT:
				PlayerPtr->Flag_To_Lose();
				break;

			case KN_F:
				Debug_Find_Path ^= 1;
				break;

			case KN_DELETE:
				if (CurrentObject.Count()) {
					Map.Recalc();
					//CurrentObject[0]->Detach_All();
					delete CurrentObject[0];
				}
				break;

			case KN_D:
				if (Teams.Ptr(0)) {
					delete Teams.Ptr(0);
				}
				break;

			case (int)KN_DELETE|(int)KN_SHIFT_BIT:
				if (CurrentObject.Count()) {
					Map.Recalc();
					int damage = 50;
					CurrentObject[0]->Take_Damage(damage, 0, WARHEAD_SA);
				}
				break;

			case KN_INSERT:
				if (CurrentObject.Count()) {
					Map.PendingObject = &CurrentObject[0]->Class_Of();
					if (Map.PendingObject) {
						Map.PendingHouse = CurrentObject[0]->Owner();
						Map.PendingObjectPtr = Map.PendingObject->Create_One_Of(HouseClass::As_Pointer(Map.PendingHouse));
						if (Map.PendingObjectPtr) {
							Map.Set_Cursor_Pos();
							Map.Set_Cursor_Shape(Map.PendingObject->Occupy_List());
						}
					}
				}
				break;

			case KN_F3:
				Debug_Icon = (Debug_Icon == false);
				Map.Flag_To_Redraw(true);
				break;


			/*
			**	Reveal entire map to player.
			*/
			case KN_F4:
				if (GameToPlay == GAME_NORMAL) {
					Debug_Unshroud = (Debug_Unshroud == false);
					Map.Flag_To_Redraw(true);
				}
				break;

			/*
			**	Shows sight and fire range in the form of circles emanating from the currently
			**	selected unit. The white circle is for sight range, the red circle is for
			**	fire range.
			*/
			case KN_F7:
				if (CurrentObject.Count() && CurrentObject[0]->Is_Techno()) {
					TechnoTypeClass const & ttype = (TechnoTypeClass const &)CurrentObject[0]->Class_Of();
					int sight = ((int)ttype.SightRange)<<8;
					int weapon = 0;
					if (ttype.Primary != WEAPON_NONE) weapon = Weapons[ttype.Primary].Range;
					Set_Logic_Page(SeenBuff);
					COORDINATE center = CurrentObject[0]->Center_Coord();
					COORDINATE center2 = CurrentObject[0]->Fire_Coord(0);

					for (int r = 0; r < 255; r += 10) {
						int	x,y,x1,y1;
						DirType r1 = (DirType)r;
						DirType r2 = (DirType)((r+10) & 0xFF);

						if (Map.Coord_To_Pixel(Coord_Move(center, r1, sight), x, y)) {
							Map.Coord_To_Pixel(Coord_Move(center, r2, sight), x1, y1);
							LogicPage->Draw_Line(x, y+8, x1, y1+8, WHITE);
						}
						if (Map.Coord_To_Pixel(Coord_Move(center2, r1, weapon), x, y)) {
							Map.Coord_To_Pixel(Coord_Move(center2, r2, weapon), x1, y1);
							LogicPage->Draw_Line(x, y+8, x1, y1+8, RED);
						}
					}
				}
				break;

			case ((int)KN_F4 | (int)KN_CTRL_BIT):
				Debug_Unshroud = (Debug_Unshroud == false);
				Map.Flag_To_Redraw(true);
				break;
		}

	}
}


/***********************************************************************************************
 * Self_Regulate -- Regulates the logic timer to result in smooth animation                    *
 *                                                                                             *
 *    The self regulation process checks the number of frames displayed                        *
 *    per second and from this determines the amount of time to devote                         *
 *    to internal logic processing. By adjusting the time allotted to                          *
 *    internal processing, smooth animation can be maintained.                                 *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   In order for this routine to work properly it MUST be                           *
 *             called every display loop.                                                      *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/31/1991 JLB : Created.                                                                 *
 *   07/05/1994 JLB : Handles new monochrome system.                                           *
 *=============================================================================================*/
#define	UPDATE_INTERVAL	TIMER_SECOND
void Self_Regulate(void)
{
	static CountDownTimerClass DebugTimer(BT_SYSTEM);
	static ObjectClass * _lastobject = 0;

	if (!DebugTimer.Time()) {
		DebugTimer.Set(UPDATE_INTERVAL);

		if (MonoClass::Is_Enabled()) {
			MonoClass *mono = MonoClass::Get_Current();
			mono->Set_Default_Attribute(2);

			switch (MonoPage) {
				case 0:
					mono = &MonoArray[0];
					mono->Clear();

					/*
					**	Display the status of the currently selected object.
					*/
					if (CurrentObject.Count()) {
						_lastobject = CurrentObject[0];
					}
					if (_lastobject && !_lastobject->IsActive) {
						_lastobject = 0;
					}
					if (_lastobject) {
						_lastobject->Debug_Dump(mono);
					}
					Logic.Debug_Dump(mono);
					mono->Set_Cursor(0, 20);
					mono->Printf(
						"Heap size:%10ld \r"
						"Largest:  %10ld \r"
						"Ttl Free: %10ld \r"
						"Frag:     %10ld \r",
						Heap_Size(MEM_NORMAL),
						Ram_Free(MEM_NORMAL),
						Total_Ram_Free(MEM_NORMAL),
						Total_Ram_Free(MEM_NORMAL)-Ram_Free(MEM_NORMAL)
					);
					*MonoClass::Get_Current() = *mono;
					break;
			}

			MonoArray[MonoPage] = *mono;
		}
	}
}
#endif
