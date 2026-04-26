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

/* $Header:   F:\projects\c&c\vcs\code\init.cpv   2.18   16 Oct 1995 16:50:16   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : INIT.CPP                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : January 20, 1992                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Anim_Init -- Initialize the VQ animation control structure.                               *
 *   Init_Game -- Main game initialization routine.                                            *
 *   Load_Recording_Values -- Loads recording values from recording file                       *
 *   Obfuscate -- Sufficiently transform parameter to thwart casual hackers.                   *
 *   Parse_Command_Line -- Parses the command line parameters.                                 *
 *   Parse_INI_File -- Parses CONQUER.INI for special options                                  *
 *   Play_Intro -- plays the introduction & logo movies                                        *
 *   Save_Recording_Values -- Saves recording values to a recording file                       *
 *   Select_Game -- The game's main menu                                                       *
 *   Version_Number -- Determines the version number.                                          *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#include	"KEYBOARD.H"
#include	"LOADDLG.H"
#include	"SDLINPUT.H"
#include	"TCPIP.H"
#include	"WSMGR.H"

static HANDLE			hCCLibrary;
static bool				BootstrapCoreMixfilesRegistered = false;

static void Wait_For_Countdown_With_Callback(CountDownTimerClass& timer)
{
	while (timer.Time()) {
		Call_Back();
		SDL_GameInput_Delay(1);
	}
}

/****************************************
**	Function prototypes for this module **
*****************************************/
static void Play_Intro(bool for_real = false);
static void* Load_Alloc_Data_From_Name(char const* filename);

int32_t Calculate_CRC(void* buffer, int32_t len)
{
	auto rotate_left = [](int32_t value, uint32_t count) -> int32_t {
		const uint32_t bits = static_cast<uint32_t>(value);
		return static_cast<int32_t>((bits << count) | (bits >> ((sizeof(bits) * 8) - count)));
	};

	if (buffer == NULL || len <= 0) {
		return 0;
	}

	int32_t crc = 0;
	int index = 0;
	char staging_buffer[sizeof(int32_t)] = {0};

	auto composite_value = [&]() -> int32_t {
		uint32_t composite = 0;
		for (int element = 0; element < index; ++element) {
			composite |= static_cast<uint32_t>(static_cast<uint8_t>(staging_buffer[element])) << (element * 8);
		}
		return static_cast<int32_t>(composite);
	};

	auto submit_byte = [&](char datum) {
		staging_buffer[index++] = datum;

		if (index == static_cast<int>(sizeof(staging_buffer))) {
			crc = static_cast<int32_t>(static_cast<uint32_t>(rotate_left(crc, 1))
				+ static_cast<uint32_t>(composite_value()));
			memset(staging_buffer, 0, sizeof(staging_buffer));
			index = 0;
		}
	};

	const char* data = static_cast<const char*>(buffer);
	int bytes_left = len;

	while (bytes_left && index != 0) {
		submit_byte(*data++);
		--bytes_left;
	}

	int longcount = bytes_left / static_cast<int>(sizeof(int32_t));
	while (longcount--) {
		const uint32_t composite = static_cast<uint32_t>(static_cast<uint8_t>(data[0]))
			| (static_cast<uint32_t>(static_cast<uint8_t>(data[1])) << 8)
			| (static_cast<uint32_t>(static_cast<uint8_t>(data[2])) << 16)
			| (static_cast<uint32_t>(static_cast<uint8_t>(data[3])) << 24);
		crc = static_cast<int32_t>(static_cast<uint32_t>(rotate_left(crc, 1)) + composite);
		data += sizeof(int32_t);
		bytes_left -= sizeof(int32_t);
	}

	while (bytes_left) {
		submit_byte(*data++);
		--bytes_left;
	}

	if (index != 0) {
		return static_cast<int32_t>(static_cast<uint32_t>(rotate_left(crc, 1))
			+ static_cast<uint32_t>(composite_value()));
	}

	return crc;
}

extern "C" {
extern uint32_t RandNumb;
}

extern int SimRandIndex;

static void* Load_Alloc_Data_From_Name(char const* filename)
{
	CCFileClass file(filename);
	return Load_Alloc_Data(file);
}

#define ATTRACT_MODE_TIMEOUT	3600		// timeout for attract mode

extern bool Server_Remote_Connect(void);
extern bool Client_Remote_Connect(void);
extern bool Remote_Connect(void);


/***********************************************************************************************
 * Init_Game -- Main game initialization routine.                                              *
 *                                                                                             *
 *    Perform all one-time game initializations here. This includes all                        *
 *    allocations and table setups. The intro and other one-time startup                       *
 *    tasks are also performed here.                                                           *
 *                                                                                             *
 * INPUT:   argc,argv   -- Command line arguments.                                             *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only call this ONCE!                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   10/07/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Init_Game(int , char *[])
{
	void const *temp_mouse_shapes;

	CCDebugString ("C&C95 - About to load reslib.dll\n");
	hCCLibrary = LoadLibrary("reslib.dll");

	/*
	**	Initialize the game object heaps.
	*/
	CCDebugString ("C&C95 - About to enter Units.Set_Heap\n");
	Units.Set_Heap(UNIT_MAX);
	CCDebugString ("C&C95 - About to enter Factories.Set_Heap\n");
	Factories.Set_Heap(FACTORY_MAX);
	CCDebugString ("C&C95 - About to enter Terrains.Set_Heap\n");
	Terrains.Set_Heap(TERRAIN_MAX);
	CCDebugString ("C&C95 - About to enter Templates.Set_Heap\n");
	Templates.Set_Heap(TEMPLATE_MAX);
	CCDebugString ("C&C95 - About to enter Smudges.Set_Heap\n");
	Smudges.Set_Heap(SMUDGE_MAX);
	CCDebugString ("C&C95 - About to enter Overlays.Set_Heap\n");
	Overlays.Set_Heap(OVERLAY_MAX);
	CCDebugString ("C&C95 - About to enter Infantry.Set_Heap\n");
	Infantry.Set_Heap(INFANTRY_MAX);
	CCDebugString ("C&C95 - About to enter Bullets.Set_Heap\n");
	Bullets.Set_Heap(BULLET_MAX);
	CCDebugString ("C&C95 - About to enter Buildings.Set_Heap\n");
	Buildings.Set_Heap(BUILDING_MAX);
	CCDebugString ("C&C95 - About to enter Anims.Set_Heap\n");
	Anims.Set_Heap(ANIM_MAX);
	CCDebugString ("C&C95 - About to enter Aircraft.Set_Heap\n");
	Aircraft.Set_Heap(AIRCRAFT_MAX);
	CCDebugString ("C&C95 - About to enter Triggers.Set_Heap\n");
	Triggers.Set_Heap(TRIGGER_MAX);
	CCDebugString ("C&C95 - About to enter TeamTypes.Set_Heap\n");
	TeamTypes.Set_Heap(TEAMTYPE_MAX);
	CCDebugString ("C&C95 - About to enter Teams.Set_Heap\n");
	Teams.Set_Heap(TEAM_MAX);
	CCDebugString ("C&C95 - About to enter Houses.Set_Heap\n");
	Houses.Set_Heap(HOUSE_MAX);

	/*
	**	Initialize all the waypoints to invalid values.
	*/
	CCDebugString ("C&C95 - About to clear waypoints\n");
	memset(Waypoint, 0xFF, sizeof(Waypoint));

	/*
	**	Setup the keyboard processor in preparation for the game.
	*/
	CCDebugString ("C&C95 - About to do various keyboard inits\n");
#ifdef FIX_ME_LATER
	Keyboard_Attributes_Off(TRACKEXT | PAUSEON | BREAKON | SCROLLLOCKON | CTRLSON | CTRLCON | PASSBREAKS | FILTERONLY | TASKSWITCHABLE);
#endif //FIX_ME_LATER
	Keyboard::Clear();
	Kbd.Clear();

	/*
	**	This is the shape staging buffer. It must always be available, so it is
	**	allocated here and never freed. The library sets the globals ShapeBuffer
	**	and ShapeBufferSize to these values, so it can be accessed for other
	**	purposes.
	*/
	CCDebugString ("C&C95 - About to call Set_Shape_Buffer\n");
	Set_Shape_Buffer(new unsigned char[SHAPE_BUFFER_SIZE], SHAPE_BUFFER_SIZE);

	/*
	**	Bootstrap enough of the system so that the error dialog box can sucessfully
	**	be displayed.
	*/
	CCDebugString ("C&C95 - About to register CCLOCAL.MIX\n");
#ifdef DEMO
	new MixFileClass("DEMOL.MIX");
	MixFileClass::Cache("DEMOL.MIX");
#else
	int temp = RequiredCD;
	RequiredCD = -2;
	new MixFileClass("CCLOCAL.MIX");			// Cached.
	MixFileClass::Cache("CCLOCAL.MIX");
	CCDebugString ("C&C95 - About to register UPDATE.MIX\n");
	new MixFileClass("UPDATE.MIX");			// Cached.
	CCDebugString ("C&C95 - About to register UPDATEC.MIX\n");
	new MixFileClass("UPDATEC.MIX");			// Cached.
	MixFileClass::Cache("UPDATEC.MIX");
#ifdef JAPANESE
	CCDebugString ("C&C95 - About to register LANGUAGE.MIX\n");
	new MixFileClass("LANGUAGE.MIX");
#endif	//JAPANESE

	RequiredCD = temp;

#ifndef DEMO
	const bool bootstrap_core_data_available = CCFileClass("CONQUER.MIX").Is_Available()
		&& CCFileClass("GENERAL.MIX").Is_Available();
	if (bootstrap_core_data_available) {
		if (!CCFileClass::Is_There_Search_Drives()) {
			char local_search[] = ".";
			CCFileClass::Set_Search_Drives(local_search);
		}

		RequiredCD = -2;

		if (!CCFileClass("12GREEN.FNT").Is_Available()) {
			CCDebugString("C&C95 - About to pre-register GENERAL.MIX\n");
			if (!GeneralMix) {
				GeneralMix = new MixFileClass("GENERAL.MIX");
			}

			CCDebugString("C&C95 - About to pre-register CONQUER.MIX\n");
			new MixFileClass("CONQUER.MIX");
			BootstrapCoreMixfilesRegistered = true;
		}
	}
#endif

#endif
	CCDebugString ("C&C95 - About to load fonts\n");
	Green12FontPtr = Load_Alloc_Data_From_Name("12GREEN.FNT");
	Green12GradFontPtr = Load_Alloc_Data_From_Name("12GRNGRD.FNT");
	MapFontPtr = Load_Alloc_Data_From_Name("8FAT.FNT");
	Font8Ptr = MixFileClass::Retrieve(FONT8);
	FontPtr = (char *)Font8Ptr;
	Set_Font(FontPtr);
	Font3Ptr = MixFileClass::Retrieve(FONT3);
//	Font6Ptr = MixFileClass::Retrieve(FONT6);
    	 Font6Ptr = Load_Alloc_Data_From_Name("6POINT.FNT");
	//ScoreFontPtr = MixFileClass::Retrieve("12GRNGRD.FNT");	//GRAD12FN");	//("SCOREFNT.FNT");
	ScoreFontPtr = Load_Alloc_Data_From_Name("12GRNGRD.FNT");
	FontLEDPtr = MixFileClass::Retrieve("LED.FNT");
	VCRFontPtr = MixFileClass::Retrieve("VCR.FNT");
//	GradFont6Ptr = MixFileClass::Retrieve("GRAD6FNT.FNT");
	GradFont6Ptr = Load_Alloc_Data_From_Name("GRAD6FNT.FNT");
	BlackPalette = new(MEM_CLEAR|MEM_REAL) unsigned char[768];
	GamePalette = new(MEM_CLEAR|MEM_REAL) unsigned char[768];
	OriginalPalette = new(MEM_CLEAR|MEM_REAL) unsigned char[768];
	WhitePalette = new(MEM_CLEAR|MEM_REAL) unsigned char[768];
	memset(WhitePalette, 63, 768);

	CCDebugString ("C&C95 - About to set palette\n");
	memset(BlackPalette, 0x01, 768);
	if (!Special.IsFromInstall) Set_Palette(BlackPalette);
	memset(BlackPalette, 0, 768);
	if (!Special.IsFromInstall) {
		Set_Palette(BlackPalette);
		CCDebugString ("C&C95 - About to clear visible page\n");
		VisiblePage.Clear();
	}

	Set_Palette(GamePalette);

	CCDebugString ("C&C95 - About to set the mouse shape\n");
	/*
	** Since there is no mouse shape currently available we need'
	** to set one of our own.
	*/
	SDL_GameInput_ShowCursor(false);
	if (MouseInstalled) {
		temp_mouse_shapes = MixFileClass::Retrieve("MOUSE.SHP");
		if (temp_mouse_shapes) {
			Set_Mouse_Cursor(0, 0, Extract_Shape(temp_mouse_shapes,0));
			while (Get_Mouse_State() > 1) {
				Show_Mouse();
			}
		}
	}

	CCDebugString ("C&C95 - About to enter wait for focus loop\n");
	/*
	** Process the message loop until we are in focus.
	*/
	do {
		CCDebugString ("C&C95 - About to call Keyboard::Check\n");
		Keyboard::Check();
	}while (!GameInFocus);
	AllSurfaces.SurfacesRestored=FALSE;

	CCDebugString ("C&C95 - About to load the language file\n");
	/*
	**	Fetch the language text from the hard drive first. If it cannot be
	**	found on the hard drive, then look for it in the mixfile.
	*/
	RawFileClass language_file(Language_Name("CONQUER"));
	if (language_file.Is_Available()) {
		SystemStrings = (char const *)Load_Alloc_Data(language_file);
	} else {
		CCFileClass language_mix_file(Language_Name("CONQUER"));
		SystemStrings = language_mix_file.Is_Available()
			? (char const *)Load_Alloc_Data(language_mix_file)
			: NULL;
	}

	/*
	**	Default palette initialization. Uses the desert palette for convenience,
	**	but only the non terrain specific colors matter.
	*/
	//Mem_Copy((void *)MixFileClass::Retrieve("TEMPERAT.PAL"), GamePalette, 768L);
	CCFileClass palfile ("TEMPERAT.PAL");
	palfile.Read (GamePalette, 768L);

	if (!MouseInstalled) {
		char buffer[255];
		Set_Palette(GamePalette);
#ifdef GERMAN
		sprintf(buffer, "Command & Conquer kann Ihren Maustreiber nicht finden..");
#else
#ifdef FRENCH
		sprintf(buffer, "Command & Conquer ne peut pas d�tecter votre gestionnaire de souris.");
#else
		sprintf(buffer, "Command & Conquer is unable to detect your mouse driver.");
#endif
#endif
		CCMessageBox().Process(buffer, TXT_OK);
		Prog_End();
		exit(1);
	}

#ifdef DEMO
	/*
	**	Add in any override path specified in the conquer.ini file.
	*/
	if (strlen(OverridePath)) {
		CCFileClass::Set_Search_Drives(OverridePath);
	}
#endif

	CCDebugString ("C&C95 - About to search for CD drives\n");
	/*
	**	Always try to look at the CD-ROM for data files.
	*/
	if (!CCFileClass::Is_There_Search_Drives()) {
		RawFileClass local_ccmix("CONQUER.MIX");
		RawFileClass local_generalmix("GENERAL.MIX");
		if (local_ccmix.Is_Available() && local_generalmix.Is_Available()) {
			char local_search[] = ".";
			CCFileClass::Set_Search_Drives(local_search);
		}
	}

	if (!CCFileClass::Is_There_Search_Drives()) {

		/*
		** If there are no search drives specified then we must be playing
		** off cd, so read files from there.
		*/
		int error;

		do {
			if (!CDList.Get_Number_Of_Drives()){
				Set_Palette(GamePalette);
				Show_Mouse();
				CCMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
				Prog_End();
				exit(EXIT_FAILURE);
			}
			CCFileClass::Set_CD_Drive( CDList.Get_First_CD_Drive() );

			error = CCFileClass::Set_Search_Drives("?:\\");
			switch(error) {
				case 1:
					Set_Palette(GamePalette);
					Show_Mouse();
					CCMessageBox().Process(TXT_CD_ERROR1, TXT_OK);
					Prog_End();
					exit(EXIT_FAILURE);

				case 2:
					Set_Palette(GamePalette);
					Show_Mouse();
					if (CCMessageBox().Process(TXT_CD_DIALOG_1, TXT_OK, TXT_CANCEL) == 1) {
						Prog_End();
						exit(EXIT_FAILURE);
					}
					Hide_Mouse();
					break;

				default:
					Show_Mouse();
					if (!Force_CD_Available(RequiredCD)) {
						Prog_End();
						exit(EXIT_FAILURE);
					}
					Hide_Mouse();
					break;
			}
		} while (error);

#ifdef DEMO
		RequiredCD = -2;
#else
		RequiredCD = -1;
#endif
	} else {

		/*
		** If there are search drives specified then all files are to be
		** considered local.
		*/
		RequiredCD = -2;
	}
#ifndef DEMO
	CCDebugString ("C&C95 - About to register addon mixfiles\n");
	/*
	**	Before all else, cache any additional mixfiles.
	*/
	int scenario_mix_count = 0;
	char** scenario_mix_files = WWFS_GlobDirectory(".", "SC*.MIX", SDL_GLOB_CASEINSENSITIVE, &scenario_mix_count);
	if (scenario_mix_files != NULL) {
		for (int index = 0; index < scenario_mix_count; ++index) {
			char* ptr = strdup(scenario_mix_files[index]);
			new MixFileClass(ptr);
			MixFileClass::Cache(ptr);
			free(ptr);
		}
		SDL_free(scenario_mix_files);
	}

	int score_mix_count = 0;
	char** score_mix_files = WWFS_GlobDirectory(".", "SS*.MIX", SDL_GLOB_CASEINSENSITIVE, &score_mix_count);
	if (score_mix_files != NULL) {
		for (int index = 0; index < score_mix_count; ++index) {
			char* ptr = strdup(score_mix_files[index]);
			new MixFileClass(ptr);
			free(ptr);
		}
		SDL_free(score_mix_files);
	}
#endif	//DEMO

	CCDebugString ("C&C95 - About to register GENERAL.MIX\n");
	if (!BootstrapCoreMixfilesRegistered) {
		if (GeneralMix) delete GeneralMix;
		GeneralMix = new MixFileClass("GENERAL.MIX");
	}

	/*
	**	Inform the file system of the various MIX files.
	*/
#ifdef DEMO
	new MixFileClass("DEMO.MIX");
	if (CCFileClass("DEMOM.MIX").Is_Available()) {
		if (!MoviesMix) MoviesMix = new MixFileClass("DEMOM.MIX");
		ScoresPresent = true;
		ThemeClass::Scan();
	}

#else
	CCDebugString ("C&C95 - About to register CONQUER.MIX\n");
	if (!BootstrapCoreMixfilesRegistered) {
		new MixFileClass("CONQUER.MIX");			// Cached.
	}
	CCDebugString ("C&C95 - About to register TRANSIT.MIX\n");
	new MixFileClass("TRANSIT.MIX");

	CCDebugString ("C&C95 - About to register GENERAL.MIX\n");
	if (!GeneralMix) GeneralMix = new MixFileClass("GENERAL.MIX");			// Never cached.

//	if (CCFileClass("MOVIES.MIX").Is_Available()) {
	CCDebugString ("C&C95 - About to register MOVIES.MIX\n");
		if (!MoviesMix) MoviesMix = new MixFileClass("MOVIES.MIX");			// Never cached.
//	}


	/*
	**	Register the score mixfile.
	*/
	CCDebugString ("C&C95 - About to register SCORES.MIX\n");
	ScoresPresent = false;
//	if (CCFileClass("SCORES.MIX").Is_Available()) {
		ScoresPresent = true;
		if (!ScoreMix) {
			ScoreMix = new MixFileClass("SCORES.MIX");
			ThemeClass::Scan();
		}
//	}
#endif

	/*
	**	These are sound card specific, but the install program would have
	**	copied the coorect versions to the hard drive.
	*/
	CCDebugString ("C&C95 - About to register SPEECH.MIX\n");
	if (CCFileClass("SPEECH.MIX").Is_Available()) {
		new MixFileClass("SPEECH.MIX");			// Never cached.
	}
	CCDebugString ("C&C95 - About to register SOUNDS.MIX\n");
	new MixFileClass("SOUNDS.MIX");			// Cached.

	/*
	**	Initialize the animation system.
	*/
	CCDebugString ("C&C95 - About to initialise the animation system\n");
	Anim_Init();

	/*
	**	Read game options before intro playback so movie/cinematic audio uses
	**	the player's saved slider settings rather than constructor defaults.
	**	(Initialize working palettes first because Options.Load_Settings()
	**	may apply brightness/color adjustments through them.)
	*/
	memcpy(GamePalette, Palette, 768);
	memcpy(OriginalPalette, Palette, 768);
	Options.Load_Settings();

	/*
	**	Play the introduction movies.
	*/
	CCDebugString ("C&C95 - About to play the intro movie\n");
	if (!Special.IsFromInstall) Play_Intro(true);

	/*
	**	Wait for a VSync; during the vertical blank, set the game palette & blit
	**	the title screen.  We must ensure no RGB values in the game palette match
	**	those in the WWLIB's 'CurrentPalette', or the WWLIB palette-set routine
	**	will skip that color; the VQ player will have changed that color (behind
	**	WWLIB's back), so it will be incorrect.
	*/
	memset(CurrentPalette, 0x01, 768);

	if (!Special.IsFromInstall) {
		Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
		HidPage.Blit(SeenBuff);
	}

	Hide_Mouse();
	Wait_Vert_Blank();
	if (!Special.IsFromInstall) {
		Set_Palette(Palette);
		HidPage.Blit(SeenBuff);
		Show_Mouse();
	}
	Call_Back();
//	Window_Dialog_Box(hCCLibrary, "DIALOG_1", MainWindow, MakeProcInstance((FARPROC)Start_Game_Proc, hInstance));
//	if (hCCLibrary) FreeLibrary(hCCLibrary);

#ifdef DEMO
	MixFileClass::Cache("DEMO.MIX");
	MixFileClass::Cache("SOUNDS.MIX");
#else
	/*
	**	Cache the main game data. This operation can take a very long time.
	*/
	MixFileClass::Cache("CONQUER.MIX");
	if (SampleType != 0 && !Debug_Quiet) {
		MixFileClass::Cache("SOUNDS.MIX");
		if (Special.IsJuvenile) {
			new MixFileClass("ZOUNDS.MIX");		// Cached.
			MixFileClass::Cache("ZOUNDS.MIX");
		}
	}
	Call_Back();
#endif

//	malloc(2);

	/*
	**	Perform any special debug-only processing. This includes preparing the
	**	monochrome screen.
	*/
	Mono_Clear_Screen();

#ifdef ONHOLD
	/*
	** Check for addition options not specified on the command-line.  This must
	** be done before the One_Time calls, but after the shape buffer is set up.
	*/
	Parse_INI_File();
#endif

	/*
	**	Perform one-time game system initializations.
	*/
	Call_Back();
//	malloc(3);
	Map.One_Time();
//	malloc(4);
	Logic.One_Time();
//	malloc(5);
	Options.One_Time();

//	malloc(6);

	ObjectTypeClass::One_Time();
	BuildingTypeClass::One_Time();
	BulletTypeClass::One_Time();
	HouseTypeClass::One_Time();

	TemplateTypeClass::One_Time();
	OverlayTypeClass::One_Time();
	SmudgeTypeClass::One_Time();
	TerrainTypeClass::One_Time();
	UnitTypeClass::One_Time();

	InfantryTypeClass::One_Time();
	AnimTypeClass::One_Time();
	AircraftTypeClass::One_Time();
	HouseClass::One_Time();

	/*
	**	Speech holding tank buffer. Since speech does not mix, it can be placed
	**	into a custom holding tank only as large as the largest speech file to
	**	be played.
	*/
	SpeechBuffer = new char [SPEECH_BUFFER_SIZE];
	Call_Back();

	/*
	**	WWLIB bug: MouseState is in some undefined state; show the mouse until
	**	it really shows.
	*/
	Map.Set_Default_Mouse(MOUSE_NORMAL, false);
	Show_Mouse();
//#ifdef FIX_ME_LATER
	while (Get_Mouse_State() > 0) Show_Mouse();
//#endif //FIX_ME_LATER
	Call_Back();

#ifndef DEMO
	/*
	**	Load multiplayer scenario descriptions
	*/
	Read_Scenario_Descriptions();
#endif

	/*
	**	Initialize the multiplayer score values
	*/
	MultiplayerGamesPlayed = 0;
	MultiplayerNumScores = 0;
	MultiplayerCurrentGame = 0;
	for (int i = 0; i < MAX_MULTI_NAMES; i++) {
		MultiplayerScore[i].Name[0] = '\0';
		MultiplayerScore[i].Wins = 0;
		for (int j = 0; j < MAX_MULTI_GAMES; j++) {
			MultiplayerScore[i].Kills[j] = -1;	// -1 = this player didn't play this round
		}
	}

	/*
	** Copy the title screen's palette into the GamePalette & OriginalPalette,
	** because the options Load routine uses these palettes to set the brightness, etc.
	*/
	memcpy (GamePalette, Palette, 768);
	memcpy (OriginalPalette, Palette, 768);

	/*
	**	Read game options, so the GameSpeed is initialized when multiplayer
	** dialogs are invoked.  (GameSpeed must be synchronized between systems.)
	*/
	Options.Load_Settings();

	return(true);
}


#ifndef NOMEMCHECK
void Uninit_Game(void)
{
	delete Map.ShadowPage;
	Map.ShadowPage = NULL;
	Map.Free_Cells();

	delete [] static_cast<char *>(SpeechBuffer);

	CCFileClass::Clear_Search_Drives();
	MixFileClass::Free_All();

	Units.Set_Heap(0);
	Factories.Set_Heap(0);
	Terrains.Set_Heap(0);
	Templates.Set_Heap(0);
	Smudges.Set_Heap(0);
	Overlays.Set_Heap(0);
	Infantry.Set_Heap(0);
	Bullets.Set_Heap(0);
	Buildings.Set_Heap(0);
	Anims.Set_Heap(0);
	Aircraft.Set_Heap(0);
	Triggers.Set_Heap(0);
	TeamTypes.Set_Heap(0);
	Teams.Set_Heap(0);
	while (Houses.Count()) {
		delete Houses.Ptr(0);
	}
	Houses.Set_Heap(0);

	delete [] _ShapeBuffer;
	Set_Shape_Buffer(NULL, 0);
	Free(BlackPalette);
	BlackPalette = NULL;
	Free(GamePalette);
	GamePalette = NULL;
	Free(OriginalPalette);
	OriginalPalette = NULL;
	Free(WhitePalette);
	WhitePalette = NULL;

	WWDOS_Shutdown();
	Free(Palette);
	Palette = NULL;
}
#endif

extern GameType Do_The_Internet_Menu_Thang(void);
extern int ShowCommand;

/***********************************************************************************************
 * Select_Game -- The game's main menu                                                         *
 *                                                                                             *
 * INPUT:                                                                                      *
 *		fade		if true, will fade the palette in gradually												  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *		none.																												  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *		none.																												  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/05/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
extern int Com_Fake_Scenario_Dialog(void);
extern int Com_Show_Fake_Scenario_Dialog(void);
extern int InternetMaxAhead;
extern int InternetSendRate;

bool Select_Game(bool fade)
{
	enum {
		SEL_TIMEOUT = -1,				// main menu timeout--go into attract mode
#ifdef NEWMENU
		SEL_NEW_SCENARIO,				// Expansion scenario to play.
#endif
		SEL_START_NEW_GAME,			// start a new game
#ifdef BONUS_MISSIONS
		SEL_BONUS_MISSIONS,
#endif	//BONUS_MISSIONS
		SEL_INTERNET,
		SEL_LOAD_MISSION,				// load a saved game
		SEL_MULTIPLAYER_GAME,		// play multiplayer network game
		SEL_INTRO,						// replay the intro
		SEL_EXIT,						// exit to DOS
		SEL_FAME,						// view the hall of fame
		SEL_NONE,						// placeholder default value
	};
	bool gameloaded=false;			// Has the game been loaded from the menu?
	int selection;						// the default selection
	bool process = true;				// false = break out of while loop
	bool display = true;
	CountDownTimerClass count;
	int cd_index;

	MEMORYSTATUS	mem_info;
	mem_info.dwLength=sizeof(mem_info);
	GlobalMemoryStatus(&mem_info);

	if (Special.IsFromInstall) {
		/*
		** Special case for machines with 12 megs or less - just play intro, no choose side screen
		*/
		if (mem_info.dwTotalPhys < 12*1024*1024){
			VisiblePage.Clear();
			Play_Movie("INTRO2", THEME_NONE, false);
			BreakoutAllowed = true;
			Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
			fade = true;
			VisiblePage.Clear();
		}else{
			display = false;
			Show_Mouse();
		}
	}

	/*
	**	[Re]set any globals that need it, in preparation for a new scenario
	*/
	GameActive = true;
	DoList.Init();
	OutList.Init();
	Frame = 0;
	PlayerWins = false;
	PlayerLoses = false;
	MultiplayerObiWan = false;
	Debug_Unshroud = false;
	Map.Set_Cursor_Shape(0);
	Map.PendingObjectPtr = 0;
	Map.PendingObject = 0;
	Map.PendingHouse = HOUSE_NONE;

	/*
	** Initialize multiplayer-protocol-specific variables:
	** If CommProtocol MULTI_E_COMP is used, you must:
	** Init FrameSendRate to a sensible value (3 is good)
	** Init MultiplayerMaxAhead to an even multiple of FrameSendRate, and it must
	**   be at least 2 * MultiplayerMaxAhead
	*/
	CommProtocol = COMM_PROTOCOL_SINGLE_NO_COMP;
	FrameSendRate = 3;

	ProcessTicks = 0;
	ProcessFrames = 0;
	DesiredFrameRate = 30;
//#if(TIMING_FIX)
	NewMaxAheadFrame1 = 0;
	NewMaxAheadFrame2 = 0;
//#endif

	/*
	**	Init multiplayer game scores.  Let Wins accumulate; just init the current
	** Kills for this game.  Kills of -1 means this player didn't play this round.
	*/
	for (int i = 0 ; i < MAX_MULTI_GAMES; i++) {
		MultiplayerScore[i].Kills[MultiplayerCurrentGame] = -1;
	}

	/*
	**	Set default mouse shape
	*/
	Map.Set_Default_Mouse(MOUSE_NORMAL, false);

	/*
	**	If the last game we played was a multiplayer game, jump right to that
	**	menu by pre-setting 'selection'.
	*/
	if (GameToPlay == GAME_NORMAL) {
		selection = SEL_NONE;
	} else {
		selection = SEL_MULTIPLAYER_GAME;
	}

	/*
	**	Main menu processing; only do this if we're not in editor mode.
	*/
	if (!Debug_Map) {

		/*
		**	Menu selection processing loop
		*/
		ScenarioInit++;
		Theme.Queue_Song(THEME_MAP1);
		ScenarioInit--;

		/*
		** If we're playing back a recording, load all pertinant values & skip
		** the menu loop.  Hide the now-useless mouse pointer.
		*/
		if (PlaybackGame && RecordFile.Is_Available()) {
			if (RecordFile.Open(READ)) {
				Load_Recording_Values();
				process = false;
				Theme.Fade_Out();
			} else
				PlaybackGame = false;
		}



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
			**	Redraw the title page if needed
			*/
			if (display) {
				Hide_Mouse();

				/*
				**	Display the title page; fade it in if this is the first time
				**	through the loop, and the 'fade' flag is true
				*/
				Load_Title_Screen("HTITLE.PCX", &HidPage, Palette);
				memcpy (GamePalette, Palette, 768);
				HidPage.Blit(SeenBuff);

				if (fade) {
					Fade_Palette_To(Palette, FADE_PALETTE_SLOW, Call_Back);
					fade = false;
				}

				Set_Logic_Page(SeenBuff);
#ifdef VIRGIN_CHEAT_KEYS
				Fancy_Text_Print("V.%d%s", SeenBuff.Get_Width() - 1, SeenBuff.Get_Height() - 10, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
//				Fancy_Text_Print("V.%d%s%02d", 319, 190, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText, FOREIGN_VERSION_NUMBER);
#else

#ifdef DEMO
				Version_Number();
				Fancy_Text_Print("DEMO V%s", SeenBuff.Get_Width() - 1, SeenBuff.Get_Height() - 10, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, VersionText);
#else
				Fancy_Text_Print("V.%d%s", SeenBuff.Get_Width() - 1, SeenBuff.Get_Height() - 10, DKGREY, TBLACK, TPF_6POINT|TPF_FULLSHADOW|TPF_RIGHT, Version_Number(), VersionText);
#endif
#endif
				display = false;
				Show_Mouse();
			}

			/*
			**	Display menu and fetch selection from player.
			*/
			if (Special.IsFromInstall && mem_info.dwTotalPhys >= 12*1024*1024){
				selection = SEL_START_NEW_GAME;
				Theme.Queue_Song(THEME_NONE);
			}

			if (selection == SEL_NONE) {
//				selection = Main_Menu(0);
				selection = Main_Menu(ATTRACT_MODE_TIMEOUT);
			}
			Call_Back();

			switch (selection) {

#ifdef NEWMENU

				case SEL_INTERNET:
					GameToPlay = Do_The_Internet_Menu_Thang();
					if (GameToPlay != GAME_NORMAL) {
						display = false;
						selection = SEL_MULTIPLAYER_GAME;
					} else {
						selection = SEL_NONE;
						display = true;
					}
					break;


				/*
				**	Pick an expansion scenario.
				*/
				case SEL_NEW_SCENARIO:
					CarryOverMoney = 0;
					if (Expansion_Dialog()) {
						Theme.Fade_Out();
//						Theme.Queue_Song(THEME_AOI);
						GameToPlay = GAME_NORMAL;
						process = false;
					} else {
						display = true;
						selection = SEL_NONE;
					}
					break;


#ifdef BONUS_MISSIONS

				/*
				**	User selected to play a bonus scenario.
				*/
				case SEL_BONUS_MISSIONS:
					CarryOverMoney = 0;

					/*
					** Ensure that CD1 or CD2 is in the drive. These missions
					** are not on the covert CD.
					*/
					cd_index = Get_CD_Index(CCFileClass::Get_CD_Drive(), 1*60);
					/*
					** If cd_index == 2 then its a covert CD
					*/
					if (cd_index == 2){
						RequiredCD = 0;
						if (!Force_CD_Available (RequiredCD)){
							Prog_End();
							exit(EXIT_FAILURE);
						}
					}

					if (Bonus_Dialog()) {
						Theme.Fade_Out();
						GameToPlay = GAME_NORMAL;
						process = false;
					} else {
						display = true;
						selection = SEL_NONE;
					}
					break;


#endif	//BONUS_MISSIONS

#endif

				/*
				**	SEL_START_NEW_GAME: Play the game
				*/
				case SEL_START_NEW_GAME:
					CarryOverMoney = 0;

#ifdef DEMO
					Hide_Mouse();
					Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
					Load_Title_Screen("PREPICK.PCX", &HidPage, Palette);
					HidPage.Blit(SeenBuff);
					Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
					Clear_KeyBuffer();
					while (!Check_Key_Num()) {
						Call_Back();
					}
					Get_Key_Num();
					Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
					Show_Mouse();

					Scenario = 1;
					BuildLevel = 1;
#else
					Scenario = 1;
					BuildLevel = 1;
#endif
					ScenPlayer = SCEN_PLAYER_GDI;
					ScenDir = SCEN_DIR_EAST;
					Whom = HOUSE_GOOD;

#ifndef DEMO
					Theme.Fade_Out();
					Choose_Side();
#endif

					/*
					** If user is playing special mode, do NOT change Whom; leave it set to
					** GDI or NOD.  Ini.cpp will set the player's ActLike to mirror the
					** Whom value.
					*/
					if (Special.IsJurassic && AreThingiesEnabled) {
						ScenPlayer = SCEN_PLAYER_JP;
						ScenDir = SCEN_DIR_EAST;
					}

					GameToPlay = GAME_NORMAL;
					process = false;
					break;

				/*
				**	Load a saved game.
				*/
				case SEL_LOAD_MISSION:
					if (LoadOptionsClass(LoadOptionsClass::LOAD).Process()) {
						//Theme.Fade_Out();
						Theme.Queue_Song(THEME_AOI);
						process = false;
						gameloaded = true;
					} else {
						display = true;
						selection = SEL_NONE;
					}
					break;

				/*
				**	SEL_MULTIPLAYER_GAME: set 'GameToPlay' to multiplayer
				**	network play.
				*/
				case SEL_MULTIPLAYER_GAME:

#ifdef DEMO
					Hide_Mouse();
					Set_Palette(BlackPalette);
					Load_Title_Screen("DEMOPIC.PCX", &HidPage, Palette);
					HidPage.Blit(SeenBuff);
					Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
					Clear_KeyBuffer();
					while (!Check_Key()) {
						Call_Back();
					}
					Get_Key();
					Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
					Show_Mouse();
					display = true;
					fade = true;
					selection = SEL_NONE;
#else
					switch (GameToPlay) {

						/*
						**	If 'GameToPlay' isn't already set up for a multiplayer game,
						**	we must prompt the user for which type of multiplayer game
						**	they want.
						*/
						case GAME_NORMAL:
							GameToPlay = Select_Multiplayer_Game();
							if (GameToPlay == GAME_NORMAL) {		// 'Cancel'
								display = true;
								selection = SEL_NONE;
							}
							break;

						case GAME_UDP:
							if (!Init_Network()) {
								GameToPlay = GAME_NORMAL;
								selection = SEL_NONE;
								display = true;
								break;
							}

							if (Remote_Connect()) {
								break;
							}

							Shutdown_Network();
							GameToPlay = GAME_NORMAL;
							selection = SEL_NONE;
							display = true;
							break;

						case GAME_INTERNET:
							if (!Winsock.Init()) {
								GameToPlay = GAME_NORMAL;
								selection = SEL_NONE;
								display = true;
								break;
							}

							Read_MultiPlayer_Settings();
							Server = PlanetWestwoodIsHost;
							Winsock.Set_Protocol_UDP(GameToPlay != GAME_INTERNET);
							Winsock.Set_Host_Address(PlanetWestwoodIPAddress);

							if (Server) {
								InternetGameRole = INTERNET_HOST;
								Winsock.Start_Server();
							} else {
								InternetGameRole = INTERNET_JOIN;
								Winsock.Start_Client();
							}

							if (!Init_Network()) {
								Winsock.Close();
								GameToPlay = GAME_NORMAL;
								selection = SEL_NONE;
								display = true;
								break;
							}

							if ((Server && Server_Remote_Connect()) || (!Server && Client_Remote_Connect())) {
								break;
							}

							Winsock.Close();
							GameToPlay = GAME_NORMAL;
							selection = SEL_NONE;
							display = true;
							break;

						case GAME_WOL: {
							char server_url[256];
							char const *server_host = PlanetWestwoodIPAddress[0] ? PlanetWestwoodIPAddress : "127.0.0.1";
							char const *nickname = MultiplayerName[0] ? MultiplayerName : "Noname";

							Read_MultiPlayer_Settings();
							server_host = PlanetWestwoodIPAddress[0] ? PlanetWestwoodIPAddress : "127.0.0.1";
							nickname = MultiplayerName[0] ? MultiplayerName : "Noname";

							if (SDL_strstr(server_host, "://") != NULL) {
								SDL_strlcpy(server_url, server_host, sizeof(server_url));
							} else {
								SDL_snprintf(server_url, sizeof(server_url), "ws://%s:%u/ws", server_host, PlanetWestwoodPortNumber);
							}

							Wol.Configure(server_url, nickname);

							if (!Init_Network()) {
								GameToPlay = GAME_NORMAL;
								selection = SEL_NONE;
								display = true;
								break;
							}

							if (Remote_Connect()) {
								break;
							}

							Shutdown_Network();
							GameToPlay = GAME_NORMAL;
							selection = SEL_NONE;
							display = true;
							break;
						}

						default:
							break;
						}

						switch (GameToPlay) {
						/*
						**	Modern direct-connect and WOL-style games
						*/
						case GAME_INTERNET:
						case GAME_WOL:
						case GAME_UDP:
							Theme.Fade_Out();
							ScenPlayer = SCEN_PLAYER_2PLAYER;
							ScenDir = SCEN_DIR_EAST;
							process = false;
							Options.ScoreVolume = 0;
							break;

						default:
							break;
					}
#endif
					break;

				/*
				**	Play a VQ
				*/
				case SEL_INTRO:
					Theme.Fade_Out();
					Theme.Stop();
					Call_Back();

					Force_CD_Available(-1);
					Play_Intro(false);
					Hide_Mouse();

// verify existance of movie file before playing this sequence.
					if (CCFileClass("TRAILER.VQA").Is_Available()) {
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
						VisiblePage.Clear();
						if (CCFileClass("ATTRACT2.CPS").Is_Available()){
							Load_Uncompress(CCFileClass("ATTRACT2.CPS"), SysMemPage, SysMemPage, Palette);
							SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
							Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
						}
						Clear_KeyBuffer();
						count.Set(TIMER_SECOND*1);
						Wait_For_Countdown_With_Callback(count);
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);

						Play_Movie("TRAILER");	// Red Alert teaser.
					}

					if (CCFileClass("SIZZLE.VQA").Is_Available()) {
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
						VisiblePage.Clear();
						if (CCFileClass("ATTRACT2.CPS").Is_Available()){
							Load_Uncompress(CCFileClass("ATTRACT2.CPS"), SysMemPage, SysMemPage, Palette);
							SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
							Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
						}
						Clear_KeyBuffer();
						count.Set(TIMER_SECOND*1);
						Wait_For_Countdown_With_Callback(count);
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);

						Play_Movie("SIZZLE");	// Red Alert teaser.
					}

					if (CCFileClass("SIZZLE2.VQA").Is_Available()) {
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
						VisiblePage.Clear();
						if (CCFileClass("ATTRACT2.CPS").Is_Available()){
							Load_Uncompress(CCFileClass("ATTRACT2.CPS"), SysMemPage, SysMemPage, Palette);
							SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
							Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
						}
						Clear_KeyBuffer();
						count.Set(TIMER_SECOND*1);
						Wait_For_Countdown_With_Callback(count);
						Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);

						Play_Movie("SIZZLE2");	// Red Alert teaser.
					}

					Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
					VisiblePage.Clear();
					if (CCFileClass("ATTRACT2.CPS").Is_Available()){
						Load_Uncompress(CCFileClass("ATTRACT2.CPS"), SysMemPage, SysMemPage, Palette);
						SysMemPage.Scale(SeenBuff, 0, 0, 0, 0, 320, 199, 640, 398);
						Fade_Palette_To(Palette, FADE_PALETTE_MEDIUM, Call_Back);
					}
					Clear_KeyBuffer();
					count.Set(TIMER_SECOND*3);
					Wait_For_Countdown_With_Callback(count);
					Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);

					Play_Movie("CC2TEASE");
					Show_Mouse();

					ScenarioInit++;
					Theme.Play_Song (THEME_MAP1);
					ScenarioInit--;
					display = true;
					fade = true;
					selection = SEL_NONE;
					break;

				/*
				**	Exit to DOS.
				*/
				case SEL_EXIT:
#ifdef JAPANESE
	Hide_Mouse();
#endif
					Theme.Fade_Out();
					Fade_Palette_To(BlackPalette, FADE_PALETTE_SLOW, NULL);
#ifdef JAPANESE
	VisiblePage.Clear();
#endif
					return(false);

				/*
				**	Display the hall of fame.
				*/
				case SEL_FAME:
					break;

				case SEL_TIMEOUT:
					if (AllowAttract && RecordFile.Is_Available()) {
						PlaybackGame = true;
						if (RecordFile.Open(READ)) {
							Load_Recording_Values();
							process = false;
							Theme.Fade_Out();
						} else {
							PlaybackGame = false;
							selection = SEL_NONE;
						}
					} else {
						selection = SEL_NONE;
					}
					break;

				default:
					break;
			}
		}
	} else {

		/*
		** For Debug_Map (editor) mode, if JP option is on, set to load that scenario
		*/
		Scenario = 1;
		if (Special.IsJurassic && AreThingiesEnabled) {
			ScenPlayer = SCEN_PLAYER_JP;
			ScenDir = SCEN_DIR_EAST;
		}
	}
	CCDebugString ("C&C95 - About to start game initialisation.\n");
#ifdef FORCE_WINSOCK
	if (Is_Online_Game(GameToPlay)){
		CommProtocol = COMM_PROTOCOL_MULTI_E_COMP;
		FrameSendRate = (GameToPlay == GAME_WOL) ? InternetSendRate : 5;
	}
#endif	//FORCE_WINSOCK
	/*
	**	Don't carry stray keystrokes into game.
	*/
	Kbd.Clear();

	/*
	** Get a pointer to the compiler's random number seed.
	*/
	srand(0);
	RandSeedPtr = &RandNumb;

	/*
	**	Initialize the random number Seed.  For multiplayer, this will have been done
	** in the connection dialogs.  For single-player games, AND if we're not playing
	** back a recording, init the Seed to a random value.
	*/
	if (GameToPlay == GAME_NORMAL && !PlaybackGame) {
		randomize();
		Seed = rand();
	}

	/*
	** If user has specified a desired random number seed, use it for multiplayer games
	*/
	if (CustomSeed != 0) {
		Seed = CustomSeed;
	}

	/*
	** Save initialization values if we're recording this game.
	** This must be done after 'Seed' has been initialized.
	*/
	if (RecordGame) {
		if (RecordFile.Open(WRITE)) {
			Save_Recording_Values();
		} else {
			RecordGame = false;
		}
	}

	/*
	**	Initialize the random-number generator.
	*/
	//Seed = 1;

	srand(Seed);
	RandNumb = Seed;
	SimRandIndex = 0;

	/*
	**	Load the scenario.  Specify variation 'A' for the editor; for the game,
	**	don't specify a variation, to make 'Set_Scenario_Name()' pick a random one.
	**	Skip this if we've already loaded a save-game.
	*/
	if (!gameloaded) {
		if (Debug_Map) {
			Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir, SCEN_VAR_A);
		} else {
			Set_Scenario_Name(ScenarioName, Scenario, ScenPlayer, ScenDir);
		}

		/*
		** Start_Scenario() changes the palette; so, fade out & clear the screen
		** before calling it.
		*/
		Hide_Mouse();

		if (selection != SEL_START_NEW_GAME) {
			Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
			HiddenPage.Clear();
			VisiblePage.Clear();
		}
		Show_Mouse();

		Special.IsFromInstall = 0;
		CCDebugString ("C&C95 - Starting scenario.\n");
		if (!Start_Scenario(ScenarioName)) {
			return(false);
		}
		CCDebugString ("C&C95 - Scenario started OK.\n");
	}

	/*
	**	For multiplayer games, initialize the inter-player message system.
	**	Do this after loading the scenario, so the map's upper-left corner is
	**	properly set.
	*/
	CCDebugString ("C&C95 - Initialising message system.\n");
	int factor = (SeenBuff.Get_Width() == 320) ? 1 : 2;
	Messages.Init(Map.TacPixelX, Map.TacPixelY, 6, MAX_MESSAGE_LENGTH, 6*factor+1);

	/*
	**	Hide the SeenBuff; force the map to render one frame.  The caller can
	**	then fade the palette in.
	**	(If we loaded a game, this step will fade out the title screen.  If we
	**	started a scenario, Start_Scenario() will have played a couple of VQ
	**	movies, which will have cleared the screen to black already.)
	*/
	CCDebugString ("C&C95 - About to call Call_Back.\n");
	Call_Back();

	/*
	** This is desperately sad isnt it?
	*/
	Hide_Mouse();
	Hide_Mouse();
	Hide_Mouse();
	Hide_Mouse();
	WWMouse->Erase_Mouse(&HidPage, TRUE);


	Fade_Palette_To(BlackPalette, FADE_PALETTE_MEDIUM, Call_Back);
	HiddenPage.Clear();
	VisiblePage.Clear();
	Set_Logic_Page(SeenBuff);
	Map.Flag_To_Redraw();
	Call_Back();
	Map.Render();
	//Show_Mouse();

	/*
	** Special hack initialization of 'MultiplayerMaxAhead' to accommodate the
	** compression protocol technology.
	*/
#ifdef FORCE_WINSOCK
	if (CommProtocol==COMM_PROTOCOL_MULTI_E_COMP && GameToPlay!=GAME_NORMAL) {
		if (GameToPlay == GAME_WOL) {
			MultiplayerMaxAhead = InternetMaxAhead;
			FrameSendRate = InternetSendRate;
		} else {
			MultiplayerMaxAhead = FrameSendRate * 3;	//2;
		}
	}
#endif	//FORCE_WINSOCK

	if (Debug_Map) {
		while (Get_Mouse_State() > 1) {
			Show_Mouse();
		}
	}

	return(true);
}


/***********************************************************************************************
 * Play_Intro -- plays the introduction & logo movies                                          *
 *                                                                                             *
 * INPUT:                                                                                      *
 *		for_real			if true, this function plays the "real" intro; otherwise, it plays		  *
 *							a delicious smorgasbord of visual delights, guaranteed to titillate		  *
 *							the ocular & auditory nerve pathways.												  *
 *							Well, it plays movies, anyway.														  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *		none.																												  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *		none.																												  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/06/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
static void Play_Intro(bool for_real)
{
	bool playright = !Key_Down(KN_LCTRL) || !Key_Down(KN_RCTRL);
	static int _counter = -1;
	static char const * _names[] = {
#ifdef DEMO
		"LOGO",

#else

		"INTRO2",
//#ifdef CHEAT_KEYS
		"GDIEND1",
		"GDIEND2",
		"GDIFINA",
		"GDIFINB",
		"AIRSTRK",
		"AKIRA",
		"BANNER",
		"BCANYON",
		"BKGROUND",
		"BOMBAWAY",
		"BOMBFLEE",
		"BURDET1",
		"BURDET2",
		"CC2TEASE",
		"CONSYARD",
		"DESFLEES",
		"DESKILL",
		"DESOLAT",
		"DESSWEEP",
		"FLAG",
		"FLYY",
		"FORESTKL",
		"GAMEOVER",
		"GDI1",
		"GDI10",
		"GDI11",
		"GDI12",
		"GDI13",
		"GDI14",
		"GDI15",
		"GDI2",
		"GDI3",
		"GDI3LOSE",
		"GDI4A",
		"GDI4B",
		"GDI5",
		"GDI6",
		"GDI7",
		"GDI8A",
		"GDI8B",
		"GDI9",
		"GDILOSE",
		"GUNBOAT",
		"HELLVALY",
		"INSITES",
		"KANEPRE",
		"LANDING",
		"LOGO",
		"NAPALM",
		"NITEJUMP",
		"NOD1",
		"NOD10A",
		"NOD10B",
		"NOD11",
		"NOD12",
		"NOD13",
		"NOD1PRE",
		"NOD2",
		"NOD3",
		"NOD4A",
		"NOD4B",
		"NOD5",
		"NOD6",
		"NOD7A",
		"NOD7B",
		"NOD8",
		"NOD9",
		"NODEND1",
		"NODEND2",
		"NODEND3",
		"NODEND4",
		"NODFINAL",
		"NODFLEES",
		"NODLOSE",
		"NODSWEEP",
		"NUKE",
		"OBEL",
		"PARATROP",
		"PINTLE",
		"PLANECRA",
		"PODIUM",
		"REFINT",
		"RETRO",
		"SABOTAGE",
		"SAMDIE",
		"SAMSITE",
		"SEIGE",
		"SETHPRE",
		"SPYCRASH",
		"STEALTH",
		"SUNDIAL",
		"TANKGO",
		"TANKKILL",
		"TBRINFO1",
		"TBRINFO2",
		"TBRINFO3",
		"TIBERFX",
		"TRTKIL_D",
		"TURTKILL",
		"VISOR",
//#endif
#endif
		NULL
	};

	Keyboard::Clear();
	if (for_real) {
		Hide_Mouse();
		Play_Movie("LOGO", THEME_NONE, false);
		Show_Mouse();
	} else {
		if (!Debug_Flag) {
			_counter = 0;
		} else {
			if (playright) _counter++;
			if (_counter == -1) _counter = 0;
		}
		Hide_Mouse();
		Play_Movie(_names[_counter], THEME_NONE);
		Show_Mouse();
		if (!_names[_counter]) {
			_counter = -1;
		}
	}
}


/***********************************************************************************************
 * Anim_Init -- Initialize the VQ animation control structure.                                 *
 *                                                                                             *
 *    VQ animations are controlled by a structure passed to the VQ player. This routine        *
 *    initializes the structure to values required by C&C.                                     *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   Only need to call this routine once at the beginning of the game.               *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   12/20/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
extern AudioBackendDevice*		SoundObject;
extern AudioBackendBuffer*	PrimaryBufferPtr;
void Anim_Init(void)
{
	/* Configure player with INI file */
	VQA_DefaultConfig(&AnimControl);
//	void const * font = Load_Font(FONT8);
//	AnimControl.EVAFont = (char *)font;
//	AnimControl.CapFont = (char *)font;

	AnimControl.DrawFlags = VQACFGF_TOPLEFT;
	AnimControl.DrawFlags |= VQACFGF_BUFFER;

	AnimControl.DrawFlags |= VQACFGF_NOSKIP;

	//AnimControl.X1 =0;
	//AnimControl.Y1 =0;
	AnimControl.FrameRate = -1;
	AnimControl.DrawRate = -1;

	AnimControl.DrawerCallback = VQ_Call_Back;
	AnimControl.ImageWidth = 320;
	AnimControl.ImageHeight = 200;
	AnimControl.Vmode = 0;
	AnimControl.ImageBuf = (unsigned char *)SysMemPage.Get_Offset();
	//AnimControl.VBIBit = VertBlank;
	//AnimControl.DrawFlags |= VQACFGF_TOPLEFT;
	AnimControl.OptionFlags |= VQAOPTF_CAPTIONS|VQAOPTF_EVA;

	if (SlowPalette) {
		AnimControl.OptionFlags |= VQAOPTF_SLOWPAL;
	}

//	AnimControl.AudioBuf = (unsigned char *)HidPage.Get_Buffer();
//	AnimControl.AudioBufSize = 32768U;
	//AnimControl.DigiCard = NewConfig.DigitCard;
	//AnimControl.HMIBufSize = 8192;
	//AnimControl.DigiHandle = Get_Digi_Handle();
	//AnimControl.Volume = 0x00FF;
	//AnimControl.AudioRate = 22050;
//	if (NewConfig.Speed) AnimControl.AudioRate = 11025;
	AnimControl.AudioObject = SoundObject;	//Get_Sound_Object();
	AnimControl.PrimaryBuffer = PrimaryBufferPtr;	//Get_Primart_Buffer();
	//if (!Debug_Quiet && Get_Digi_Handle() != -1) {
		//AnimControl.OptionFlags |= VQAOPTF_AUDIO;
	//}

	if (MonoClass::Is_Enabled()) {
		AnimControl.OptionFlags |= VQAOPTF_MONO;
	}
}


/***********************************************************************************************
 * Parse_Command_Line -- Parses the command line parameters.                                   *
 *                                                                                             *
 *    This routine should be called before the graphic mode is initialized. It examines the    *
 *    command line parameters and sets the appropriate globals. If there is an error, then     *
 *    it outputs a command summary and then returns false.                                     *
 *                                                                                             *
 * INPUT:   argc  -- The number of command line arguments.                                     *
 *                                                                                             *
 *          argv  -- Pointer to character string array that holds the individual arguments.    *
 *                                                                                             *
 * OUTPUT:  bool; Was the command line parsed successfully?                                    *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/18/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
bool Parse_Command_Line(int argc, char *argv[])
{
	/*
	**	Parse the command line and set globals to reflect the parameters
	**	passed in.
	*/
#ifdef DEMO
	Scenario = 3;
#else
	Scenario = 1;
#endif
	ScenPlayer = SCEN_PLAYER_GDI;
	ScenDir = SCEN_DIR_EAST;
	Whom = HOUSE_GOOD;
	Special.Init();

	Debug_Map = false;
//	Debug_Play_Map = false;
	Debug_Unshroud = false;

	for (int index = 1; index < argc; index++) {
		char * string;		// Pointer to argument.

		if (argv[index] != NULL && stricmp(argv[index], "-gamedata") == 0) {
			if (index + 1 < argc) {
				++index;
			}
			continue;
		}

		string = strupr(argv[index]);

		/*
		**	Print usage text only if requested.
		*/
		if (stricmp("/?", string) == 0 || stricmp("-?", string) == 0 || stricmp("-h", string) == 0 || stricmp("/h", string) == 0) {
			/*
			**	Unrecognized command line parameter... Display usage
			**	and then exit.
			*/
	#ifdef GERMAN
			puts("Command & Conquer (c) 1995,1996 Westwood Studios\r\n"
						"Parameter:\r\n"
//						"  -CD<Pfad> = Suchpfad f�r Daten-Dateien festlegen.\r\n"
						"  -DESTNET  = Netzwerkkennung des Zielrechners festlegen\r\n"
						"              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
						"  -SOCKET   = Kennung des Netzwerk-Sockets (0 - 16383)\n"
						"  -STEALTH  = Namen im Mehrspieler-Modus verstecken (\"Boss-Modus\")\r\n"
						"  -MESSAGES = Mitteilungen von au�erhalb des Spiels zulassen\r\n"
	//					"  -ELITE    = Fortgeschrittene KI und Gefechtstechniken.\r\n"
						"\r\n");
	#else
	#ifdef FRENCH
			puts("Command & Conquer (c) 1995, Westwood Studios\r\n"
						"Param�tres:\r\n"
//						"  -CD<chemin d'acc�s> = Recherche des fichiers dans le\r\n"
//						"                        r�pertoire indiqu�.\r\n"
						"  -DESTNET  = Sp�cifier le num�ro de r�seau du syst�me de destination\r\n"
						"              (Syntaxe: DESTNETxx.xx.xx.xx)\r\n"
						"  -SOCKET   = ID poste r�seau (0 � 16383)\r\n"
						"  -STEALTH  = Cacher les noms en mode multijoueurs (\"Mode Boss\")\r\n"
						"  -MESSAGES = Autorise les messages ext�rieurs � ce jeu.\r\n"
						"\r\n");
	#else
			puts("Command & Conquer (c) 1995, 1996 Westwood Studios\r\n"
						"Parameters:\r\n"
//						"  -CD<path> = Set search path for data files.\r\n"
						"  -DESTNET  = Specify Network Number of destination system\r\n"
						"              (Syntax: DESTNETxx.xx.xx.xx)\r\n"
						"  -SOCKET   = Network Socket ID (0 - 16383)\n"
						"  -STEALTH  = Hide multiplayer names (\"Boss mode\")\r\n"
						"  -MESSAGES = Allow messages from outside this game.\r\n"
						"  -o        = Enable compatability with version 1.07.\r\n"
#ifdef JAPANESE
						"  -ENGLISH  = Enable English keyboard compatibility.\r\n"
#endif
	//					"  -ELITE    = Advanced AI and combat characteristics.\r\n"
						"\r\n");
	#endif
	#endif
			return(false);
		}


		bool processed = true;
		switch (Obfuscate(string)) {

			/*
			**	Signal that easy mode is active.
			*/
			case PARM_EASY:
				Special.IsHealthBar = true;
				Special.IsEasy = true;
				Special.IsDifficult = false;
				break;

			/*
			**	Signal that hard mode is active.
			*/
			case PARM_HARD:
				Special.IsHealthBar = false;
				Special.IsEasy = false;
				Special.IsDifficult = true;
				break;

#ifdef VIRGIN_CHEAT_KEYS
			case PARM_PLAYTEST:
				Debug_Playtest = true;
				break;
#endif

#ifdef PARM_CHEATERIK
			case PARM_CHEATERIK:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEATADAM
			case PARM_CHEATADAM:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEATMIKE
			case PARM_CHEATMIKE:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEATDAVID
			case PARM_CHEATDAVID:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEATPHIL
			case PARM_CHEATPHIL:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEATBILL
			case PARM_CHEATBILL:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_CHEAT_STEVET

			case PARM_CHEAT_STEVET:
				Debug_Playtest = true;
				Debug_Flag = true;
				break;

#endif

#ifdef PARM_EDITORBILL
			case PARM_EDITORBILL:
				Debug_Map = true;
				Debug_Unshroud = true;
				Debug_Flag = true;
				break;
#endif

#ifdef PARM_EDITORERIK
			case PARM_EDITORERIK:
				Debug_Map = true;
				Debug_Unshroud = true;
				Debug_Flag = true;
				break;
#endif

			case PARM_SPECIAL:
				Special.IsJurassic = true;
				AreThingiesEnabled = true;
				break;

			/*
			** Special flag - is C&C being run from the install program?
			*/
			case PARM_INSTALL:
#ifndef DEMO
				Special.IsFromInstall = true;
#endif
				break;

			default:
				processed = false;
				break;
		}
		if (processed) continue;


#ifdef CHEAT_KEYS
		/*
		**	Scenario Editor Mode
		*/
		if (stricmp(string, "-CHECKMAP") == 0) {
			Debug_Check_Map = true;
			continue;
		}

#endif

		/*
		**	Older version override.
		*/
		if (stricmp(string, "-O") == 0 || stricmp(string, "-0") == 0) {
			IsV107 = true;
			continue;
		}

		/*
		**	File search path override.
		*/
		if (strstr(string, "-CD")) {
			CCFileClass::Set_Search_Drives(&string[3]);
			continue;
		}
#ifdef JAPANESE
		/*
		** Enable english-compatible keyboard
		*/
		if (!stricmp(string,"-ENGLISH")) {
			ForceEnglish = true;
			continue;
		}
#endif

		/*
		**	Specify destination connection for network play
		*/
		if (strstr(string, "-DESTNET")) {
			NetNumType net;
			NetNodeType node;

			/*
			** Scan the command-line string, pulling off each address piece
			*/
			int i = 0;
			char * p = strtok(string + 8,".");
			while (p) {
				int x;

				sscanf(p,"%x",&x);			// convert from hex string to int
				if (i < 4) {
					net[i] = (char)x;			// fill NetNum
				} else {
					node[i-4] = (char)x;		// fill NetNode
				}
				i++;
				p = strtok(NULL,".");
			}

			/*
			** If all the address components were successfully read, fill in the
			** BridgeNet with a broadcast address to the network across the bridge.
			*/
			if (i >= 4) {
				IsBridge = 1;
				memset(node, 0xff, 6);
				BridgeNet = UDPAddressClass(net, node);
			}
			continue;
		}

		/*
		**	Specify socket ID, as an offset from 0x4000.
		*/
		if (strstr(string, "-SOCKET")) {
			unsigned short socket;

			socket = (unsigned short)(atoi(string + strlen("SOCKET")));
			socket += 0x4000;
			if (socket >= 0x4000 && socket < 0x8000) {
				Udp.Set_Socket (socket);
			}
			continue;
		}

		/*
		**	Set the Net Stealth option
		*/
		if (strstr(string, "-STEALTH")) {
			NetStealth = true;
			continue;
		}

		/*
		**	Set the Net Protection option
		*/
		if (strstr(string, "-MESSAGES")) {
			NetProtect = false;
			continue;
		}

		/*
		**	Allow "attract" mode
		*/
		if (strstr(string, "-ATTRACT")) {
			AllowAttract = true;
			continue;
		}

		/*
		** Set screen to 640x480 instead of 640x400
		*/
		if (strstr(string,"-480")){
			ScreenHeight = 480;
			continue;
		}

#ifdef CHEAT_KEYS
		/*
		**	Allow solo net play
		*/
		if (stricmp(string, "-HANSOLO") == 0) {
			MultiplayerSoloAllowed = true;
			continue;
		}

		/*
		**	Specify the random number seed (for debugging)
		*/
		if (strstr(string, "-SEED")) {
			CustomSeed = (unsigned short)(atoi(string + strlen("SEED")));
			continue;
		}
#endif

#ifdef FIX_ME_LATER
		/*
		** look for passed-in video mode to default to
		*/
		if (strnicmp(string, "-V", strlen("-V")) == 0) {
			Set_Video_Mode(MCGA_MODE);	// do this to get around first_time variable...
			Set_Original_Video_Mode(atoi(string+2));
			continue;
		}
#endif //FIX_ME_LATER

		/*
		**	Special command line control parsing.
		*/
		if (strnicmp(string, "-X", strlen("-O")) == 0) {
			string += strlen("-X");
			while (*string) {
				char code = *string++;
				switch (toupper(code)) {

#ifdef ONHOLD
					/*
					**	Should human generated sound effects be used?
					*/
					case 'J':
						Special.IsJuvenile = true;
						break;
#endif

#ifdef  CHEAT_KEYS
					/*
					**	Monochrome debug screen enable.
					*/
					case 'M':
						Special.IsMonoEnabled = true;
						break;

					/*
					**	Inert weapons -- no units take damage.
					*/
					case 'I':
						Special.IsInert = true;
						break;
#endif

#ifdef CHEAT_KEYS
					/*
					**	Hussled recharge timer.
					*/
					case 'H':
						Special.IsSpeedBuild = true;
						break;

					/*
					**	Turn on super-record mode, which thrashes your disk terribly,
					** but is really really cool.  Well, sometimes it is, anyway.
					** At least, it can be.  Once in a while.
					** This flag tells the recording system to re-open the file for
					** each write, so the recording survives a crash.
					*/
					case 'S':
						SuperRecord = 1;
						break;

					/*
					**	"Record" a multi-player game
					*/
					case 'X':
						RecordGame = 1;
						break;

					/*
					**	"Play Back" a multi-player game
					*/
					case 'Y':
						PlaybackGame = 1;
						break;
#endif

#ifdef ONHOLD
					/*
					**	Bonus scenario enable.
					*/
					case 'Z':
						Special.IsJurassic = true;
						break;
#endif

					/*
					**	Quiet mode override control.
					*/
					case 'Q':
						Debug_Quiet = true;
						break;

#ifdef CHEAT_KEYS
					/*
					**	Target selection by human opponent (network play) will
					**	be visible to the player?
					*/
					case 'V':
						Special.IsVisibleTarget = true;
						break;
#endif

					default:
#ifdef GERMAN
						puts("Ung�ltiger Parameter.\n");
#else
#ifdef FRENCH
						puts("Commande d'option invalide.\n");
#else
						puts("Invalid option switch.\n");
#endif
#endif
						return(false);
				}

			}

			if (Special.IsMonoEnabled) {
				MonoClass::Enable();
			}
			continue;
		}
	}
	return(true);
}


#ifdef ONHOLD
/***********************************************************************************************
 * Parse_INI_File -- Parses CONQUER.INI for certain options                                    *
 *                                                                                             *
 * INPUT:                                                                                      *
 *		none.																												  *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *		none.																												  *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *		none.																												  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/18/1995 BRR : Created.                                                                 *
 *=============================================================================================*/
void Parse_INI_File(void)
{
	char *buffer;							// INI staging buffer pointer.
	char buf[128];
	static char section[40];
	static char entry[40];
	static char name[40];
	int len;
	int i;

	/*
	** These arrays store the coded version of the names Geologic, Period, & Jurassic.
	** Decode them by subtracting 83.  For you curious types, the names look like:
	** ��¿º��
	** ��ż·
	** ��Ŵ�Ƽ�
	** If these INI entries aren't found, the IsJurassic flag does nothing.
	*/
	static char coded_section[] = {154,184,194,191,194,186,188,182,0};
	static char coded_entry[] = {163,184,197,188,194,183,0};
	static char coded_name[] = {157,200,197,180,198,198,188,182,0};

	/*------------------------------------------------------------------------
	Fetch working pointer to the INI staging buffer. Make sure that the buffer
	is cleared out before proceeding.
	------------------------------------------------------------------------*/
	buffer = (char *)_ShapeBuffer;
	memset(buffer, '\0', _ShapeBufferSize);

	/*------------------------------------------------------------------------
	Decode the desired section, entry, & name
	------------------------------------------------------------------------*/
	strcpy (section,coded_section);
	len = strlen(coded_section);
	for (i = 0; i < len; i++) {
		section[i] -= 83;
	}

	strcpy (entry,coded_entry);
	len = strlen(coded_entry);
	for (i = 0; i < len; i++) {
		entry[i] -= 83;
	}

	strcpy (name,coded_name);
	len = strlen(coded_name);
	for (i = 0; i < len; i++) {
		name[i] -= 83;
	}

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

	WWGetPrivateProfileString(section, entry, "", buf, sizeof(buf), buffer);

	if (!stricmp (buf,name))
		AreThingiesEnabled = true;

	memset (section, 0, sizeof(section));
	memset (entry, 0, sizeof(entry));
	memset (name, 0, sizeof(name));
}
#endif


/***********************************************************************************************
 * Version_Number -- Determines the version number.                                            *
 *                                                                                             *
 *    This routine will determine the version number by analyzing the date and teim that the   *
 *    program was compiled and then generating a unique version number based on it. The        *
 *    version numbers are guaranteed to be larger for later dates.                             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with the version number.                                                   *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/24/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
int Version_Number(void)
{
#ifdef WIN32

#if (FRENCH)
		sprintf(VersionText, ".02");            // Win95 french version number
#endif	//FRENCH

#if (GERMAN)
		sprintf(VersionText, ".01");            // Win95 german version number
#endif	//GERMAN

#if (JAPANESE)
		sprintf(VersionText, ".01");            // Win95 german version number
#endif	//GERMAN

#if !(FRENCH | GERMAN | JAPANESE)
		sprintf(VersionText, ".07");            // Win95 USA version number
#endif	//FRENCH | GERMAN

		RawFileClass file("VERSION.TXT");
		char version [16];
		memset (version, 0, sizeof (version));
		if (file.Is_Available()){
			file.Read (version, sizeof (version));
		}
		strncat (VersionText, version, sizeof (VersionText) - strlen (VersionText) - 1);

#if (FRENCH)
		return (1);            						// Win95 french version number
#endif	//FRENCH

#if (GERMAN)
		return (1);										// Win95 german version number
#endif	//GERMAN

#if (JAPANESE)
		return (1);										// Win95 german version number
#endif	//GERMAN

#if !(FRENCH | GERMAN | JAPANESE)
		return (1);										// Win95 USA version number
#endif	//FRENCH | GERMAN

#else


#ifdef PATCH

	#ifdef DEMO
		sprintf(VersionText, " 1.0a");		// Demo version.
	#else
		strcpy(VersionText, ".34 ");
	#endif
	return(1);

#else

	#ifdef DEMO
		sprintf(VersionText, " 1.0a");		// Demo version.
	#else
	//	sprintf(VersionText, ".%02dp", 13);			// Patch version.
		sprintf(VersionText, ".%02d", 14);			// Master version.
	#endif
	return(1);
#endif

#endif	//WIN32
}


/***************************************************************************
 * Save_Recording_Values -- Saves recording values to a recording file     *
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
 *   05/15/1995 BRR : Created.                                             *
 *=========================================================================*/
void Save_Recording_Values(void)
{
	RecordFile.Write(&GameToPlay, sizeof(GameToPlay));
	RecordFile.Write(&InternetGameRole, sizeof(InternetGameRole));
	RecordFile.Write(&BuildLevel, sizeof(BuildLevel));
	RecordFile.Write(MultiplayerName, sizeof(MultiplayerName));
	RecordFile.Write(&MultiplayerPreferredColor, sizeof(MultiplayerPreferredColor));
	RecordFile.Write(&MultiplayerColorIndex, sizeof(MultiplayerColorIndex));
	RecordFile.Write(&MultiplayerHouse, sizeof(MultiplayerHouse));
	RecordFile.Write(&MultiplayerLocalID, sizeof(MultiplayerLocalID));
	RecordFile.Write(&MultiplayerPlayerCount, sizeof(MultiplayerPlayerCount));
	RecordFile.Write(&MultiplayerBasesEnabled, sizeof(MultiplayerBasesEnabled));
	RecordFile.Write(&MultiplayerCredits, sizeof(MultiplayerCredits));
	RecordFile.Write(&MultiplayerTiberiumEnabled, sizeof(MultiplayerTiberiumEnabled));
	RecordFile.Write(&MultiplayerGoodiesEnabled, sizeof(MultiplayerGoodiesEnabled));
	RecordFile.Write(&MultiplayerGhostsEnabled, sizeof(MultiplayerGhostsEnabled));
	RecordFile.Write(&MultiplayerUnitCount, sizeof(MultiplayerUnitCount));
	RecordFile.Write(MultiplayerID, sizeof(MultiplayerID));
	RecordFile.Write(MultiplayerHouses, sizeof(MultiplayerHouses));
	RecordFile.Write(&Seed, sizeof(Seed));
	RecordFile.Write(&Scenario, sizeof(Scenario));
	RecordFile.Write(&ScenPlayer, sizeof(ScenPlayer));
	RecordFile.Write(&ScenDir, sizeof(ScenDir));
	RecordFile.Write(&Whom, sizeof(Whom));
	RecordFile.Write(&Special, sizeof(SpecialClass));
	RecordFile.Write(&Options, sizeof(GameOptionsClass));
	RecordFile.Write(&FrameSendRate, sizeof(FrameSendRate));
	RecordFile.Write(&CommProtocol, sizeof(CommProtocol));

	if (SuperRecord) {
		RecordFile.Close();
	}
}


/***************************************************************************
 * Load_Recording_Values -- Loads recording values from recording file     *
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
 *   05/15/1995 BRR : Created.                                             *
 *=========================================================================*/
void Load_Recording_Values(void)
{
	Read_MultiPlayer_Settings();

	RecordFile.Read(&GameToPlay, sizeof(GameToPlay));
	RecordFile.Read(&InternetGameRole, sizeof(InternetGameRole));
	RecordFile.Read(&BuildLevel, sizeof(BuildLevel));
	RecordFile.Read(MultiplayerName, sizeof(MultiplayerName));
	RecordFile.Read(&MultiplayerPreferredColor, sizeof(MultiplayerPreferredColor));
	RecordFile.Read(&MultiplayerColorIndex, sizeof(MultiplayerColorIndex));
	RecordFile.Read(&MultiplayerHouse, sizeof(MultiplayerHouse));
	RecordFile.Read(&MultiplayerLocalID, sizeof(MultiplayerLocalID));
	RecordFile.Read(&MultiplayerPlayerCount, sizeof(MultiplayerPlayerCount));
	RecordFile.Read(&MultiplayerBasesEnabled, sizeof(MultiplayerBasesEnabled));
	RecordFile.Read(&MultiplayerCredits, sizeof(MultiplayerCredits));
	RecordFile.Read(&MultiplayerTiberiumEnabled, sizeof(MultiplayerTiberiumEnabled));
	RecordFile.Read(&MultiplayerGoodiesEnabled, sizeof(MultiplayerGoodiesEnabled));
	RecordFile.Read(&MultiplayerGhostsEnabled, sizeof(MultiplayerGhostsEnabled));
	RecordFile.Read(&MultiplayerUnitCount, sizeof(MultiplayerUnitCount));
	RecordFile.Read(MultiplayerID, sizeof(MultiplayerID));
	RecordFile.Read(MultiplayerHouses, sizeof(MultiplayerHouses));
	RecordFile.Read(&Seed, sizeof(Seed));
	RecordFile.Read(&Scenario, sizeof(Scenario));
	RecordFile.Read(&ScenPlayer, sizeof(ScenPlayer));
	RecordFile.Read(&ScenDir, sizeof(ScenDir));
	RecordFile.Read(&Whom, sizeof(Whom));
	RecordFile.Read(&Special, sizeof(SpecialClass));
	RecordFile.Read(&Options, sizeof(GameOptionsClass));
	RecordFile.Read(&FrameSendRate, sizeof(FrameSendRate));
	RecordFile.Read(&CommProtocol, sizeof(CommProtocol));
}


/***********************************************************************************************
 * Obfuscate -- Sufficiently transform parameter to thwart casual hackers.                     *
 *                                                                                             *
 *    This routine borrows from CRC and PGP technology to sufficiently alter the parameter     *
 *    in order to make it difficult to reverse engineer the key phrase. This is designed to    *
 *    be used for hidden game options that will be released at a later time over Westwood's    *
 *    Web page or through magazine hint articles.                                              *
 *                                                                                             *
 *    Since this is a one way transformation, it becomes much more difficult to reverse        *
 *    engineer the pass phrase even if the resultant pass code is known. This has an added     *
 *    benefit of making this algorithm immune to traditional cyrptographic attacks.            *
 *                                                                                             *
 *    The largest strength of this transformation algorithm lies in the restriction on the     *
 *    source vector being legal ASCII uppercase characters. This restriction alone makes even  *
 *    a simple CRC transformation practically impossible to reverse engineer. This algorithm   *
 *    uses far more than a simple CRC transformation to achieve added strength from advanced   *
 *    attack methods.                                                                          *
 *                                                                                             *
 * INPUT:   string   -- Pointer to the key phrase that will be transformed into a code.        *
 *                                                                                             *
 * OUTPUT:  Returns with the code that the key phrase is translated into.                      *
 *                                                                                             *
 * WARNINGS:   A zero length pass phrase results in a 0x00000000 result code.                  *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/19/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
uint32_t Obfuscate(char const * string)
{
	char buffer[128];

	if (!string) return(0);
	memset(buffer, '\xA5', sizeof(buffer));

	/*
	**	Copy key phrase into a working buffer. This hides any transformation done
	**	to the string.
	*/
	strncpy(buffer, string, sizeof(buffer));
	buffer[sizeof(buffer)-1] = '\0';
	int length = strlen(buffer);
	int index = 0;

	/*
	**	Only upper case letters are significant.
	*/
	strupr(buffer);

	/*
	**	Ensure that only visible ASCII characters compose the key phrase. This
	**	discourages the direct forced illegal character input method of attack.
	*/
	for (int index = 0; index < length; index++) {
		if (!isgraph(buffer[index])) {
			buffer[index] = 'A' + (index%26);
		}
	}

	/*
	**	Increase the strength of even short pass phrases by extending the
	**	length to be at least a minimum number of characters. This helps prevent
	**	a weak pass phrase from compromising the obfuscation process. This
	**	process also forces the key phrase to be an even multiple of four.
	**	This is necessary to support the cypher process that occurs later.
	*/
	if (length < 16 || (length & 0x03)) {
		int maxlen = 16;
		if (((length+3) & 0x00FC) > maxlen) {
			maxlen = ((length+3) & 0x00FC);
		}
		for (index = length; index < maxlen; index++) {
			buffer[index] = 'A' + ((('?' ^ buffer[index-length]) + index) % 26);
		}
		length = index;
		buffer[length] = '\0';
	}

	/*
	**	Transform the buffer into a number. This transformation is character
	**	order dependant.
	*/
	int32_t code = Calculate_CRC(buffer, length);

	/*
	**	Record a copy of this initial transformation to be used in a later
	**	self referential transformation.
	*/
	int32_t copy = code;

	/*
	**	Reverse the character string and combine with the previous transformation.
	**	This doubles the workload of trying to reverse engineer the CRC calculation.
	*/
	SDL_strrev(buffer);
	code ^= Calculate_CRC(buffer, length);

	/*
	**	Perform a self referential transformation. This makes a reverse engineering
	**	by using a cause and effect attack more difficult.
	*/
	code = code ^ copy;

	/*
	**	Unroll and combine the code value into the pass phrase and then perform
	**	another self referential transformation. Although this is a trivial cypher
	**	process, it gives the sophisticated hacker false hope since the strong
	**	cypher process occurs later.
	*/
	SDL_strrev(buffer);		// Restore original string order.
	for (index = 0; index < length; index++) {
		code ^= (unsigned char)buffer[index];
		uint8_t temp = (uint8_t)code;
		buffer[index] ^= temp;
		code >>= 8;
		code |= (((int32_t)temp)<<24);
	}

	/*
	**	Introduce loss into the vector. This strengthens the key against traditional
	**	cryptographic attack engines. Since this also weakens the key against
	**	unconventional attacks, the loss is limited to less than 10%.
	*/
	for (index = 0; index < length; index++) {
		static unsigned char _lossbits[] = {0x00,0x08,0x00,0x20,0x00,0x04,0x10,0x00};
		static unsigned char _addbits[] = {0x10,0x00,0x00,0x80,0x40,0x00,0x00,0x04};

		buffer[index] |= _addbits[index % (sizeof(_addbits)/sizeof(_addbits[0]))];
		buffer[index] &= ~_lossbits[index % (sizeof(_lossbits)/sizeof(_lossbits[0]))];
	}

	/*
	**	Perform a general cypher transformation on the vector
	**	and use the vector itself as the cypher key. This is a variation on the
	**	cypher process used in PGP. It is a very strong cypher process with no known
	**	weaknesses. However, in this case, the cypher key is the vector itself and this
	**	opens up a weakness against attacks that have access to this transformation
	**	algorithm. The sheer workload of reversing this transformation should be enough
	**	to discourage even the most determined hackers.
	*/
	for (index = 0; index < length; index += 4) {
		short key1 = buffer[index];
		short key2 = buffer[index+1];
		short key3 = buffer[index+2];
		short key4 = buffer[index+3];
		short val1 = key1;
		short val2 = key2;
		short val3 = key3;
		short val4 = key4;

		val1 *= key1;
		val2 += key2;
		val3 += key3;
		val4 *= key4;

		short s3 = val3;
		val3 ^= val1;
		val3 *= key1;
		short s2 = val2;
		val2 ^= val4;
		val2 += val3;
		val2 *= key3;
		val3 += val2;

		val1 ^= val2;
		val4 ^= val3;

		val2 ^= s3;
		val3 ^= s2;

		buffer[index] = val1;
		buffer[index+1] = val2;
		buffer[index+2] = val3;
		buffer[index+3] = val4;
	}

	/*
	**	Convert this final vector into a cypher key code to be
	**	returned by this routine.
	*/
	code = Calculate_CRC(buffer, length);

	/*
	**	Return the final code value.
	*/
	return(code);
}
