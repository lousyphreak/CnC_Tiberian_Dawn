#ifndef RA_MESSAGEBOX_H
#define RA_MESSAGEBOX_H

#include "win32_compat.h"

#include <SDL3/SDL_messagebox.h>

inline void RA_ShowMessageBox(const char* text, const char* caption, Uint32 flags = SDL_MESSAGEBOX_INFORMATION)
{
    SDL_ShowSimpleMessageBox(flags, caption ? caption : "Red Alert", text ? text : "", nullptr);
}

inline bool RA_ShowConfirmationMessageBox(const char* text, const char* caption, Uint32 flags = SDL_MESSAGEBOX_INFORMATION)
{
    constexpr int kYesButtonId = 1;
    constexpr int kNoButtonId = 0;

    const SDL_MessageBoxButtonData buttons[] = {
        {SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, kYesButtonId, "Yes"},
        {SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, kNoButtonId, "No"},
    };
    const SDL_MessageBoxData data = {
        flags,
        nullptr,
        caption ? caption : "Red Alert",
        text ? text : "",
        2,
        buttons,
        nullptr,
    };

    int button_id = kNoButtonId;
    if (SDL_ShowMessageBox(&data, &button_id) != 0) {
        return false;
    }

    return button_id == kYesButtonId;
}

#endif
