#ifndef SDLINPUT_H
#define SDLINPUT_H
#include <stdint.h>

#include "win32_compat.h"
#include <SDL3/SDL.h>
#include <cstdint>

void SDL_GameInput_Set_Main_Thread(void);
void SDL_GameInput_Set_AppCallbacks_Mode(bool enabled);
bool SDL_GameInput_Is_AppCallbacks_Mode(void);
void SDL_GameInput_SetTextInputActive(bool enabled);
void SDL_GameInput_RequestRedraw(void);
void SDL_GameInput_Handle_Event(const SDL_Event& event);
void SDL_GameInput_Delay(uint32_t milliseconds);
void SDL_GameInput_Pump(void);
int32_t SDL_GameInput_Wait(void);
void SDL_GameInput_Maybe_Pump(void);
void SDL_GameInput_Handle_Key(SDL_Scancode scancode, SDL_Keycode keycode, SDL_Keymod mod_state, bool repeat, bool release);
void SDL_GameInput_Handle_Mouse_Motion(int x, int y, int xrel, int yrel, uint32_t button_state);
void SDL_GameInput_Handle_Mouse_Button(int button, int x, int y, bool release);
void* SDL_GameInput_SetCursor(void* cursor);
int SDL_GameInput_ShowCursor(int32_t show);
void SDL_GameInput_SetMissionMouseCapture(bool enabled);
void SDL_GameInput_PushMouseCaptureSuppression(void);
void SDL_GameInput_PopMouseCaptureSuppression(void);
int32_t SDL_GameInput_ClipCursor(const RECT* rect);
int32_t SDL_GameInput_GetCursorPos(POINT* point);
int SDL_GameInput_GetGlobalMouseX(void);
int SDL_GameInput_GetGlobalMouseY(void);
int32_t SDL_GameInput_GetViewportRect(RECT* rect);
void SDL_GameInput_SetCursorClip(const RECT* rect);
bool SDL_GameInput_IsModifierPressed(SDL_Keymod mod_mask);
bool SDL_GameInput_IsModifierToggled(SDL_Keymod mod_mask);
bool SDL_GameInput_IsScancodePressed(SDL_Scancode scancode);
bool SDL_GameInput_IsMouseButtonPressed(uint8_t button);
bool SDL_GameInput_IsLegacyKeyPressed(uint16_t key);
char SDL_GameInput_KeyToASCII(uint16_t key);
int16_t SDL_GameInput_AsciiToLegacyKey(uint8_t ch);

#endif
