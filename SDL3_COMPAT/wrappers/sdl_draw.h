#ifndef RA_SDL_DRAW_H
#define RA_SDL_DRAW_H

#include "win32_compat.h"

#include <cstdint>
#include <vector>

#define WWDRAW_OK 0
#define E_NOTIMPL ((HRESULT)-1)
#define WWDRAW_ERROR_SURFACELOST ((HRESULT)-2000)
#define WWDRAW_ERROR_GENERIC ((HRESULT)-2001)
#define WWDRAW_ERROR_INVALIDPARAMS ((HRESULT)-2002)
#define WWDRAW_ERROR_NOPALETTEATTACHED ((HRESULT)-2003)

struct WWLockData {
    LONG pitch;
    LPVOID pixels;
};

class WWPalette {
public:
    WWPalette();
    HRESULT GetEntries(DWORD start, DWORD count, PALETTEENTRY* entries);
    HRESULT SetEntries(DWORD start, DWORD count, const PALETTEENTRY* entries);
    HRESULT Release();
    const PALETTEENTRY* Entries() const;
private:
    PALETTEENTRY entries_[256];
    int ref_count_;
};

class WWSurface {
public:
    WWSurface(int width, int height, bool primary, RAWindow* window);
    HRESULT Lock(RECT* rect, WWLockData* lock_data);
    HRESULT Unlock(LPVOID data);
    HRESULT Blit(RECT* dest_rect, WWSurface* src_surface, RECT* src_rect, bool use_source_key);
    HRESULT FillRect(RECT* dest_rect, uint8_t color);
    bool CanBlit() const;
    bool IsBlitDone() const;
    HRESULT Restore();
    HRESULT Release();
    HRESULT GetPalette(WWPalette** palette);
    HRESULT SetPalette(WWPalette* palette);
    void AddAttachedSurface(WWSurface* surface);

    int Width() const;
    int Height() const;
    uint8_t* Pixels();
    const PALETTEENTRY* PaletteEntries() const;
    bool IsPrimary() const;
    bool UsesPalette(const WWPalette* palette) const;
    RAWindow* Window() const;
    void Present();
private:
    int width_;
    int height_;
    bool primary_;
    RAWindow* window_;
    int ref_count_;
    WWPalette* palette_;
    std::vector<uint8_t> pixels_;
};

class WWDraw {
public:
    WWDraw();
    void SetWindow(RAWindow* window);
    HRESULT SetDisplayMode(int width, int height, int bits_per_pixel);
    HRESULT CreatePalette(PALETTEENTRY* entries, WWPalette** palette);
    HRESULT CreatePrimarySurface(WWSurface** surface);
    HRESULT CreateSurface(int width, int height, WWSurface** surface);
    uint32_t GetTotalVideoMemory() const;
    HRESULT WaitForVerticalBlank();
    HRESULT RestoreDisplayMode();
    HRESULT Release();

    int Width() const;
    int Height() const;
    RAWindow* Window() const;
private:
    int width_;
    int height_;
    int bits_per_pixel_;
    RAWindow* window_;
    int ref_count_;
};

HRESULT WWDraw_Create(WWDraw** direct_draw);
void WWDraw_Begin_Present_Batch(void);
void WWDraw_End_Present_Batch(void);
void WWDraw_Flush_Present(void);
bool WWDraw_Has_Pending_Present(void);
void WWDraw_Request_Present(void);
#endif
