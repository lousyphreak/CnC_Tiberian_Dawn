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

/* $Header:   F:\projects\c&c\vcs\code\startup.cpv   2.17   16 Oct 1995 16:48:12   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Command & Conquer                                            *
 *                                                                                             *
 *                    File Name : STARTUP.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : October 3, 1994                                              *
 *                                                                                             *
 *                  Last Update : August 27, 1995 [JLB]                                        *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Delete_Swap_Files -- Deletes previously existing swap files.                              *
 *   Prog_End -- Cleans up library systems in prep for game exit.                              *
 *   main -- Initial startup routine (preps library systems).                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include	"FUNCTION.H"
#include "ra_messagebox.h"

#include <SDL3/SDL_filesystem.h>

#include <cstdio>
#include <cstring>
#include <string>

bool Read_Private_Config_Struct(char *profile, NewConfigType *config);
void Delete_Swap_Files(void);
void Print_Error_End_Exit(char *string);
void Print_Error_Exit(char *string);
WinTimerClass *WinTimer;
extern void Create_Main_Window ( HANDLE instance , int command_show , int width , int height);

extern bool ReadyToQuit;
void Read_Setup_Options(char *config_data);

bool VideoBackBufferAllowed = true;

namespace
{
const char* const kStartupGameDataBaseMixfiles[] = {
	"CONQUER.MIX",
	"GENERAL.MIX"
};

void copy_base_path(char* buffer, size_t buffer_size)
{
	if (buffer == NULL || buffer_size == 0) {
		return;
	}

	const char* base_path = SDL_GetBasePath();
	std::string base_path_string = (base_path != NULL) ? base_path : "./";
	if (!base_path_string.empty() && base_path_string.back() != '/' && base_path_string.back() != '\\') {
		base_path_string.push_back('/');
	}
	std::snprintf(buffer, buffer_size, "%s", base_path_string.c_str());
}

std::string normalize_directory_path(const char* path)
{
	std::string normalized = WWFS_NormalizePath(path);
	while (normalized.size() > 1 && (normalized.back() == '/' || normalized.back() == '\\')) {
		normalized.pop_back();
	}
	return normalized.empty() ? std::string(".") : normalized;
}

std::string current_working_directory_path()
{
	char* current_directory = SDL_GetCurrentDirectory();
	if (current_directory == NULL || current_directory[0] == '\0') {
		if (current_directory != NULL) {
			SDL_free(current_directory);
		}
		return std::string();
	}

	std::string normalized = normalize_directory_path(current_directory);
	SDL_free(current_directory);
	return normalized;
}

std::string startup_base_directory_path()
{
	const char* base_path = SDL_GetBasePath();
	if (base_path != NULL && base_path[0] != '\0') {
		return normalize_directory_path(base_path);
	}

	return std::string();
}

std::string executable_directory_path(const char* executable_path)
{
	const std::string base_directory = startup_base_directory_path();
	if (!base_directory.empty()) {
		return base_directory;
	}

	char drive[_MAX_DRIVE];
	char path[_MAX_PATH];
	char working_directory[_MAX_PATH];

	WWFS_SplitPath(executable_path, drive, path, NULL, NULL);
	if (!path[0]) {
		std::strcpy(path, ".");
	}
	const size_t path_length = std::strlen(path);
	if (path_length > 0 && (path[path_length - 1] == '\\' || path[path_length - 1] == '/')) {
		path[path_length - 1] = '\0';
	}

	working_directory[0] = '\0';
	if (drive[0]) {
		std::strcat(working_directory, drive);
	}
	std::strcat(working_directory, path);
	return normalize_directory_path(working_directory);
}

bool directory_contains_game_data(const std::string& directory)
{
	SDL_PathInfo info;
	if (!WWFS_GetPathInfo(directory.c_str(), &info) || info.type != SDL_PATHTYPE_DIRECTORY) {
		return false;
	}

	for (size_t index = 0; index < (sizeof(kStartupGameDataBaseMixfiles) / sizeof(kStartupGameDataBaseMixfiles[0])); ++index) {
		const std::string mixfile = directory + "/" + kStartupGameDataBaseMixfiles[index];
		if (!WWFS_GetPathInfo(mixfile.c_str(), NULL)) {
			return false;
		}
	}

	return true;
}

bool try_get_command_line_gamedata_directory(int argc, char* argv[], std::string* directory)
{
	if (directory == NULL) {
		return false;
	}

	for (int index = 1; index < argc; ++index) {
		const char* argument = argv[index];
		if (argument == NULL || SDL_strcasecmp(argument, "-gamedata") != 0) {
			continue;
		}
		if (index + 1 >= argc || argv[index + 1] == NULL || argv[index + 1][0] == '\0') {
			return false;
		}

		*directory = normalize_directory_path(argv[index + 1]);
		return true;
	}

	return false;
}

std::string resolve_game_data_directory(int argc, char* argv[], const char* executable_path)
{
	std::string command_line_directory;
	if (try_get_command_line_gamedata_directory(argc, argv, &command_line_directory) && directory_contains_game_data(command_line_directory)) {
		return command_line_directory;
	}

	const std::string current_working_directory = current_working_directory_path();
	if (!current_working_directory.empty() && directory_contains_game_data(current_working_directory)) {
		return current_working_directory;
	}

	const std::string executable_directory = executable_directory_path(executable_path);
	if (directory_contains_game_data(executable_directory)) {
		return executable_directory;
	}

	return std::string();
}

void print_missing_game_data_error(int argc, char* argv[], const char* executable_path)
{
	std::string command_line_directory;
	const bool has_command_line_directory = try_get_command_line_gamedata_directory(argc, argv, &command_line_directory);
	const std::string current_working_directory = current_working_directory_path();
	const std::string executable_directory = executable_directory_path(executable_path);

	std::fprintf(stderr, "Required game data was not found.\n");
	std::fprintf(stderr, "Checked for CONQUER.MIX and GENERAL.MIX in this order:\n");
	if (has_command_line_directory) {
		std::fprintf(stderr, "  1. -gamedata %s\n", command_line_directory.c_str());
	} else {
		std::fprintf(stderr, "  1. -gamedata <dir> (not provided)\n");
	}
	std::fprintf(stderr,
		"  2. current working directory: %s\n",
		current_working_directory.empty() ? "(unavailable)" : current_working_directory.c_str());
	std::fprintf(stderr,
		"  3. executable directory: %s\n",
		executable_directory.empty() ? "(unavailable)" : executable_directory.c_str());
	std::fprintf(stderr, "Use -gamedata <dir> or launch the game from a directory that contains those files.\n");
}
}


/***********************************************************************************************
 * main -- Initial startup routine (preps library systems).                                    *
 *                                                                                             *
 *    This is the routine that is first called when the program starts up. It basically        *
 *    handles the command line parsing and setting up library systems.                         *
 *                                                                                             *
 * INPUT:   argc  -- Number of command line arguments.                                         *
 *                                                                                             *
 *          argv  -- Pointer to array of comman line argument strings.                         *
 *                                                                                             *
 * OUTPUT:  Returns with execution failure code (if any).                                      *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/

HANDLE	ProgramInstance;
extern BOOL CC95AlreadyRunning;
void Move_Point(short &x, short &y, DirType dir, unsigned short distance);

void Check_Use_Compressed_Shapes (void);

int main(int argc, char *argv[])
{
// Heap_Dump_Check( "first thing in main" );
//	malloc(1);

	CCDebugString ("C&C95 - Starting up.\n");


	//WindowsTimer = new WinTimerClass(60,FALSE);
	//CD_Test();



	/*
	** These values return 0x47 if code is working correctly
	*/
//	int temp = Desired_Facing256 (1070, 5419, 1408, 5504);



	/*
	** If we are already running then switch to the existing process and exit
	*/
	if (CC95AlreadyRunning) {
		//MessageBox (NULL, "Error - attempt to restart C&C95 when already running.", "Command & Conquer", MB_ICONEXCLAMATION|MB_OK);

		return (EXIT_SUCCESS);
	}

	if (Ram_Free(MEM_NORMAL) < 5000000) {
#ifdef GERMAN
		printf("Zuwenig Hauptspeicher verf�gbar.\n");
#else
#ifdef FRENCH
		printf("M�moire vive (RAM) insuffisante.\n");
#else
		printf("Insufficient RAM available.\n");
#endif
#endif
		return(EXIT_FAILURE);
	}


	//void *test_buffer = Alloc(20,MEM_NORMAL);

	//memset ((char*)test_buffer, 0, 21);

	//Free(test_buffer);


	HANDLE	instance = NULL;
	int		command_show = SW_SHOWMAXIMIZED;
	int		startup_argc;				// Command line argument count
	char *	startup_argv[20];		// Pointers to command line arguments
	char		path_to_exe[280];
	char*	fallback_argv[1];

	ProgramInstance = instance;

	if (argv == NULL || argc <= 0 || argv[0] == NULL || argv[0][0] == '\0') {
		copy_base_path(path_to_exe, sizeof(path_to_exe));
		fallback_argv[0] = &path_to_exe[0];
		argv = fallback_argv;
		argc = 1;
	}

	/*
	** Get the full path to the .EXE
	*/
	copy_base_path(path_to_exe, sizeof(path_to_exe));

	/*
	** First argument is supposed to be a pointer to the .EXE that is running
	**
	*/
	startup_argc = 1;
	startup_argv[0] = &path_to_exe[0];
	for (int argument_index = 1; argument_index < argc && startup_argc < 20; ++argument_index) {
		startup_argv[startup_argc++] = argv[argument_index];
	}



	/*
	**	Select the asset directory in this exact order:
	**	1) -gamedata <dir>
	**	2) current working directory
	**	3) executable directory
	*/
	const std::string game_data_directory = resolve_game_data_directory(startup_argc, startup_argv, startup_argv[0]);
	if (game_data_directory.empty()) {
		print_missing_game_data_error(startup_argc, startup_argv, startup_argv[0]);
		return(EXIT_FAILURE);
	}
	if (!WWFS_SetCurrentDirectory(game_data_directory.c_str())) {
		std::fprintf(stderr, "Failed to use game data directory: %s\n", game_data_directory.c_str());
		return(EXIT_FAILURE);
	}

#ifdef JAPANESE
	ForceEnglish = false;
#endif
	if (Parse_Command_Line(startup_argc, startup_argv)) {


		WindowsTimer = new WinTimerClass(60,FALSE);

		uint32_t time_test = WindowsTimer->Get_System_Tick_Count();
		SDL_Delay(1000);
		if (WindowsTimer->Get_System_Tick_Count() == time_test){
#ifdef FRENCH
			RA_ShowMessageBox("Error - L'horloge syst�me n'a pas pu s'initialiser en raison de l'instabilit� du syt�me. Vous devez red�marrer Windows.", "Command & Conquer", SDL_MESSAGEBOX_ERROR);
#else
#ifdef	GERMAN
			RA_ShowMessageBox("Fehler - das Timer-System konnte aufgrund einer Instabilit�t des Systems nicht initialisiert werden. Bitte starten Sie Windows neu.", "Command & Conquer", SDL_MESSAGEBOX_ERROR);
#else
			RA_ShowMessageBox("Error - Timer system failed to start due to system instability. You need to restart Windows.", "Command & Conquer", SDL_MESSAGEBOX_ERROR);
#endif	//GERMAN
#endif	//FRENCH
			return(EXIT_FAILURE);
		}

		RawFileClass cfile("CONQUER.INI");

#ifdef JAPANESE
	//////////////////////////////////////if(!ForceEnglish) KBLanguage = 1;
#endif

		/*
		** If there is loads of memory then use uncompressed shapes
		*/
		Check_Use_Compressed_Shapes();

		/*
		** If there is not enough disk space free, dont allow the product to run.
		*/
		if (Disk_Space_Available() < INIT_FREE_DISK_SPACE) {
#ifdef GERMAN
			char	disk_space_message [512];
			sprintf (disk_space_message, "Nicht genug Festplattenplatz f�r Command & Conquer.\nSie brauchen %d MByte freien Platz auf der Festplatte.", (INIT_FREE_DISK_SPACE) / (1024 * 1024));
			RA_ShowMessageBox(disk_space_message, "Command & Conquer", SDL_MESSAGEBOX_WARNING);
			if ( WindowsTimer )
				delete WindowsTimer;
			return (EXIT_FAILURE);
#endif
#ifdef FRENCH
			char	disk_space_message [512];
			sprintf (disk_space_message, "Espace disque insuffisant pour lancer Command & Conquer.\nVous devez disposer de %d Mo d'espace disponsible sur disque dur.", (INIT_FREE_DISK_SPACE) / (1024 * 1024));
			RA_ShowMessageBox(disk_space_message, "Command & Conquer", SDL_MESSAGEBOX_WARNING);
			if ( WindowsTimer )
				delete WindowsTimer;
			return (EXIT_FAILURE);
#endif
#if !(FRENCH | GERMAN)
			if (!RA_ShowConfirmationMessageBox("Warning - you are critically low on free disk space for virtual memory and save games. Do you want to play C&C anyway?", "Command & Conquer", SDL_MESSAGEBOX_WARNING)){

				if ( WindowsTimer )
					delete WindowsTimer;
				return (EXIT_FAILURE);
			}

#endif
		}

		if (cfile.Is_Available()) {
			const int32_t config_size = cfile.Size();
			char* config_data = new char[config_size + 1];
			cfile.Read(config_data, config_size);
			config_data[config_size] = '\0';

			Read_Private_Config_Struct(config_data, &NewConfig);
			Read_Setup_Options(config_data);
			delete[] config_data;

			CCDebugString ("C&C95 - Creating main window.\n");

			Create_Main_Window( instance , command_show , ScreenWidth , ScreenHeight );

			CCDebugString ("C&C95 - Initialising audio.\n");

			SoundOn = Audio_Init ( MainWindow , 16 , false , 11025*2 , 0 );

			Palette = new(MEM_CLEAR) unsigned char[768];

			BOOL video_success = FALSE;
			CCDebugString ("C&C95 - Setting video mode.\n");
			/*
			** Set 640x400 video mode. If its not available then try for 640x480
			*/
			if (ScreenHeight == 400){
				if (Set_Video_Mode (MainWindow, ScreenWidth, ScreenHeight, 8)){
					video_success = TRUE;
				}else{
					if (Set_Video_Mode (MainWindow, ScreenWidth, 480, 8)){
						video_success = TRUE;
						ScreenHeight = 480;
					}
				}
			}else{
				if (Set_Video_Mode (MainWindow, ScreenWidth, ScreenHeight, 8)){
					video_success = TRUE;
				}
			}

			if (!video_success){
				CCDebugString ("C&C95 - Failed to set video mode.\n");
				RA_ShowMessageBox(Text_String(TXT_UNABLE_TO_SET_VIDEO_MODE), "Command & Conquer", SDL_MESSAGEBOX_ERROR);
				if (WindowsTimer) delete WindowsTimer;
				if (Palette) delete [] Palette;
				return (EXIT_FAILURE);
			}

			CCDebugString ("C&C95 - Initialising video surfaces.\n");

			if (ScreenWidth==320){
				VisiblePage.Init( ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)0);
				ModeXBuff.Init( ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)(GBC_VISIBLE | GBC_VIDEOMEM));
			} else {
				VisiblePage.Init( ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)(GBC_VISIBLE | GBC_VIDEOMEM));

				/*
				** If we have enough left then put the hidpage in video memory unless...
				**
				** If there is no blitter then we will get better performance with a system
				** memory hidpage
				**
				** Use a system memory page if the user has specified it via the ccsetup program.
				*/
				CCDebugString ("C&C95 - Allocating back buffer ");
				uint32_t video_memory = VideoDrawObject ? VideoDrawObject->GetTotalVideoMemory() : 0;
				if (video_memory < (uint32_t)(ScreenWidth * ScreenHeight) ||
						!VideoBackBufferAllowed){
					CCDebugString ("in system memory.\n");
					HiddenPage.Init (ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)0);
				} else {
					CCDebugString ("in video memory.\n");
					HiddenPage.Init (ScreenWidth , ScreenHeight , NULL , 0 , (GBC_Enum)GBC_VIDEOMEM);
				}
				VisiblePage.Attach_Surface(&HiddenPage);
			}

			ScreenHeight = 400;

			if (VisiblePage.Get_Height() == 480){
				SeenBuff.Attach(&VisiblePage,0, 40, 640, 400);
				HidPage.Attach(&HiddenPage, 0, 40, 640, 400);
			}else{
				SeenBuff.Attach(&VisiblePage,0, 0, 640, 400);
				HidPage.Attach(&HiddenPage, 0, 0, 640, 400);
			}
			CCDebugString ("C&C95 - Adjusting variables for resolution.\n");
			Options.Adjust_Variables_For_Resolution();

			CCDebugString ("C&C95 - Setting palette.\n");
			/////////Set_Palette(Palette);

			WindowList[0][WINDOWWIDTH] 	= SeenBuff.Get_Width() >> 3;
			WindowList[0][WINDOWHEIGHT]	= SeenBuff.Get_Height();

			/*
			** Install the memory error handler
			*/
			Memory_Error = &Memory_Error_Handler;

			CCDebugString ("C&C95 - Creating mouse class.\n");
			WWMouse = new WWMouseClass(&SeenBuff, 32, 32);
//			MouseInstalled = Install_Mouse(32,24,320,200);
			MouseInstalled = TRUE;

			/*
			** See if we should run the intro
			*/
			CCDebugString ("C&C95 - Reading CONQUER.INI.\n");
			const int32_t ini_size = cfile.Size();
			const int32_t ini_capacity = ini_size + 1024;
			char *buffer = (char*)Alloc(ini_capacity, MEM_NORMAL);		//(char *)HidPage.Get_Buffer();
			cfile.Read(buffer, ini_size);
			buffer[ini_size] = '\0';

			/*
			**	Check for forced intro movie run disabling. If the conquer
			**	configuration file says "no", then don't run the intro.
			*/
			char tempbuff[5];
			WWGetPrivateProfileString("Intro", "PlayIntro", "Yes", tempbuff, 4, buffer);
			if (stricmp(tempbuff, "No") == 0) {
				Special.IsFromInstall = false;
			}else{
				Special.IsFromInstall = true;
			}
			SlowPalette = WWGetPrivateProfileInt("Options", "SlowPalette", 1, buffer);

#ifdef DEMO
			/*
			**	Check for override directory path for CD searches.
			*/
			WWGetPrivateProfileString("CD", "Path", ".", OverridePath, sizeof(OverridePath), buffer);
#endif

			/*
			** Regardless of whether we should run it or not, here we're
			** gonna change it to say "no" in the future.
			*/
			WWWritePrivateProfileString("Intro", "PlayIntro", "No", buffer);
			cfile.Write(buffer, strlen(buffer));

			Free(buffer);

			CCDebugString ("C&C95 - Skipping legacy startup packet path.\n");

			/*
			**	If the intro is being run for the first time, then don't
			**	allow breaking out of it with the <ESC> key.
			*/
			if (Special.IsFromInstall) {
				BreakoutAllowed = false;
			}

			Memory_Error_Exit = Print_Error_End_Exit;

			CCDebugString ("C&C95 - Entering main game.\n");
			Main_Game(startup_argc, startup_argv);

			VisiblePage.Clear();
			HiddenPage.Clear();
//			Set_Video_Mode(RESET_MODE);

			Memory_Error_Exit = Print_Error_Exit;

			CCDebugString ("C&C95 - About to exit.\n");
			ReadyToQuit = 1;
			Prog_End();
			Invalidate_Cached_Icons();
			VisiblePage.Un_Init();
			HiddenPage.Un_Init();
			AllSurfaces.Release();
			Reset_Video_Mode();
			ReadyToQuit = 2;

			CCDebugString ("C&C95 - Returned from final message loop.\n");
			//Prog_End();
			//Invalidate_Cached_Icons();
			//VisiblePage.Un_Init();
			//HiddenPage.Un_Init();
			//AllSurfaces.Release();
			//Reset_Video_Mode();
			//Stop_Profiler();
			return (EXIT_SUCCESS);

		} else {
#ifdef GERMAN
			puts("Bitte erst das SETUP-Programm starten.\n");
#else
#ifdef FRENCH
			puts("Lancez d'abord le programme de configuration SETUP.\n");
#else
			puts("Run SETUP program first.");
			puts("\n");
#endif
			Kbd.Get();
#endif
		}

//		Remove_Keyboard_Interrupt();
		if (WindowsTimer){
		 	delete WindowsTimer;
		 	WindowsTimer = NULL;
		}

		if (Palette){
			delete [] Palette;
			Palette = NULL;
		}
	}
	return(EXIT_SUCCESS);
}


/***********************************************************************************************
 * Prog_End -- Cleans up library systems in prep for game exit.                                *
 *                                                                                             *
 *    This routine should be called before the game terminates. It handles cleaning up         *
 *    library systems so that a graceful return to the host operating system is achieved.      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   03/20/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void Prog_End(void)
{
#ifndef DEMO
#endif
	CCDebugString ("C&C95 - About to call Sound_End.\n");
	Sound_End();
	CCDebugString ("C&C95 - Returned from Sound_End.\n");
	if (WWMouse){
		CCDebugString ("C&C95 - Deleting mouse object.\n");
		delete WWMouse;
		WWMouse = NULL;
	}
	if (WindowsTimer){
		CCDebugString ("C&C95 - Deleting windows timer.\n");
		 delete WindowsTimer;
		 WindowsTimer = NULL;
	}

	if (Palette){
		CCDebugString ("C&C95 - Deleting palette object.\n");
		Free(Palette);
		Palette = NULL;
	}
}


/***********************************************************************************************
 * Delete_Swap_Files -- Deletes previously existing swap files.                                *
 *                                                                                             *
 *    This routine will scan through the current directory and delete any swap files it may    *
 *    find. This is used to clear out any left over swap files from previous runs (crashes)    *
 *    of the game. This routine presumes that it cannot delete the swap file that is created   *
 *    by the current run of the game.                                                          *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   08/27/1995 JLB : Created.                                                                 *
 *=============================================================================================*/
void Delete_Swap_Files(void)
{
	int swap_count = 0;
	char** swap_files = WWFS_GlobDirectory(".", "*.SWP", SDL_GLOB_CASEINSENSITIVE, &swap_count);

	if (swap_files != NULL) {
		for (int index = 0; index < swap_count; ++index) {
			WWFS_RemovePath(swap_files[index]);
		}
		SDL_free(swap_files);
	}
}


void Print_Error_End_Exit(char *string)
{
	printf( "%s\n", string );
	Get_Key();
	Prog_End();
	printf( "%s\n", string );
	exit(1);
}


void Print_Error_Exit(char *string)
{
	printf( "%s\n", string );
	exit(1);
}








/***********************************************************************************************
 * Read_Setup_Options -- Read stuff in from the INI file that we need to know sooner           *
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
 *    6/7/96 4:09PM ST : Created                                                               *
 *=============================================================================================*/
void Read_Setup_Options(char *config_data)
{
	if (!config_data) {
		return;
	}

	VideoBackBufferAllowed = WWGetPrivateProfileInt("Options", "VideoBackBuffer", 1, config_data);
	ScreenHeight = WWGetPrivateProfileInt("Options", "Resolution", 0, config_data) ? 480 : 400;
	IsV107 = WWGetPrivateProfileInt("Options", "Compatibility", 0, config_data);

	/*
	** See if an alternative socket number has been specified
	*/
	int socket = WWGetPrivateProfileInt("Options", "Socket", 0, config_data);
	if (socket > 0) {
		socket += 0x4000;
		if (socket >= 0x4000 && socket < 0x8000) {
			Udp.Set_Socket(socket);
		}
	}

	/*
	** See if a destination network has been specified
	*/
	char netbuf[512];
	memset(netbuf, 0, sizeof(netbuf));
	char* netptr = WWGetPrivateProfileString("Options", "DestNet", NULL, netbuf, sizeof(netbuf), config_data);

	if (netptr && strlen(netbuf)) {
		NetNumType net;
		NetNodeType node;

		/*
		** Scan the string, pulling off each address piece
		*/
		int i = 0;
		char* p = strtok(netbuf, ".");
		int x;
		while (p) {
			sscanf(p, "%x", &x);			// convert from hex string to int
			if (i < 4) {
				net[i] = (char)x;			// fill NetNum
			} else {
				node[i - 4] = (char)x;		// fill NetNode
			}
			i++;
			p = strtok(NULL, ".");
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
	}
}
