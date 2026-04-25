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

/* $Header:   F:\projects\c&c\vcs\code\mplayer.cpv   1.9   16 Oct 1995 16:51:08   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : MPLAYER.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Bill Randolph                                                *
 *                                                                                             *
 *                   Start Date : April 14, 1995                                               *
 *                                                                                             *
 *                  Last Update : July 5, 1995 [BRR]                                           *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Select_MPlayer_Game -- prompts user for UDP, TCP direct, or WOL game                      *
 *   Read_MultiPlayer_Settings -- reads multi-player settings from conquer.ini                 *
 *   Write_MultiPlayer_Settings -- writes multi-player settings to conquer.ini                 *
 *   Read_Scenario_Descriptions -- reads multi-player scenario #'s # descriptions              *
 *   Free_Scenario_Descriptions -- frees memory for the scenario descriptions                  *
 *   Computer_Message -- "sends" a message from the computer                                   *
 *   Garble_Message -- "garbles" a message                                                     *
 *   Surrender_Dialog -- Prompts user for surrendering                                         *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "FUNCTION.H"
#include "TCPIP.H"

static void Garble_Message(char *buf);

static bool Choose_Internet_Game(InternetTransportType& transport);
static bool Get_Internet_Host_Or_Join(bool& host_game);
static bool Get_IP_Address(char* address_buffer, bool required);
static bool Configure_TCP_Direct_Game(void);

static bool Choose_Internet_Game(InternetTransportType& transport)
{
	int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
	int dialog_w = 180 * factor;
	int dialog_h = 76 * factor;
	int dialog_x = (320 * factor - dialog_w) / 2;
	int dialog_y = (136 * factor - dialog_h) / 2;
	int dialog_cx = dialog_x + (dialog_w / 2);
	int margin = 6 * factor;
	int button_w = 78 * factor;
	int button_h = 9 * factor;
	int button_x = dialog_cx - (button_w / 2);
	int udp_y = dialog_y + 20 * factor;
	int tcp_y = udp_y + button_h + 2 * factor;
	int wol_y = tcp_y + button_h + 2 * factor;
	int cancel_y = wol_y + button_h + margin;

	enum {
		BUTTON_UDP = 100,
		BUTTON_TCP,
		BUTTON_WOL,
		BUTTON_CANCEL
	};

	ControlClass* commands = nullptr;
	TextButtonClass udpbtn(BUTTON_UDP, "UDP Direct", TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		button_x, udp_y, button_w, button_h);
	TextButtonClass tcpbtn(BUTTON_TCP, "TCP Direct", TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		button_x, tcp_y, button_w, button_h);
	TextButtonClass wolbtn(BUTTON_WOL, "Westwood Online", TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		button_x, wol_y, button_w, button_h);
	TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		dialog_cx - (60 * factor / 2), cancel_y, 60 * factor, button_h);
	TextButtonClass* buttons[] = {&udpbtn, &tcpbtn, &wolbtn, &cancelbtn};
	int current_button = 0;
	bool display = true;

	commands = &udpbtn;
	tcpbtn.Add_Tail(*commands);
	wolbtn.Add_Tail(*commands);
	cancelbtn.Add_Tail(*commands);
	buttons[current_button]->Turn_On();

	Set_Logic_Page(SeenBuff);
	Keyboard::Clear();

	while (true) {
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored = FALSE;
			display = true;
		}

		Call_Back();

		if (display) {
			Hide_Mouse();
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);
			Dialog_Box(dialog_x, dialog_y, dialog_w, dialog_h);
			Draw_Caption(TXT_MULTIPLAYER_GAME, dialog_x, dialog_y, dialog_w);
			Fancy_Text_Print("Select network transport", dialog_cx, dialog_y + 10 * factor, CC_GREEN, TBLACK,
				TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
			commands->Flag_List_To_Redraw();
			Show_Mouse();
			WWDraw_Flush_Present();
			display = false;
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = commands->Input();
		WWDraw_Flush_Present();
		int selection = 0;
		bool pressed = false;

		switch ((int)input) {
			case BUTTON_UDP | KN_BUTTON:
			case BUTTON_TCP | KN_BUTTON:
			case BUTTON_WOL | KN_BUTTON:
			case BUTTON_CANCEL | KN_BUTTON:
				selection = input & 0x7fff;
				pressed = true;
				break;

			case KN_UP:
				buttons[current_button]->Turn_Off();
				buttons[current_button]->Flag_To_Redraw();
				current_button = (current_button + 3) % 4;
				buttons[current_button]->Turn_On();
				buttons[current_button]->Flag_To_Redraw();
				break;

			case KN_DOWN:
				buttons[current_button]->Turn_Off();
				buttons[current_button]->Flag_To_Redraw();
				current_button = (current_button + 1) % 4;
				buttons[current_button]->Turn_On();
				buttons[current_button]->Flag_To_Redraw();
				break;

			case KN_RETURN:
				selection = buttons[current_button]->ID;
				pressed = true;
				break;

			case KN_ESC:
				selection = BUTTON_CANCEL;
				pressed = true;
				break;

			default:
				break;
		}

		if (!pressed) {
			continue;
		}

		switch (selection) {
			case BUTTON_UDP:
				transport = INTERNET_TRANSPORT_UDP;
				return true;

			case BUTTON_TCP:
				transport = INTERNET_TRANSPORT_TCP;
				return true;

			case BUTTON_WOL:
				transport = INTERNET_TRANSPORT_WOL;
				return true;

			default:
				return false;
		}
	}
}

static bool Get_Internet_Host_Or_Join(bool& host_game)
{
	int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
	int dialog_w = 168 * factor;
	int dialog_h = 58 * factor;
	int dialog_x = (320 * factor - dialog_w) / 2;
	int dialog_y = (136 * factor - dialog_h) / 2;
	int dialog_cx = dialog_x + (dialog_w / 2);
	int button_w = 60 * factor;
	int button_h = 9 * factor;
	int host_x = dialog_x + 16 * factor;
	int join_x = dialog_x + dialog_w - button_w - 16 * factor;
	int button_y = dialog_y + 26 * factor;
	int cancel_y = button_y + button_h + 6 * factor;

	enum {
		BUTTON_HOST = 100,
		BUTTON_JOIN,
		BUTTON_CANCEL
	};

	ControlClass* commands = nullptr;
	TextButtonClass hostbtn(BUTTON_HOST, "Host", TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		host_x, button_y, button_w, button_h);
	TextButtonClass joinbtn(BUTTON_JOIN, "Join", TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		join_x, button_y, button_w, button_h);
	TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		dialog_cx - (60 * factor / 2), cancel_y, 60 * factor, button_h);
	TextButtonClass* buttons[] = {&hostbtn, &joinbtn, &cancelbtn};
	int current_button = 0;
	bool display = true;

	commands = &hostbtn;
	joinbtn.Add_Tail(*commands);
	cancelbtn.Add_Tail(*commands);
	buttons[current_button]->Turn_On();

	Set_Logic_Page(SeenBuff);
	Keyboard::Clear();

	while (true) {
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored = FALSE;
			display = true;
		}

		Call_Back();

		if (display) {
			Hide_Mouse();
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);
			Dialog_Box(dialog_x, dialog_y, dialog_w, dialog_h);
			Draw_Caption(TXT_INTERNET, dialog_x, dialog_y, dialog_w);
			Fancy_Text_Print("Create or join a game", dialog_cx, dialog_y + 12 * factor, CC_GREEN, TBLACK,
				TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
			commands->Flag_List_To_Redraw();
			Show_Mouse();
			WWDraw_Flush_Present();
			display = false;
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = commands->Input();
		WWDraw_Flush_Present();
		int selection = 0;
		bool pressed = false;

		switch ((int)input) {
			case BUTTON_HOST | KN_BUTTON:
			case BUTTON_JOIN | KN_BUTTON:
			case BUTTON_CANCEL | KN_BUTTON:
				selection = input & 0x7fff;
				pressed = true;
				break;

			case KN_LEFT:
			case KN_UP:
				buttons[current_button]->Turn_Off();
				buttons[current_button]->Flag_To_Redraw();
				current_button = (current_button + 2) % 3;
				buttons[current_button]->Turn_On();
				buttons[current_button]->Flag_To_Redraw();
				break;

			case KN_RIGHT:
			case KN_DOWN:
				buttons[current_button]->Turn_Off();
				buttons[current_button]->Flag_To_Redraw();
				current_button = (current_button + 1) % 3;
				buttons[current_button]->Turn_On();
				buttons[current_button]->Flag_To_Redraw();
				break;

			case KN_RETURN:
				selection = buttons[current_button]->ID;
				pressed = true;
				break;

			case KN_ESC:
				selection = BUTTON_CANCEL;
				pressed = true;
				break;

			default:
				break;
		}

		if (!pressed) {
			continue;
		}

		if (selection == BUTTON_HOST) {
			host_game = true;
			return true;
		}
		if (selection == BUTTON_JOIN) {
			host_game = false;
			return true;
		}
		return false;
	}
}

static bool Get_IP_Address(char* address_buffer, bool required)
{
	int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
	int dialog_w = 200 * factor;
	int dialog_h = 64 * factor;
	int dialog_x = (320 * factor - dialog_w) / 2;
	int dialog_y = (136 * factor - dialog_h) / 2;
	int dialog_cx = dialog_x + (dialog_w / 2);
	int field_w = 150 * factor;
	int field_x = dialog_cx - (field_w / 2);
	int field_y = dialog_y + 24 * factor;
	int button_w = 45 * factor;
	int button_h = 9 * factor;
	int ok_x = dialog_cx - button_w - 5 * factor;
	int cancel_x = dialog_cx + 5 * factor;
	int button_y = field_y + 16 * factor;

	enum {
		BUTTON_OK = 100,
		BUTTON_CANCEL,
		BUTTON_ADDRESS
	};

	ControlClass* commands = nullptr;
	EditClass address_edt(BUTTON_ADDRESS, address_buffer, IP_ADDRESS_MAX,
		TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		field_x, field_y, field_w, -1, EditClass::ALPHANUMERIC);
	TextButtonClass okbtn(BUTTON_OK, TXT_OK, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		ok_x, button_y, button_w, button_h);
	TextButtonClass cancelbtn(BUTTON_CANCEL, TXT_CANCEL, TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		cancel_x, button_y, button_w, button_h);
	bool display = true;

	commands = &okbtn;
	cancelbtn.Add_Tail(*commands);
	address_edt.Add_Tail(*commands);
	address_edt.Set_Focus();

	Set_Logic_Page(SeenBuff);
	Keyboard::Clear();

	while (true) {
		if (AllSurfaces.SurfacesRestored) {
			AllSurfaces.SurfacesRestored = FALSE;
			display = true;
		}

		Call_Back();

		if (display) {
			Hide_Mouse();
			Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
			HidPage.Blit(SeenBuff);
			Dialog_Box(dialog_x, dialog_y, dialog_w, dialog_h);
			Draw_Caption(TXT_INTERNET, dialog_x, dialog_y, dialog_w);
			Fancy_Text_Print("Remote address or host name", dialog_cx, dialog_y + 12 * factor, CC_GREEN, TBLACK,
				TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
			commands->Flag_List_To_Redraw();
			Show_Mouse();
			WWDraw_Flush_Present();
			display = false;
		}

		Set_Logic_Page(SeenBuff);
		KeyNumType input = commands->Input();
		WWDraw_Flush_Present();
		switch (static_cast<int>(input)) {
			case KN_RETURN:
			case BUTTON_OK | KN_BUTTON:
				strtrim(address_buffer);
				if (!required || address_buffer[0] != '\0') {
					return true;
				}
				break;

			case KN_ESC:
			case BUTTON_CANCEL | KN_BUTTON:
				return false;

			default:
				break;
		}
	}
}

static bool Configure_TCP_Direct_Game(void)
{
	bool host_game = true;
	char address_buffer[IP_ADDRESS_MAX];

	if (!Get_Internet_Host_Or_Join(host_game)) {
		return false;
	}

	SDL_strlcpy(address_buffer, PlanetWestwoodIPAddress, sizeof(address_buffer));
	if (!Get_IP_Address(address_buffer, !host_game)) {
		return false;
	}

	SDL_strlcpy(PlanetWestwoodIPAddress, address_buffer, sizeof(PlanetWestwoodIPAddress));
	PlanetWestwoodIsHost = host_game;
	InternetMaxPlayers = 2;
	UseVirtualSubnetServer = false;
	ModemGameToPlay = host_game ? INTERNET_HOST : INTERNET_JOIN;
	Winsock.Set_Protocol_UDP(FALSE);

	return true;
}

/***********************************************************************************************
 * Select_MPlayer_Game -- prompts user for UDP, TCP direct, or WOL game                        *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      GAME_NORMAL, GAME_UDP, GAME_INTERNET, or GAME_WOL.                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
GameType Select_MPlayer_Game (void)
{
	if (!Choose_Internet_Game(InternetTransport)) {
		return GAME_NORMAL;
	}

	switch (InternetTransport) {
		case INTERNET_TRANSPORT_UDP:
			return GAME_UDP;

		case INTERNET_TRANSPORT_TCP:
			return Configure_TCP_Direct_Game() ? GAME_INTERNET : GAME_NORMAL;

		case INTERNET_TRANSPORT_WOL:
			return GAME_WOL;
	}

	return GAME_NORMAL;
}


/***********************************************************************************************
 * Read_MultiPlayer_Settings -- reads multi-player settings from conquer.ini                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void Read_MultiPlayer_Settings (void)
{
	char *buffer;							// INI staging buffer pointer.
	char *tbuffer;							// Accumulation buffer of trigger IDs.
	int len;									// Length of data in buffer.
	char *tokenptr;						// ptr to token
	PhoneEntryClass *phone;				// a phone book entry
	char *entry;							// a phone book entry
	char buf[128];							// buffer for parsing INI entry
	int i;
	CELL cell;

	/*------------------------------------------------------------------------
	Fetch working pointer to the INI staging buffer. Make sure that the buffer
	is cleared out before proceeding.  (Don't use the HidPage for this, since
	the HidPage may be needed for various uncompressions during the INI
	parsing.)
	------------------------------------------------------------------------*/
	buffer = (char *)_ShapeBuffer;
	memset(buffer, '\0', _ShapeBufferSize);

	/*------------------------------------------------------------------------
	Clear the initstring entries
	------------------------------------------------------------------------*/
	for (i = 0; i < InitStrings.Count(); i++) {
		delete[] InitStrings[i];
	}
	InitStrings.Clear();

	/*------------------------------------------------------------------------
	Clear the dialing entries
	------------------------------------------------------------------------*/
	for (i = 0; i < PhoneBook.Count(); i++) {
		delete PhoneBook[i];
	}
	PhoneBook.Clear();

	/*------------------------------------------------------------------------
	Create filename and read the file.
	------------------------------------------------------------------------*/
	CCFileClass file ("CONQUER.INI");
	if (!file.Is_Available()) {
		return;
	} else {
		file.Read(buffer, _ShapeBufferSize-1);
	}
	file.Close();

	/*------------------------------------------------------------------------
	Get the player's last-used Handle
	------------------------------------------------------------------------*/
	WWGetPrivateProfileString("MultiPlayer", "Handle", "Noname", MPlayerName,
		sizeof(MPlayerName), buffer);

	/*------------------------------------------------------------------------
	Get the player's last-used Color
	------------------------------------------------------------------------*/
	MPlayerPrefColor = WWGetPrivateProfileInt("MultiPlayer", "Color", 0, buffer);
	MPlayerHouse = (HousesType)WWGetPrivateProfileInt("MultiPlayer", "Side",
		HOUSE_GOOD, buffer);
	CurPhoneIdx = WWGetPrivateProfileInt("MultiPlayer", "PhoneIndex", -1, buffer);

#if 1
	TrapCheckHeap = WWGetPrivateProfileInt( "MultiPlayer", "CheckHeap", 0, buffer);
#endif

	/*------------------------------------------------------------------------
	Read in default serial settings
	------------------------------------------------------------------------*/
	WWGetPrivateProfileString ("SerialDefaults", "ModemName", "NoName", SerialDefaults.ModemName, MODEM_NAME_MAX, buffer);
	if (!strcmp ( SerialDefaults.ModemName, "NoName")) {
		SerialDefaults.ModemName[0] = 0;
	}
	WWGetPrivateProfileString ("SerialDefaults", "Port", "0", buf, 5, buffer);
	sscanf (buf, "%x", &SerialDefaults.Port);
	SerialDefaults.IRQ = WWGetPrivateProfileInt("SerialDefaults", "IRQ", -1, buffer);
	SerialDefaults.Baud = WWGetPrivateProfileInt("SerialDefaults", "Baud", -1, buffer);
	SerialDefaults.Init = WWGetPrivateProfileInt("SerialDefaults", "Init", 0, buffer);
	SerialDefaults.Compression = WWGetPrivateProfileInt ("SerialDefaults", "Compression", 0, buffer);
	SerialDefaults.ErrorCorrection = WWGetPrivateProfileInt ("SerialDefaults", "ErrorCorrection", 0, buffer);
	SerialDefaults.HardwareFlowControl = WWGetPrivateProfileInt ("SerialDefaults", "HardwareFlowControl", 1, buffer);
	WWGetPrivateProfileString ("SerialDefaults", "DialMethod", "T",
		buf, 2, buffer);


	// find dial method

	for (i = 0; i < DIAL_METHODS; i++) {
		if ( !strcmpi( buf, DialMethodCheck[ i ]) ) {
			SerialDefaults.DialMethod = (DialMethodType)i;
			break;
		}
	}

	// if method not found set to touch tone

	if (i == DIAL_METHODS) {
		SerialDefaults.DialMethod = DIAL_TOUCH_TONE;
	}

	SerialDefaults.InitStringIndex =
											WWGetPrivateProfileInt("SerialDefaults",
												"InitStringIndex", 0, buffer);

	SerialDefaults.CallWaitStringIndex =
											WWGetPrivateProfileInt("SerialDefaults",
												"CallWaitStringIndex", CALL_WAIT_CUSTOM,
												buffer);

	WWGetPrivateProfileString ("SerialDefaults", "CallWaitString", "",
		SerialDefaults.CallWaitString, CWAITSTRBUF_MAX, buffer);

	if (SerialDefaults.IRQ == 0 ||
		SerialDefaults.Baud == 0) {

		SerialDefaults.Port = 0;
		SerialDefaults.IRQ = -1;
		SerialDefaults.Baud = -1;
	}

	/*------------------------------------------------------------------------
	Set 'tbuffer' to point past the actual INI data
	------------------------------------------------------------------------*/
	len = strlen(buffer) + 2;
	tbuffer = buffer + len;

	/*------------------------------------------------------------------------
	Read all Base-Scenario names into 'tbuffer'
	------------------------------------------------------------------------*/
	WWGetPrivateProfileString("InitStrings", NULL, NULL, tbuffer,
		ShapeBufferSize-len, buffer);

	/*------------------------------------------------------------------------
	Read in & store each entry
	------------------------------------------------------------------------*/
	while (*tbuffer != '\0') {
		entry = new char[ INITSTRBUF_MAX ];

		entry[0] = 0;

		WWGetPrivateProfileString("InitStrings", tbuffer, NULL, entry,
			INITSTRBUF_MAX, buffer);

		strupr( entry );

		InitStrings.Add( entry );

		tbuffer += strlen(tbuffer) + 1;
	}

	// if no entries then have at least one

	if ( tbuffer == (buffer + len) ) {
		entry = new char[ INITSTRBUF_MAX ];
		strcpy( entry, "ATZ" );
		InitStrings.Add( entry );
		SerialDefaults.InitStringIndex = 0;
	} else {
		len = strlen(buffer) + 2;
	}

	/*------------------------------------------------------------------------
	Repeat the process for the phonebook
	------------------------------------------------------------------------*/
	tbuffer = buffer + len;

	/*------------------------------------------------------------------------
	Read in all phone book listings.
	Format: Name=PhoneNum,Port,IRQ,Baud,InitString
	------------------------------------------------------------------------*/

	/*........................................................................
	Read the entry names in
	........................................................................*/
	WWGetPrivateProfileString("PhoneBook", NULL, NULL, tbuffer,
		ShapeBufferSize-len, buffer);

	while (*tbuffer != '\0') {
		/*.....................................................................
		Create a new phone book entry
		.....................................................................*/
		phone = new PhoneEntryClass();

		/*.....................................................................
		Read the entire entry in
		.....................................................................*/
		WWGetPrivateProfileString("PhoneBook", tbuffer, NULL, buf, 128, buffer);

		/*.....................................................................
		Extract name, phone # & serial port settings
		.....................................................................*/
		tokenptr = strtok( buf, "|" );
		if (tokenptr) {
			strcpy( phone->Name, tokenptr );
			strupr( phone->Name );
		} else {
			phone->Name[0] = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			strcpy( phone->Number, tokenptr );
			strupr( phone->Number );
		} else {
			phone->Number[0] = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			sscanf( tokenptr, "%x", &phone->Settings.Port );
		} else {
			phone->Settings.Port = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.IRQ = atoi( tokenptr );
		} else {
			phone->Settings.IRQ = -1;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.Baud = atoi( tokenptr );
		} else {
			phone->Settings.Baud = -1;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.Compression = atoi( tokenptr );
		} else {
			phone->Settings.Compression = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.ErrorCorrection = atoi( tokenptr );
		} else {
			phone->Settings.ErrorCorrection = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.HardwareFlowControl = atoi( tokenptr );
		} else {
			phone->Settings.HardwareFlowControl = 1;
		}


		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			strcpy( buf, tokenptr );

			// find dial method

			for (i = 0; i < DIAL_METHODS; i++) {
				if ( !strcmpi( buf, DialMethodCheck[ i ]) ) {
					phone->Settings.DialMethod = (DialMethodType)i;
					break;
				}
			}

			// if method not found set to touch tone

			if (i == DIAL_METHODS) {
				phone->Settings.DialMethod = DIAL_TOUCH_TONE;
			}
		} else {
			phone->Settings.DialMethod = DIAL_TOUCH_TONE;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.InitStringIndex = atoi( tokenptr );
		} else {
			phone->Settings.InitStringIndex = 0;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			phone->Settings.CallWaitStringIndex = atoi( tokenptr );
		} else {
			phone->Settings.CallWaitStringIndex = CALL_WAIT_CUSTOM;
		}

		tokenptr = strtok( NULL, "|" );
		if (tokenptr) {
			strcpy (phone->Settings.CallWaitString, tokenptr);
		} else {
			phone->Settings.CallWaitString[0] = 0;
		}

		/*.....................................................................
		Add it to our list
		.....................................................................*/
		PhoneBook.Add(phone);

		tbuffer += strlen(tbuffer) + 1;
	}

	/*------------------------------------------------------------------------
	Read special recording playback values, to help find sync bugs
	------------------------------------------------------------------------*/
	if (PlaybackGame) {
		TrapFrame = WWGetPrivateProfileInt ("SyncBug","Frame",0x7fffffff, buffer);

		TrapObjType = (RTTIType)WWGetPrivateProfileInt ("SyncBug","Type",RTTI_NONE, buffer);
		WWGetPrivateProfileString ("SyncBug","Type","NONE",buf,80,buffer);
		if (!stricmp(buf,"AIRCRAFT"))
			TrapObjType = RTTI_AIRCRAFT;
		else if (!stricmp(buf,"ANIM"))
			TrapObjType = RTTI_ANIM;
		else if (!stricmp(buf,"BUILDING"))
			TrapObjType = RTTI_BUILDING;
		else if (!stricmp(buf,"BULLET"))
			TrapObjType = RTTI_BULLET;
		else if (!stricmp(buf,"INFANTRY"))
			TrapObjType = RTTI_INFANTRY;
		else if (!stricmp(buf,"UNIT"))
			TrapObjType = RTTI_UNIT;
		else {
			TrapObjType = RTTI_NONE;
		}

		WWGetPrivateProfileString ("SyncBug","Coord","0",buf,80,buffer);
		sscanf(buf,"%x",&TrapCoord);

		WWGetPrivateProfileString ("SyncBug","this","0",buf,80,buffer);
		TrapThis = reinterpret_cast<void *>(static_cast<uintptr_t>(strtoull(buf, NULL, 16)));

		WWGetPrivateProfileString ("SyncBug","Cell","0",buf,80,buffer);
		cell = atoi(buf);
		if (cell) {
			TrapCell = &(Map[cell]);
		}
	}
}


/***********************************************************************************************
 * Write_MultiPlayer_Settings -- writes multi-player settings to conquer.ini                   *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void Write_MultiPlayer_Settings (void)
{
	char * buffer;			// INI staging buffer pointer.
	CCFileClass file;
	int i;
	char entrytext[12];
	char buf[128];							// buffer for parsing INI entry

	/*------------------------------------------------------------------------
	Get a working pointer to the INI staging buffer. Make sure that the buffer
	starts cleared out of any data.
	------------------------------------------------------------------------*/
	buffer = (char *)_ShapeBuffer;
	memset(buffer, '\0', _ShapeBufferSize);

	file.Set_Name("CONQUER.INI");
	if (file.Is_Available()) {
		file.Open(READ);
		file.Read(buffer, _ShapeBufferSize-1);
		file.Close();
	}

	/*------------------------------------------------------------------------
	Save the player's last-used Handle & Color
	------------------------------------------------------------------------*/
	WWWritePrivateProfileInt("MultiPlayer", "PhoneIndex", CurPhoneIdx, buffer);
	WWWritePrivateProfileInt ("MultiPlayer", "Color", MPlayerPrefColor, buffer);
	WWWritePrivateProfileInt ("MultiPlayer", "Side", MPlayerHouse, buffer);
	WWWritePrivateProfileString("MultiPlayer", "Handle", MPlayerName, buffer);

	/*------------------------------------------------------------------------
	Clear all existing SerialDefault entries.
	------------------------------------------------------------------------*/
	WWWritePrivateProfileString ("SerialDefaults", NULL, NULL, buffer);

	/*------------------------------------------------------------------------
	Save default serial settings in opposite order you want to see them
	------------------------------------------------------------------------*/
	WWWritePrivateProfileString("SerialDefaults", "CallWaitString",
		SerialDefaults.CallWaitString, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "CallWaitStringIndex", SerialDefaults.CallWaitStringIndex, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "InitStringIndex", SerialDefaults.InitStringIndex, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "Init", SerialDefaults.Init, buffer);
	WWWritePrivateProfileString("SerialDefaults", "DialMethod",
		DialMethodCheck[ SerialDefaults.DialMethod ], buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "Baud", SerialDefaults.Baud, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "IRQ", SerialDefaults.IRQ, buffer);
	sprintf(buf, "%x", SerialDefaults.Port);
	WWWritePrivateProfileString("SerialDefaults", "Port", buf, buffer);
	WWWritePrivateProfileString("SerialDefaults", "ModemName", SerialDefaults.ModemName, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "Compression", SerialDefaults.Compression , buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "ErrorCorrection", SerialDefaults.ErrorCorrection, buffer);
	WWWritePrivateProfileInt ("SerialDefaults", "HardwareFlowControl", SerialDefaults.HardwareFlowControl, buffer);

	/*------------------------------------------------------------------------
	Clear all existing InitString entries.
	------------------------------------------------------------------------*/
	WWWritePrivateProfileString ("InitStrings", NULL, NULL, buffer);

	/*------------------------------------------------------------------------
	Save all InitString entries.  In descending order so they come out in
	ascending order.
	------------------------------------------------------------------------*/
	for (i = (InitStrings.Count() - 1); i >= 0; i--) {
		sprintf( buf, "%03d", i);
		WWWritePrivateProfileString ("InitStrings", buf, InitStrings[i], buffer);
	}

	/*------------------------------------------------------------------------
	Clear all existing Phone Book entries.
	------------------------------------------------------------------------*/
	WWWritePrivateProfileString ("PhoneBook", NULL, NULL, buffer);

	/*------------------------------------------------------------------------
	Save all Phone Book entries.
	Format: Entry=Name,PhoneNum,Port,IRQ,Baud,InitString
	------------------------------------------------------------------------*/
	for (i = (PhoneBook.Count() - 1); i >= 0; i--) {
		sprintf(buf,"%s|%s|%x|%d|%d|%d|%d|%d|%s|%d|%d|%s",
			PhoneBook[i]->Name,
			PhoneBook[i]->Number,
			PhoneBook[i]->Settings.Port,
			PhoneBook[i]->Settings.IRQ,
			PhoneBook[i]->Settings.Baud,
			PhoneBook[i]->Settings.Compression,
			PhoneBook[i]->Settings.ErrorCorrection,
			PhoneBook[i]->Settings.HardwareFlowControl,
			DialMethodCheck[ PhoneBook[i]->Settings.DialMethod ],
			PhoneBook[i]->Settings.InitStringIndex,
			PhoneBook[i]->Settings.CallWaitStringIndex,
			PhoneBook[i]->Settings.CallWaitString);
		snprintf(entrytext, sizeof(entrytext), "%03d", i);
		WWWritePrivateProfileString ("PhoneBook", entrytext, buf, buffer);
	}

	/*------------------------------------------------------------------------
	Write the INI data out to a file.
	------------------------------------------------------------------------*/
	file.Open(WRITE);
	file.Write(buffer,strlen(buffer));
	file.Close();
}


/***********************************************************************************************
 * Read_Scenario_Descriptions -- reads multi-player scenario #'s # descriptions                *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   02/14/1995 BR : Created.                                                                  *
 *=============================================================================================*/
void Read_Scenario_Descriptions (void)
{
	char *buffer;							// INI staging buffer pointer.
	CCFileClass file;
	int i;
	char fname[_MAX_FNAME+_MAX_EXT+sizeof(".INI")];

	/*------------------------------------------------------------------------
	Clear the scenario description lists
	------------------------------------------------------------------------*/
	MPlayerScenarios.Clear();
	MPlayerFilenum.Clear();

	/*------------------------------------------------------------------------
	Loop through all possible scenario numbers; if a file is available, add
	its number to the FileNum list.
	------------------------------------------------------------------------*/
	for (i = 0; i < 100; i++) {
		Set_Scenario_Name(ScenarioName, i, SCEN_PLAYER_MPLAYER,
			SCEN_DIR_EAST, SCEN_VAR_A);
		snprintf(fname, sizeof(fname), "%s.INI", ScenarioName);
		file.Set_Name (fname);

		if (file.Is_Available()) {
			MPlayerFilenum.Add(i);
		}
	}

	/*------------------------------------------------------------------------
	Now, for every file in the FileNum list, read in the INI file, and extract
	its description.
	------------------------------------------------------------------------*/
	for (i = 0; i < MPlayerFilenum.Count(); i++) {
		/*.....................................................................
		Fetch working pointer to the INI staging buffer. Make sure that the
		buffer is cleared out before proceeding.
		.....................................................................*/
		buffer = (char *)_ShapeBuffer;
		memset(buffer, '\0', _ShapeBufferSize);

		/*.....................................................................
		Create filename and read the file.
		.....................................................................*/
		Set_Scenario_Name(ScenarioName, MPlayerFilenum[i], SCEN_PLAYER_MPLAYER,
			SCEN_DIR_EAST, SCEN_VAR_A);
		snprintf(fname, sizeof(fname), "%s.INI", ScenarioName);
		file.Set_Name (fname);
		file.Read(buffer, _ShapeBufferSize-1);
		file.Close();

		/*.....................................................................
		Extract description & add it to the list.
		.....................................................................*/
		WWGetPrivateProfileString("Basic", "Name", "Nulls-Ville",
			MPlayerDescriptions[i], 40, buffer);
		MPlayerScenarios.Add(MPlayerDescriptions[i]);
	}
}


/***********************************************************************************************
 * Free_Scenario_Descriptions -- frees memory for the scenario descriptions                    *
 *                                                                                             *
 * INPUT:                                                                                      *
 *      none.                                                                                  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *      none.                                                                                  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *      none.                                                                                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/05/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
void Free_Scenario_Descriptions(void)
{
	int i;

	/*------------------------------------------------------------------------
	Clear the scenario descriptions & filenames
	------------------------------------------------------------------------*/
	MPlayerScenarios.Clear();
	MPlayerFilenum.Clear();

	/*------------------------------------------------------------------------
	Clear the initstring entries
	------------------------------------------------------------------------*/
	for (i = 0; i < InitStrings.Count(); i++) {
		delete InitStrings[i];
	}
	InitStrings.Clear();

	/*------------------------------------------------------------------------
	Clear the dialing entries
	------------------------------------------------------------------------*/
	for (i = 0; i < PhoneBook.Count(); i++) {
		delete PhoneBook[i];
	}
	PhoneBook.Clear();
}


/***************************************************************************
 * Computer_Message -- "sends" a message from the computer                 *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
void Computer_Message(void)
{
	int color;
	char txt[160];
	HousesType house;
	HouseClass *ptr;

	/*------------------------------------------------------------------------
	Find the computer house that the message will be from
	------------------------------------------------------------------------*/
	for (house = HOUSE_MULTI1; house < (HOUSE_MULTI1 + MPlayerMax); house++) {
		ptr = HouseClass::As_Pointer(house);

		if (!ptr || ptr->IsHuman || ptr->IsDefeated) {
			continue;
		}

		/*.....................................................................
		Decode this house's color
		.....................................................................*/
		color = MPlayerTColors[ptr->RemapColor];

		/*.....................................................................
		We now have a 1/4 chance of echoing one of the human players' messages
		back.
		.....................................................................*/
		if (IRandom(0,3) == 2) {
			/*..................................................................
			Now we have a 1/3 chance of garbling the human message.
			..................................................................*/
			if (IRandom(0,2) == 1) {
				Garble_Message(LastMessage);
			}

			/*..................................................................
			Only add the message if there is one to add.
			..................................................................*/
			if (strlen(LastMessage)) {
				sprintf(txt,"%s %s",Text_String(TXT_FROM_COMPUTER),LastMessage);
				Messages.Add_Message(txt, color,
					TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
			}
		} else {
			sprintf(txt,"%s %s",Text_String(TXT_FROM_COMPUTER),
				Text_String(TXT_COMP_MSG1 + IRandom(0,12)));
			Messages.Add_Message(txt, color,
				TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_FULLSHADOW, 600, 0, 0);
		}

		return;
	}
}


/***************************************************************************
 * Garble_Message -- "garbles" a message                                   *
 *                                                                         *
 * INPUT:                                                                  *
 *      buf      buffer to garble; stores output message                   *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      none.                                                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/06/1995 BRR : Created.                                             *
 *=========================================================================*/
static void Garble_Message(char *buf)
{
	char txt[80];
	char punct[20];		// for punctuation
	char *p;					// working ptr
	int numwords;			// # words in the phrase
	char *words[40];		// ptrs to various words in the phrase
	int i,j;

	/*------------------------------------------------------------------------
	Pull off any trailing punctuation
	------------------------------------------------------------------------*/
	p = buf + strlen(buf) - 1;
	while (1) {
		if (p < buf)
			break;
		if (p[0]=='!' || p[0]=='.' || p[0]=='?') {
			p--;
		} else {
			p++;
			break;
		}
		if (strlen(p) >= (sizeof(punct) - 1) ) {
			break;
		}
	}
	strcpy (punct, p);
	p[0] = 0;

	for (i = 0; i < 40; i++) {
		words[i] = NULL;
	}

	/*------------------------------------------------------------------------
	Copy the original buffer
	------------------------------------------------------------------------*/
	strcpy(txt,buf);

	/*------------------------------------------------------------------------
	Split it up into words
	------------------------------------------------------------------------*/
	p = strtok (txt, " ");
	numwords = 0;
	while (p) {
		words[numwords] = p;
		numwords++;
		p = strtok (NULL, " ");
	}

	/*------------------------------------------------------------------------
	Now randomly put the words back.  Don't use the real random-number
	generator, since different machines will have different LastMessage's,
	and will go out of sync.
	------------------------------------------------------------------------*/
	buf[0] = 0;
	for (i = 0; i < numwords; i++) {
		j = Sim_IRandom(0,numwords);
		if (words[j] == NULL) {		// this word has been used already
			i--;
			continue;
		}
		strcat(buf,words[j]);
		words[j] = NULL;
		if (i < numwords-1)
			strcat(buf," ");
	}
	strcat(buf,punct);
}


/***************************************************************************
 * Surrender_Dialog -- Prompts user for surrendering                       *
 *                                                                         *
 * INPUT:                                                                  *
 *      none.                                                              *
 *                                                                         *
 * OUTPUT:                                                                 *
 *      0 = user cancels, 1 = user wants to surrender.                     *
 *                                                                         *
 * WARNINGS:                                                               *
 *      none.                                                              *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 BRR : Created.                                             *
 *=========================================================================*/
int Surrender_Dialog(void)
{
	int factor			= (SeenBuff.Get_Width() == 320) ? 1 : 2;
	/*........................................................................
	Dialog & button dimensions
	........................................................................*/
	int d_dialog_w = 170*factor;											// dialog width
	int d_dialog_h = 53*factor;											// dialog height
	int d_dialog_x = ((320*factor - d_dialog_w) / 2);				// centered x-coord
	int d_dialog_y = ((200*factor - d_dialog_h) / 2);				// centered y-coord
	int d_dialog_cx = d_dialog_x + (d_dialog_w / 2);		// coord of x-center

	int d_margin = 5*factor;												// margin width/height
	int d_topmargin = 20*factor;											// top margin

	int d_ok_w = 45*factor;												// ok width
	int d_ok_h = 9*factor;													// ok height
	int d_ok_x = d_dialog_cx - d_ok_w - 5*factor;					// ok x
	int d_ok_y = d_dialog_y + d_dialog_h - d_ok_h - d_margin;	// ok y

	int d_cancel_w = 45*factor;											// cancel width
	int d_cancel_h = 9*factor;											// cancel height
	int d_cancel_x = d_dialog_cx + 5*factor;							// cancel x
	int d_cancel_y = d_dialog_y + d_dialog_h - d_cancel_h - d_margin; // cancel y


	/*........................................................................
	Button enumerations
	........................................................................*/
	enum {
		BUTTON_OK = 100,
		BUTTON_CANCEL,
	};

	/*........................................................................
	Redraw values: in order from "top" to "bottom" layer of the dialog
	........................................................................*/
	typedef enum {
		REDRAW_NONE = 0,
		REDRAW_BUTTONS,
		REDRAW_BACKGROUND,
		REDRAW_ALL = REDRAW_BACKGROUND
	} RedrawType;

	/*........................................................................
	Dialog variables
	........................................................................*/
	RedrawType display;							// requested redraw level
	bool process;									// loop while true
	KeyNumType input;
	int retcode;

	/*........................................................................
	Buttons
	........................................................................*/
	ControlClass *commands = NULL;		// the button list

	TextButtonClass okbtn (BUTTON_OK, TXT_OK,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		d_ok_x, d_ok_y, d_ok_w, d_ok_h);

	TextButtonClass cancelbtn (BUTTON_CANCEL, TXT_CANCEL,
		TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW,
		d_cancel_x, d_cancel_y, d_cancel_w, d_cancel_h);

	/*
	------------------------------- Initialize -------------------------------
	*/
	Set_Logic_Page(SeenBuff);

	/*
	......................... Create the button list .........................
	*/
	commands = &okbtn;
	cancelbtn.Add_Tail(*commands);

	/*
	-------------------------- Main Processing Loop --------------------------
	*/
	display = REDRAW_ALL;
	process = true;
	while (process) {

		/*
		** If we have just received input focus again after running in the background then
		** we need to redraw.
		*/
		if (AllSurfaces.SurfacesRestored){
			AllSurfaces.SurfacesRestored=FALSE;
			display=REDRAW_ALL;
		}

		/*
		........................ Invoke game callback .........................
		*/
		if (Main_Loop()) {
			retcode = 0;
			process = false;
		}

		/*
		...................... Refresh display if needed ......................
		*/
		if (display) {

			/*
			...................... Display the dialog box ......................
			*/
			Hide_Mouse();
			if (display >= REDRAW_BACKGROUND) {
				Dialog_Box(d_dialog_x, d_dialog_y, d_dialog_w, d_dialog_h);
				Draw_Caption(TXT_NONE, d_dialog_x, d_dialog_y, d_dialog_w);

				/*
				....................... Draw the captions .......................
				*/
				Fancy_Text_Print(Text_String(TXT_SURRENDER),
					d_dialog_cx, d_dialog_y + d_topmargin,
					CC_GREEN, TBLACK,
					TPF_CENTER | TPF_6PT_GRAD | TPF_USE_GRAD_PAL | TPF_NOSHADOW);
			}

			/*
			........................ Redraw the buttons ........................
			*/
			if (display >= REDRAW_BUTTONS) {
				commands->Flag_List_To_Redraw();
			}
			Show_Mouse();
			WWDraw_Flush_Present();
			display = REDRAW_NONE;
		}

		/*
		........................... Get user input ............................
		*/
		Set_Logic_Page(SeenBuff);
		input = commands->Input();
		WWDraw_Flush_Present();

		/*
		............................ Process input ............................
		*/
		switch ((int)input) {
			case (KN_RETURN):
			case (BUTTON_OK | KN_BUTTON):
				retcode = 1;
				process = false;
				break;

			case (KN_ESC):
			case (BUTTON_CANCEL | KN_BUTTON):
				retcode = 0;
				process = false;
				break;

			default:
				break;
		}
	}

	/*
	--------------------------- Redraw the display ---------------------------
	*/
	HiddenPage.Clear();
	Map.Flag_To_Redraw(true);
	Map.Render();

	return (retcode);
}
