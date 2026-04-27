/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
*/

#include "FUNCTION.H"
#include "TCPIP.H"

/***************************************************************************
** Internet-specific globals
*/
char PlanetWestwoodHandle[] = {"Handle"};
char PlanetWestwoodPassword[] = {"Password"};
char PlanetWestwoodIPAddress[IP_ADDRESS_MAX] = {"127.0.0.1"};
uint16_t PlanetWestwoodPortNumber = 1234;
bool PlanetWestwoodIsHost = false;
InternetCommandLineType PlanetWestwoodCommandLine = INTERNET_CMDLINE_NONE;
static char PlanetWestwoodCommandLineName[MPLAYER_NAME_MAX] = {""};
uint32_t PlanetWestwoodGameID;
uint32_t PlanetWestwoodStartTime;
bool UseVirtualSubnetServer;
int InternetMaxPlayers;
int InternetMaxAhead = 9;
int InternetSendRate = 3;

bool Internet_Parse_Port(char const* text, uint16_t* port)
{
	char* end = NULL;
	long value;

	if (text == NULL || port == NULL || text[0] == '\0') {
		return(false);
	}

	value = strtol(text, &end, 10);
	if (end == NULL || *end != '\0' || value <= 0 || value > 65535) {
		return(false);
	}

	*port = (uint16_t)value;
	return(true);
}

void Internet_Clear_Command_Line_Connect(void)
{
	PlanetWestwoodCommandLine = INTERNET_CMDLINE_NONE;
}

void Internet_Clear_Command_Line_Name(void)
{
	PlanetWestwoodCommandLineName[0] = '\0';
}

void Internet_Set_Command_Line_Host(uint16_t port)
{
	PlanetWestwoodCommandLine = INTERNET_CMDLINE_HOST;
	PlanetWestwoodIsHost = true;
	PlanetWestwoodIPAddress[0] = '\0';
	PlanetWestwoodPortNumber = port;
}

void Internet_Set_Command_Line_Connect(char const* address, uint16_t port)
{
	PlanetWestwoodCommandLine = INTERNET_CMDLINE_CONNECT;
	PlanetWestwoodIsHost = false;
	SDL_strlcpy(PlanetWestwoodIPAddress, address != NULL ? address : "", sizeof(PlanetWestwoodIPAddress));
	PlanetWestwoodPortNumber = port;
}

void Internet_Set_Command_Line_Name(char const* name)
{
	SDL_strlcpy(PlanetWestwoodCommandLineName, name != NULL ? name : "", sizeof(PlanetWestwoodCommandLineName));
}

bool Internet_Has_Command_Line_Name(void)
{
	return(PlanetWestwoodCommandLineName[0] != '\0');
}

void Internet_Apply_Command_Line_Name(void)
{
	if (PlanetWestwoodCommandLineName[0] != '\0') {
		SDL_strlcpy(MultiplayerName, PlanetWestwoodCommandLineName, sizeof(MultiplayerName));
	}
}

bool Internet_Has_Command_Line_Connect(void)
{
	return(PlanetWestwoodCommandLine != INTERNET_CMDLINE_NONE);
}

bool Internet_Command_Line_Is_Host(void)
{
	return(PlanetWestwoodCommandLine == INTERNET_CMDLINE_HOST);
}

int Read_Game_Options(char* name)
{
	if (!name) {
		return 0;
	}

	char buffer[8192];
	memset(buffer, '\0', sizeof(buffer));

	CCFileClass file(name);
	if (!file.Is_Available()) {
		return 0;
	}

	file.Read(buffer, sizeof(buffer) - 1);
	file.Close();

	WWGetPrivateProfileString("Options", "Handle", "Noname", MultiplayerName,
		sizeof(MultiplayerName), buffer);
	strcpy(MultiplayerGameName, MultiplayerName);
	MultiplayerColorIndex = WWGetPrivateProfileInt("Options", "Color", 0, buffer);
	MultiplayerPreferredColor = MultiplayerColorIndex;
	MultiplayerHouse = (HousesType)WWGetPrivateProfileInt("Options", "Side",
		HOUSE_GOOD, buffer);

	MultiplayerCredits = WWGetPrivateProfileInt("Options", "Credits", 0, buffer);
	MultiplayerBasesEnabled = WWGetPrivateProfileInt("Options", "Bases", 0, buffer);
	MultiplayerTiberiumEnabled = WWGetPrivateProfileInt("Options", "Tiberium", 0, buffer);
	MultiplayerGoodiesEnabled = WWGetPrivateProfileInt("Options", "Crates", 0, buffer);
	MultiplayerGhostsEnabled = WWGetPrivateProfileInt("Options", "AI", 0, buffer);
	BuildLevel = WWGetPrivateProfileInt("Options", "BuildLevel", BuildLevel, buffer);
	if (BuildLevel < 1) {
		BuildLevel = 1;
	}

	MultiplayerUnitCount = WWGetPrivateProfileInt("Options", "UnitCount", 0, buffer);
	Seed = WWGetPrivateProfileInt("Options", "Seed", 0, buffer);
	Special.IsCaptureTheFlag = WWGetPrivateProfileInt("Options", "CaptureTheFlag", 0, buffer);
	PlanetWestwoodGameID = WWGetPrivateProfileInt("Internet", "GameID", 0, buffer);
	PlanetWestwoodStartTime = WWGetPrivateProfileInt("Internet", "StartTime", 0, buffer);
	InternetMaxPlayers = WWGetPrivateProfileInt("Internet", "MaxPlayers", 2, buffer);
	UseVirtualSubnetServer = WWGetPrivateProfileInt("Internet", "UseVSS", 0, buffer);

	if (MultiplayerTiberiumEnabled) {
		Special.IsTGrowth = 1;
		Special.IsTSpread = 1;
	} else {
		Special.IsTGrowth = 0;
		Special.IsTSpread = 0;
	}

	ScenarioIdx = WWGetPrivateProfileInt("Options", "Scenario", 0, buffer);
	Scenario = ScenarioIdx;
	Options.GameSpeed = 0;
	MultiplayerLocalID = Build_MultiplayerID(MultiplayerColorIndex, MultiplayerHouse);

	InternetMaxAhead = WWGetPrivateProfileInt("Timing", "MaxAhead", 9, buffer);
	if (InternetMaxAhead <= 0) {
		InternetMaxAhead = 9;
	}

	InternetSendRate = WWGetPrivateProfileInt("Timing", "SendRate", 3, buffer);
	if (InternetSendRate <= 0) {
		InternetSendRate = 3;
	}

	MultiplayerMaxAhead = InternetMaxAhead;
	FrameSendRate = InternetSendRate;

	return 1;
}

void Just_Path(char* path, char* destpath)
{
	char* terminator = NULL;

	strcpy(destpath, path);
	terminator = strrchr(destpath, '\\');
	if (terminator) {
		*terminator = 0;
	}
}

#ifdef WIN32
extern HKEY Get_Registry_Sub_Key(HKEY base_key, char* search_key, BOOL close);

bool Spawn_Registration_App(void)
{
	HKEY key;
	char inetreg_loc[256];
	DWORD inetreg_loc_size = 256;

	key = Get_Registry_Sub_Key(HKEY_LOCAL_MACHINE, "SOFTWARE", FALSE);
	if (!key) return (FALSE);

	key = Get_Registry_Sub_Key(key, "Westwood", TRUE);
	if (!key) return (FALSE);

	key = Get_Registry_Sub_Key(key, "InetReg", TRUE);
	if (!key) return (FALSE);

	if (RegQueryValueEx(key, "InstallPath", NULL, NULL, (unsigned char*)inetreg_loc, &inetreg_loc_size) != ERROR_SUCCESS) {
		RegCloseKey(key);
		return (FALSE);
	}

	RegCloseKey(key);

	PROCESS_INFORMATION process_info;
	STARTUPINFO start_info;
	char justpath[256];
	memset((void*)&start_info, 0, sizeof(start_info));
	start_info.cb = sizeof(start_info);
	Just_Path(inetreg_loc, justpath);

	BOOL success = CreateProcess(inetreg_loc, NULL, NULL, NULL, false, 0, NULL, justpath, &start_info, &process_info);
	return (success);
}
#else
bool Spawn_Registration_App(void)
{
	return false;
}
#endif

GameType Do_The_Internet_Menu_Thang(void)
{
#ifndef DEMO
	return Select_Multiplayer_Game();
#endif

	return GAME_NORMAL;
}
