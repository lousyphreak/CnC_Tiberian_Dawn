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

/* $Header$ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : EXPAND.CPP                                                   *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : 11/03/95                                                     *
 *                                                                                             *
 *                  Last Update : November 3, 1995 [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"

#ifdef NEWMENU

namespace
{
struct BonusMissionEntry {
	int Scenario;
	HousesType Side;
	int Text;
};

void Build_Bonus_Scenario_Name(char* buffer, size_t buffer_size, int scenario)
{
	if (buffer == NULL || buffer_size == 0) {
		return;
	}

	std::snprintf(buffer, buffer_size, "SCM%02dEA", scenario);
}
}

bool Expansion_Present(void)
{
	CCFileClass file("EXPAND.DAT");

	return(file.Is_Available());
}




class EListClass : public ListClass
{
	public:
		EListClass(int id, int x, int y, int w, int h, TextPrintType flags, void const * up, void const * down) :
			ListClass(id, x, y, w, h, flags, up, down) {};

	protected:
		virtual void Draw_Entry(int index, int x, int y, int width, int selected);
};


void EListClass::Draw_Entry(int index, int x, int y, int width, int selected)
{
	if (TextFlags & TPF_6PT_GRAD) {
		TextPrintType flags = TextFlags;

		if (selected) {
			flags = flags | TPF_BRIGHT_COLOR;
			LogicPage->Fill_Rect (x, y, x + width - 1, y + LineHeight - 1, CC_GREEN_SHADOW);
		} else {
			if (!(flags & TPF_USE_GRAD_PAL)) {
				flags = flags | TPF_MEDIUM_COLOR;
			}
		}

		Conquer_Clip_Text_Print(List[index]+sizeof(int), x, y, CC_GREEN, TBLACK, flags, width, Tabs);

	} else {
		Conquer_Clip_Text_Print(List[index]+sizeof(int), x, y, (selected ? BLUE : WHITE), TBLACK, TextFlags, width, Tabs);
	}
}

bool Expansion_Dialog(void)
{
	int factor			= (SeenBuff.Get_Width() == 320) ? 1 : 2;

	int	option_width 	= 236 * factor;
	int	option_height	= 162 * factor;
	int	option_x			= (320*factor - option_width) /2;
	int	option_y			= (200*factor - option_height) /2;

	GadgetClass * buttons = NULL;

	void const *up_button;
	void const *down_button;

	if (InMainLoop){
		up_button = Hires_Retrieve("BTN-UP.SHP");
		down_button = Hires_Retrieve("BTN-DN.SHP");
	}else{
		up_button = Hires_Retrieve("BTN-UP2.SHP");
		down_button = Hires_Retrieve("BTN-DN2.SHP");
	}

	TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD|TPF_NOSHADOW, option_x+25*factor, option_y+option_height-15*factor);
	TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD|TPF_NOSHADOW, option_x+option_width-50*factor, option_y+option_height-15*factor);
	EListClass list(202, option_x+10*factor, option_y+20*factor, option_width-20*factor, option_height-40*factor, TPF_6PT_GRAD|TPF_NOSHADOW, up_button, down_button);

	buttons = &ok;
	cancel.Add(*buttons);
	list.Add(*buttons);


	/*
	**	Add in all the expansion scenarios.
	*/
	char * sbuffer = (char*)_ShapeBuffer;
	for (int index = 20; index < 60; index++) {
		char buffer[128];
		CCFileClass file;

		Set_Scenario_Name(buffer, index, SCEN_PLAYER_GDI, SCEN_DIR_EAST, SCEN_VAR_A);
		strcat(buffer, ".INI");
		file.Set_Name(buffer);
		if (file.Is_Available()) {
			file.Read(sbuffer, 1000);
			sbuffer[1000] = '\r';
			sbuffer[1000+1] = '\n';
			sbuffer[1000+2] = '\0';

			WWGetPrivateProfileString("Basic", "Name", "x", buffer, sizeof(buffer), sbuffer);
			char * data = new char [strlen(buffer)+1+sizeof(int)+25];
			*((int*)&data[0]) = index;
			strcpy(&data[sizeof(int)], "GDI: ");
			strcat(&data[sizeof(int)], buffer);
			list.Add_Item(data);
		}
	}

	for (int index = 20; index < 60; index++) {
		char buffer[128];
		CCFileClass file;

		Set_Scenario_Name(buffer, index, SCEN_PLAYER_NOD, SCEN_DIR_EAST, SCEN_VAR_A);
		strcat(buffer, ".INI");
		file.Set_Name(buffer);
		if (file.Is_Available()) {
			file.Read(sbuffer, 1000);
			sbuffer[1000] = '\r';
			sbuffer[1000+1] = '\n';
			sbuffer[1000+2] = '\0';

			WWGetPrivateProfileString("Basic", "Name", "x", buffer, sizeof(buffer), sbuffer);
			char * data = new char [strlen(buffer)+1+sizeof(int)+25];
			*((int*)&data[0]) = index;
			strcpy(&data[sizeof(int)], "NOD: ");
			strcat(&data[sizeof(int)], buffer);
			list.Add_Item(data);
		}
	}

	Set_Logic_Page(SeenBuff);
	bool display = true;
	bool process = true;
	bool okval = true;
	while (process) {

		Call_Back();

		/*
		** If we have just received input focus again after running in the background then
		** we need to redraw.
		*/
		if (AllSurfaces.SurfacesRestored){
			AllSurfaces.SurfacesRestored=FALSE;
			display=TRUE;
		}

		if (display) {
			display = false;

			Hide_Mouse();

			/*
			**	Load the background picture.
			*/
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);

			Dialog_Box(option_x, option_y, option_width, option_height);
			Draw_Caption(TXT_MISSION_DESCRIPTION, option_x, option_y, option_width);
			buttons->Draw_All();
			Show_Mouse();
			WWDraw_Flush_Present();
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = buttons->Input();
		WWDraw_Flush_Present();
		switch (static_cast<int>(input)) {
			case KN_RETURN:
			case 200|KN_BUTTON:
			{
				char const* current_item = list.Current_Item();
				if (current_item == NULL) {
					process = false;
					okval = false;
					break;
				}

				if (current_item[sizeof(int)] == 'G') {
					ScenPlayer = SCEN_PLAYER_GDI;
				} else {
					ScenPlayer = SCEN_PLAYER_NOD;
				}
				ScenDir = SCEN_DIR_EAST;
				Whom = HOUSE_GOOD;
				Scenario = *(int*)current_item;
				process = false;
				okval = true;
				break;
			}

			case KN_ESC:
			case 201|KN_BUTTON:
			{
				char const* current_item = list.Current_Item();
				ScenPlayer = SCEN_PLAYER_GDI;
				ScenDir = SCEN_DIR_EAST;
				Whom = HOUSE_GOOD;
				if (current_item != NULL) {
					Scenario = *(int*)current_item;
				}
				process = false;
				okval = false;
				break;
			}

			default:
				break;
		}
	}

	/*
	**	Free up the allocations for the text lines in the list box.
	*/
	for (int index = 0; index < list.Count(); index++) {
		delete [] (char *)list.Get_Item(index);
	}

	return(okval);
}












/***********************************************************************************************
 * Bonus_Dialog -- Asks the user which bonus mission he wants to play                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/26/97 11:07AM ST : Created                                                             *
 *=============================================================================================*/
bool Bonus_Dialog(void)
{
	static BonusMissionEntry const BonusMissions[] = {
		{60, HOUSE_GOOD, TXT_BONUS_MISSION_1},
		{61, HOUSE_GOOD, TXT_BONUS_MISSION_2},
		{62, HOUSE_GOOD, TXT_BONUS_MISSION_3},
		{60, HOUSE_BAD,  TXT_BONUS_MISSION_4},
		{61, HOUSE_BAD,  TXT_BONUS_MISSION_5},
	};

	int factor			= (SeenBuff.Get_Width() == 320) ? 1 : 2;

	int	option_width 	= 236 * factor;
	int	option_height	= 162 * factor;
	int	option_x			= (320*factor - option_width) /2;
	int	option_y			= (200*factor - option_height) /2;

	GadgetClass * buttons = NULL;

	void const *up_button;
	void const *down_button;

	if (InMainLoop){
		up_button = Hires_Retrieve("BTN-UP.SHP");
		down_button = Hires_Retrieve("BTN-DN.SHP");
	}else{
		up_button = Hires_Retrieve("BTN-UP2.SHP");
		down_button = Hires_Retrieve("BTN-DN2.SHP");
	}

	TextButtonClass ok(200, TXT_OK, TPF_6PT_GRAD|TPF_NOSHADOW, option_x+25*factor, option_y+option_height-15*factor);
	TextButtonClass cancel(201, TXT_CANCEL, TPF_6PT_GRAD|TPF_NOSHADOW, option_x+option_width-50*factor, option_y+option_height-15*factor);
	EListClass list(202, option_x+10*factor, option_y+20*factor, option_width-20*factor, option_height-40*factor, TPF_6PT_GRAD|TPF_NOSHADOW, up_button, down_button);

	buttons = &ok;
	cancel.Add(*buttons);
	list.Add(*buttons);


	/*
	**	Gateway bonus missions are authored as SCM multiplayer-format maps.
	**	Expose the intended five menu entries explicitly, with side selection
	**	separate from the underlying scenario root.
	*/
	for (unsigned index = 0; index < (sizeof(BonusMissions) / sizeof(BonusMissions[0])); ++index) {
		char buffer[128];
		char scenario_name[32];
		CCFileClass file;

		Build_Bonus_Scenario_Name(scenario_name, sizeof(scenario_name), BonusMissions[index].Scenario);
		std::snprintf(buffer, sizeof(buffer), "%s.INI", scenario_name);
		file.Set_Name(buffer);
		if (file.Is_Available()) {
			char const* mission_name = Text_String(BonusMissions[index].Text);
			if (mission_name == NULL) {
				mission_name = "Bonus Mission";
			}

			char * data = new char [strlen(mission_name)+1+sizeof(int)+25];
			*((int*)&data[0]) = BonusMissions[index].Scenario;
			strcpy(&data[sizeof(int)], BonusMissions[index].Side == HOUSE_GOOD ? "GDI: " : "NOD: ");
			strcat(&data[sizeof(int)], mission_name);
			list.Add_Item(data);
		}
	}

	Set_Logic_Page(SeenBuff);
	bool display = true;
	bool process = true;
	bool okval = true;
	while (process) {

		Call_Back();

		/*
		** If we have just received input focus again after running in the background then
		** we need to redraw.
		*/
		if (AllSurfaces.SurfacesRestored){
			AllSurfaces.SurfacesRestored=FALSE;
			display=TRUE;
		}

		if (display) {
			display = false;

			Hide_Mouse();

			/*
			**	Load the background picture.
			*/
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);

			Dialog_Box(option_x, option_y, option_width, option_height);
			Draw_Caption(TXT_BONUS_MISSIONS, option_x, option_y, option_width);
			buttons->Draw_All();
			Show_Mouse();
			WWDraw_Flush_Present();
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = buttons->Input();
		WWDraw_Flush_Present();
		switch (static_cast<int>(input)) {
			case KN_RETURN:
			case 200|KN_BUTTON:
			{
				char const* current_item = list.Current_Item();
				if (current_item == NULL) {
					process = false;
					okval = false;
					break;
				}

				ScenPlayer = SCEN_PLAYER_MULTIPLAYER;
				if (current_item[sizeof(int)] == 'G') {
					Whom = HOUSE_GOOD;
				} else {
					Whom = HOUSE_BAD;
				}
				ScenDir = SCEN_DIR_EAST;
				Scenario = *(int*)current_item;
				process = false;
				okval = true;
				break;
			}

			case KN_ESC:
			case 201|KN_BUTTON:
			{
				char const* current_item = list.Current_Item();
				ScenPlayer = SCEN_PLAYER_MULTIPLAYER;
				ScenDir = SCEN_DIR_EAST;
				Whom = HOUSE_GOOD;
				if (current_item != NULL) {
					Scenario = *(int*)current_item;
				}
				process = false;
				okval = false;
				break;
			}

			default:
				break;
		}
	}

	/*
	**	Free up the allocations for the text lines in the list box.
	*/
	for (int index = 0; index < list.Count(); index++) {
		delete [] (char *)list.Get_Item(index);
	}

	return(okval);
}


#endif
