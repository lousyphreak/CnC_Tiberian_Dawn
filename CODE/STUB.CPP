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

#define SDL_MAIN_USE_CALLBACKS 1

#include <cstdlib>
#include <string>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_filesystem.h>
#include <SDL3/SDL_main.h>

#include "SDLINPUT.H"
#include "sdl_fs.h"

extern int TiberianDawn_Main(int argc, char* argv[]);

namespace {

struct CallbackAppState
{
	std::vector<std::string> arguments;
	std::vector<char*> argv;
	bool main_started = false;
	bool main_finished = false;
	int exit_code = EXIT_FAILURE;
};

void Build_Startup_Arguments(CallbackAppState& state, int argc, char* argv[])
{
	state.arguments.clear();
	state.argv.clear();

#if defined(__EMSCRIPTEN__) && TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA
	std::string executable_path = WWFS_GetBaseDirectoryPath();
	if (executable_path.empty() || executable_path == ".") {
		executable_path = "/tiberian-dawn";
	} else if (executable_path == "/") {
		executable_path = "/tiberian-dawn";
	} else {
		executable_path += "/tiberian-dawn";
	}
	state.arguments.emplace_back(executable_path);
#else
	const char* base_path = SDL_GetBasePath();
	state.arguments.emplace_back((base_path != NULL && base_path[0] != '\0') ? base_path : "./");
#endif

	for (int index = 1; index < argc; ++index) {
		state.arguments.emplace_back(argv[index] != NULL ? argv[index] : "");
	}

	state.argv.reserve(state.arguments.size());
	for (std::string& argument : state.arguments) {
		state.argv.push_back(argument.data());
	}
}

void Run_TiberianDawn_Main(CallbackAppState* state)
{
	if (state == NULL) {
		return;
	}

	state->main_started = true;
	state->exit_code = TiberianDawn_Main(static_cast<int>(state->argv.size()), state->argv.data());
	state->main_finished = true;
}

SDL_AppResult App_Result_From_Exit_Code(int exit_code)
{
	return (exit_code == EXIT_SUCCESS) ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
}

} // namespace

SDL_AppResult SDLCALL SDL_AppInit(void** appstate, int argc, char* argv[])
{
	CallbackAppState* state = new CallbackAppState();
#if defined(__EMSCRIPTEN__) && TD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA
	WWFS_SetBaseDirectory("/persistent");
	if (!WWFS_InitializeEmscriptenAssetCache()) {
		delete state;
		return SDL_APP_FAILURE;
	}
#endif
	Build_Startup_Arguments(*state, argc, argv);

	SDL_GameInput_Set_Main_Thread();
	SDL_GameInput_Set_AppCallbacks_Mode(true);

	*appstate = state;

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppIterate(void* appstate)
{
	CallbackAppState* state = static_cast<CallbackAppState*>(appstate);
	if (state == NULL) {
		return SDL_APP_FAILURE;
	}

	if (!state->main_started) {
		Run_TiberianDawn_Main(state);
	}

	if (state->main_finished) {
		return App_Result_From_Exit_Code(state->exit_code);
	}

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDLCALL SDL_AppEvent(void* appstate, SDL_Event*)
{
	CallbackAppState* state = static_cast<CallbackAppState*>(appstate);
	/*
	** The legacy runtime still consumes input from Call_Back()/Message_Loop().
	** SDLINPUT's callback-mode bridge translates pumped SDL events into that
	** path, so the SDL callback entrypoint only needs to report shutdown here.
	*/
	if (state != NULL && state->main_finished) {
		return App_Result_From_Exit_Code(state->exit_code);
	}

	return SDL_APP_CONTINUE;
}

void SDLCALL SDL_AppQuit(void* appstate, SDL_AppResult)
{
	CallbackAppState* state = static_cast<CallbackAppState*>(appstate);
	SDL_GameInput_Set_AppCallbacks_Mode(false);
	delete state;
}
