#include "sdl_draw.h"
#include "FUNCTION.H"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <vector>

namespace {

SDL_Renderer* g_renderer = nullptr;
SDL_Texture* g_texture = nullptr;
int g_texture_width = 0;
int g_texture_height = 0;
WWSurface* g_primary_surface = nullptr;
WWSurface* g_pending_surface = nullptr;
RAWindow* g_pending_window = nullptr;
int g_present_batch_depth = 0;
bool g_present_pending = false;

RAWindow* ensure_window(RAWindow* window, int width, int height)
{
    if (window && window->sdl_window) {
        return window;
    }

    return RA_CreateWindow("Red Alert", width, height, SDL_WINDOW_RESIZABLE);
}

void ensure_renderer(RAWindow* window, int width, int height)
{
    if (!window || !window->sdl_window) {
        return;
    }

    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(window->sdl_window, nullptr);
        if (g_renderer) {
            SDL_SetRenderVSync(g_renderer, 1);
        }
    }

    if (!g_renderer) {
        return;
    }

    if (!g_texture || g_texture_width != width || g_texture_height != height) {
        if (g_texture) {
            SDL_DestroyTexture(g_texture);
        }
        g_texture = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
        if (g_texture) {
            SDL_SetTextureBlendMode(g_texture, SDL_BLENDMODE_NONE);
        }
        g_texture_width = width;
        g_texture_height = height;
    }
}

void present_surface(WWSurface* surface, RAWindow* window)
{
    if (!surface || !surface->IsPrimary()) {
        return;
    }

    ensure_renderer(window, surface->Width(), surface->Height());
    if (!g_renderer || !g_texture) {
        return;
    }

    std::vector<uint32_t> rgba(static_cast<size_t>(surface->Width()) * static_cast<size_t>(surface->Height()));
    const PALETTEENTRY* palette = surface->PaletteEntries();
    uint8_t* pixels = surface->Pixels();

    for (int y = 0; y < surface->Height(); ++y) {
        for (int x = 0; x < surface->Width(); ++x) {
            const uint8_t index = pixels[y * surface->Width() + x];
            const PALETTEENTRY entry = palette ? palette[index] : PALETTEENTRY{index, index, index, 0};
            rgba[y * surface->Width() + x] = (0xffu << 24) | (static_cast<uint32_t>(entry.peRed) << 16)
                | (static_cast<uint32_t>(entry.peGreen) << 8) | static_cast<uint32_t>(entry.peBlue);
        }
    }

    SDL_UpdateTexture(g_texture, nullptr, rgba.data(), surface->Width() * static_cast<int>(sizeof(uint32_t)));
    SDL_FRect source{0.0f, 0.0f, static_cast<float>(surface->Width()), static_cast<float>(surface->Height())};
    SDL_FRect destination{0.0f, 0.0f, static_cast<float>(surface->Width()), static_cast<float>(surface->Height())};
    RAWindow* present_window = window ? window : surface->Window();
    SDL_FRect source_override{};
    if (RA_GetRenderSourceRect(present_window, &source_override) && source_override.w > 0.0f && source_override.h > 0.0f
        && source_override.x >= 0.0f && source_override.y >= 0.0f
        && source_override.x + source_override.w <= static_cast<float>(surface->Width())
        && source_override.y + source_override.h <= static_cast<float>(surface->Height())) {
        source = source_override;
    }
    if (!RA_GetPresentationRect(present_window, &destination)) {
        int output_width = 0;
        int output_height = 0;
        if (SDL_GetRenderOutputSize(g_renderer, &output_width, &output_height) && output_width > 0 && output_height > 0) {
            const float scale = std::min(
                static_cast<float>(output_width) / static_cast<float>(surface->Width()),
                static_cast<float>(output_height) / static_cast<float>(surface->Height()));
            destination.w = std::max(1.0f, static_cast<float>(surface->Width()) * scale);
            destination.h = std::max(1.0f, static_cast<float>(surface->Height()) * scale);
            destination.x = (static_cast<float>(output_width) - destination.w) * 0.5f;
            destination.y = (static_cast<float>(output_height) - destination.h) * 0.5f;
        }
    }
    SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
    SDL_RenderClear(g_renderer);
    SDL_RenderTexture(g_renderer, g_texture, &source, &destination);
    SDL_RenderPresent(g_renderer);
}

void queue_present(WWSurface* surface, RAWindow* window)
{
    if (!surface || !surface->IsPrimary()) {
        return;
    }

    g_pending_surface = surface;
    g_pending_window = window;
    g_present_pending = true;
}

void flush_pending_present()
{
    if (!g_present_pending || g_present_batch_depth != 0) {
        return;
    }

    WWSurface* surface = g_pending_surface ? g_pending_surface : g_primary_surface;
    RAWindow* window = g_pending_window;
    g_pending_surface = nullptr;
    g_pending_window = nullptr;
    g_present_pending = false;

    if (surface) {
        present_surface(surface, window);
    }
}

RECT normalize_rect(const RECT* rect, int width, int height)
{
    RECT result{};
    result.left = rect ? std::max<LONG>(0, rect->left) : 0;
    result.top = rect ? std::max<LONG>(0, rect->top) : 0;
    result.right = rect ? std::min<LONG>(width, rect->right) : width;
    result.bottom = rect ? std::min<LONG>(height, rect->bottom) : height;
    return result;
}

} // namespace

WWPalette::WWPalette() : ref_count_(1)
{
    std::fill(std::begin(entries_), std::end(entries_), PALETTEENTRY{0, 0, 0, 0});
}

HRESULT WWPalette::GetEntries(DWORD start, DWORD count, PALETTEENTRY* entries)
{
    if (!entries || start + count > 256) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    std::copy(entries_ + start, entries_ + start + count, entries);
    return WWDRAW_OK;
}

HRESULT WWPalette::SetEntries(DWORD start, DWORD count, const PALETTEENTRY* entries)
{
    if (!entries || start + count > 256) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    std::copy(entries, entries + count, entries_ + start);
    if (g_primary_surface && g_primary_surface->UsesPalette(this)) {
        queue_present(g_primary_surface, nullptr);
    }
    return WWDRAW_OK;
}

HRESULT WWPalette::Release()
{
    if (--ref_count_ == 0) {
        delete this;
    }
    return WWDRAW_OK;
}

const PALETTEENTRY* WWPalette::Entries() const
{
    return entries_;
}

WWSurface::WWSurface(int width, int height, bool primary, RAWindow* window)
    : width_(width), height_(height), primary_(primary), window_(window), ref_count_(1), palette_(nullptr), pixels_(static_cast<size_t>(width) * static_cast<size_t>(height), 0)
{
    if (primary_) {
        g_primary_surface = this;
    }
}

HRESULT WWSurface::Lock(RECT*, WWLockData* lock_data)
{
    if (!lock_data) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    lock_data->pitch = width_;
    lock_data->pixels = pixels_.data();
    return WWDRAW_OK;
}

HRESULT WWSurface::Unlock(LPVOID)
{
    if (primary_) {
        queue_present(this, window_);
    }
    return WWDRAW_OK;
}

HRESULT WWSurface::Blit(RECT* dest_rect, WWSurface* src_surface, RECT* src_rect, bool use_source_key)
{
    RECT dest = normalize_rect(dest_rect, width_, height_);

    if (!src_surface) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }

    RECT src_bounds = normalize_rect(src_rect, src_surface->Width(), src_surface->Height());
    const int copy_width = std::min<int>(dest.right - dest.left, src_bounds.right - src_bounds.left);
    const int copy_height = std::min<int>(dest.bottom - dest.top, src_bounds.bottom - src_bounds.top);
    if (copy_width <= 0 || copy_height <= 0) {
        return WWDRAW_OK;
    }

    for (int row = 0; row < copy_height; ++row) {
        uint8_t* dst = pixels_.data() + (dest.top + row) * width_ + dest.left;
        uint8_t* src = src_surface->Pixels() + (src_bounds.top + row) * src_surface->Width() + src_bounds.left;
        if (use_source_key) {
            for (int col = 0; col < copy_width; ++col) {
                if (src[col] != 0) {
                    dst[col] = src[col];
                }
            }
        } else {
            std::copy(src, src + copy_width, dst);
        }
    }
    if (primary_) {
        queue_present(this, window_);
    }
    return WWDRAW_OK;
}

HRESULT WWSurface::FillRect(RECT* dest_rect, uint8_t color)
{
    RECT dest = normalize_rect(dest_rect, width_, height_);
    for (LONG y = dest.top; y < dest.bottom; ++y) {
        std::fill_n(pixels_.data() + y * width_ + dest.left, dest.right - dest.left, color);
    }
    if (primary_) {
        queue_present(this, window_);
    }
    return WWDRAW_OK;
}

bool WWSurface::CanBlit() const
{
    return true;
}

bool WWSurface::IsBlitDone() const
{
    return true;
}

HRESULT WWSurface::Restore()
{
    return WWDRAW_OK;
}

HRESULT WWSurface::Release()
{
    if (--ref_count_ == 0) {
        if (g_primary_surface == this) {
            g_primary_surface = nullptr;
        }
        delete this;
    }
    return WWDRAW_OK;
}

HRESULT WWSurface::GetPalette(WWPalette** palette)
{
    if (!palette) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    if (!palette_) {
        *palette = nullptr;
        return WWDRAW_ERROR_NOPALETTEATTACHED;
    }

    auto* copy = new WWPalette();
    if (copy->SetEntries(0, 256, palette_->Entries()) != WWDRAW_OK) {
        copy->Release();
        *palette = nullptr;
        return WWDRAW_ERROR_GENERIC;
    }

    *palette = copy;
    return WWDRAW_OK;
}

HRESULT WWSurface::SetPalette(WWPalette* palette)
{
    palette_ = palette;
    if (primary_) {
        queue_present(this, window_);
    }
    return WWDRAW_OK;
}

void WWSurface::AddAttachedSurface(WWSurface*)
{
}

int WWSurface::Width() const { return width_; }
int WWSurface::Height() const { return height_; }
uint8_t* WWSurface::Pixels() { return pixels_.data(); }
const PALETTEENTRY* WWSurface::PaletteEntries() const { return palette_ ? palette_->Entries() : nullptr; }
bool WWSurface::IsPrimary() const { return primary_; }
bool WWSurface::UsesPalette(const WWPalette* palette) const { return palette_ == palette; }
RAWindow* WWSurface::Window() const { return window_; }
void WWSurface::Present()
{
    queue_present(this, window_);
    flush_pending_present();
}

WWDraw::WWDraw() : width_(640), height_(480), bits_per_pixel_(8), window_(nullptr), ref_count_(1)
{
}

void WWDraw::SetWindow(RAWindow* window)
{
    window_ = window;
}

HRESULT WWDraw::SetDisplayMode(int width, int height, int bits_per_pixel)
{
    width_ = width;
    height_ = height;
    bits_per_pixel_ = bits_per_pixel;
    window_ = ensure_window(window_, width_, height_);
    if (window_ && window_->sdl_window) {
        window_->width = width_;
        window_->height = height_;
        SDL_SetWindowResizable(window_->sdl_window, true);
        SDL_SetWindowSize(window_->sdl_window, width_, height_);
        SDL_ShowWindow(window_->sdl_window);
    }
    return WWDRAW_OK;
}

HRESULT WWDraw::CreatePalette(PALETTEENTRY* entries, WWPalette** palette)
{
    if (!palette) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    auto* created = new WWPalette();
    if (entries) {
        created->SetEntries(0, 256, entries);
    }
    *palette = created;
    return WWDRAW_OK;
}

HRESULT WWDraw::CreatePrimarySurface(WWSurface** surface)
{
    if (!surface) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    *surface = new WWSurface(width_, height_, true, window_);
    queue_present(*surface, window_);
    flush_pending_present();
    return WWDRAW_OK;
}

HRESULT WWDraw::CreateSurface(int width, int height, WWSurface** surface)
{
    if (!surface) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    *surface = new WWSurface(width > 0 ? width : width_, height > 0 ? height : height_, false, window_);
    return WWDRAW_OK;
}

uint32_t WWDraw::GetTotalVideoMemory() const
{
    return 32U * 1024U * 1024U;
}

HRESULT WWDraw::WaitForVerticalBlank()
{
    /*
    ** The SDL renderer already owns display synchronization when vsync is
    ** enabled, and the actual wait happens at SDL_RenderPresent().
    ** Keep this legacy DirectDraw seam as a compatibility no-op instead of
    ** adding an extra fixed 16 ms sleep on top of SDL's presentation path.
    */
    return WWDRAW_OK;
}

HRESULT WWDraw::RestoreDisplayMode()
{
    return WWDRAW_OK;
}

HRESULT WWDraw::Release()
{
    if (--ref_count_ == 0) {
        delete this;
    }
    return WWDRAW_OK;
}

int WWDraw::Width() const { return width_; }
int WWDraw::Height() const { return height_; }
RAWindow* WWDraw::Window() const { return window_; }

HRESULT WWDraw_Create(WWDraw** direct_draw){
    if (!direct_draw) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    *direct_draw = new WWDraw();
    return WWDRAW_OK;
}

void WWDraw_Begin_Present_Batch(void){
    ++g_present_batch_depth;
}

void WWDraw_End_Present_Batch(void){
    if (g_present_batch_depth <= 0) {
        return;
    }

    --g_present_batch_depth;
    flush_pending_present();
}

void WWDraw_Flush_Present(void){
    flush_pending_present();
}

bool WWDraw_Has_Pending_Present(void)
{
    return g_present_pending;
}

void WWDraw_Request_Present(void)
{
    if (g_present_batch_depth != 0 || !g_primary_surface) {
        return;
    }

    queue_present(g_primary_surface, g_primary_surface->Window());
    flush_pending_present();
}
