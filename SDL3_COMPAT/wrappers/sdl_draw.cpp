#include "sdl_draw.h"
#include "FUNCTION.H"

#include <SDL3/SDL_render.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

namespace {

SDL_Renderer* g_renderer = nullptr;
SDL_Texture* g_texture = nullptr;
SDL_Window* g_renderer_window = nullptr;
int g_texture_width = 0;
int g_texture_height = 0;
bool g_texture_initialized = false;
std::vector<uint32_t> g_rgba_pixels;
WWSurface* g_primary_surface = nullptr;
WWSurface* g_pending_surface = nullptr;
RAWindow* g_pending_window = nullptr;
int g_present_batch_depth = 0;
bool g_present_pending = false;

RECT normalize_rect(const RECT* rect, int width, int height);

RAWindow* ensure_window(RAWindow* window, int width, int height)
{
    if (window && window->sdl_window) {
        return window;
    }

    return RA_CreateWindow("Command & Conquer", width, height, SDL_WINDOW_RESIZABLE);
}

void destroy_renderer_resources()
{
    if (g_texture) {
        SDL_DestroyTexture(g_texture);
        g_texture = nullptr;
    }
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }

    g_renderer_window = nullptr;
    g_texture_width = 0;
    g_texture_height = 0;
    g_texture_initialized = false;
    g_rgba_pixels.clear();
}

void ensure_renderer(RAWindow* window, int width, int height)
{
    if (!window || !window->sdl_window) {
        return;
    }

    if (g_renderer && g_renderer_window != window->sdl_window) {
        destroy_renderer_resources();
    }

    if (!g_renderer) {
        g_renderer = SDL_CreateRenderer(window->sdl_window, nullptr);
        if (g_renderer) {
            SDL_SetRenderVSync(g_renderer, 1);
            g_renderer_window = window->sdl_window;
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
        g_texture_initialized = false;
    }

    const size_t pixel_count = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (g_rgba_pixels.size() != pixel_count) {
        g_rgba_pixels.assign(pixel_count, 0);
        g_texture_initialized = false;
    }
}

void build_palette_lookup(const PALETTEENTRY* palette, std::array<uint32_t, 256>& lookup)
{
    for (size_t index = 0; index < lookup.size(); ++index) {
        const uint8_t palette_index = static_cast<uint8_t>(index);
        const PALETTEENTRY entry = palette ? palette[palette_index] : PALETTEENTRY{palette_index, palette_index, palette_index, 0};
        lookup[index] = (0xffu << 24) | (static_cast<uint32_t>(entry.peRed) << 16)
            | (static_cast<uint32_t>(entry.peGreen) << 8) | static_cast<uint32_t>(entry.peBlue);
    }
}

RECT merge_rects(const RECT& lhs, const RECT& rhs)
{
    RECT merged{};
    merged.left = std::min(lhs.left, rhs.left);
    merged.top = std::min(lhs.top, rhs.top);
    merged.right = std::max(lhs.right, rhs.right);
    merged.bottom = std::max(lhs.bottom, rhs.bottom);
    return merged;
}

bool is_rect_empty(const RECT& rect)
{
    return rect.right <= rect.left || rect.bottom <= rect.top;
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

    RECT dirty{};
    const bool has_dirty_rect = surface->ConsumeDirtyRect(&dirty);
    if (has_dirty_rect || !g_texture_initialized) {
        const RECT upload_rect = has_dirty_rect ? normalize_rect(&dirty, surface->Width(), surface->Height())
                                                : normalize_rect(nullptr, surface->Width(), surface->Height());
        if (!is_rect_empty(upload_rect)) {
            std::array<uint32_t, 256> palette_lookup{};
            build_palette_lookup(surface->PaletteEntries(), palette_lookup);

            const int surface_width = surface->Width();
            const uint8_t* pixels = surface->Pixels();
            for (LONG y = upload_rect.top; y < upload_rect.bottom; ++y) {
                const uint8_t* src = pixels + static_cast<size_t>(y) * static_cast<size_t>(surface_width) + upload_rect.left;
                uint32_t* dst = g_rgba_pixels.data() + static_cast<size_t>(y) * static_cast<size_t>(surface_width) + upload_rect.left;
                for (LONG x = 0; x < upload_rect.right - upload_rect.left; ++x) {
                    dst[x] = palette_lookup[src[x]];
                }
            }

            SDL_Rect texture_rect{
                upload_rect.left,
                upload_rect.top,
                upload_rect.right - upload_rect.left,
                upload_rect.bottom - upload_rect.top,
            };
            SDL_UpdateTexture(
                g_texture,
                &texture_rect,
                g_rgba_pixels.data() + static_cast<size_t>(upload_rect.top) * static_cast<size_t>(surface_width) + upload_rect.left,
                surface_width * static_cast<int>(sizeof(uint32_t)));
            g_texture_initialized = true;
        }
    }
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
        g_primary_surface->MarkDirty();
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
    : width_(width)
    , height_(height)
    , primary_(primary)
    , window_(window)
    , ref_count_(1)
    , palette_(nullptr)
    , pixels_(static_cast<size_t>(width) * static_cast<size_t>(height), 0)
    , has_dirty_rect_(false)
    , dirty_rect_{}
    , lock_rect_valid_(false)
    , lock_rect_{}
{
    if (primary_) {
        g_primary_surface = this;
        MarkDirty();
    }
}

HRESULT WWSurface::Lock(RECT* rect, WWLockData* lock_data)
{
    if (!lock_data) {
        return WWDRAW_ERROR_INVALIDPARAMS;
    }
    lock_rect_valid_ = rect != nullptr;
    lock_rect_ = normalize_rect(rect, width_, height_);
    lock_data->pitch = width_;
    lock_data->pixels = pixels_.data();
    return WWDRAW_OK;
}

HRESULT WWSurface::Unlock(LPVOID)
{
    if (primary_) {
        MarkDirty(lock_rect_valid_ ? &lock_rect_ : nullptr);
        queue_present(this, window_);
    }
    lock_rect_valid_ = false;
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

    const bool overlapping_self_blit = src_surface == this
        && dest.left < src_bounds.left + copy_width
        && src_bounds.left < dest.left + copy_width
        && dest.top < src_bounds.top + copy_height
        && src_bounds.top < dest.top + copy_height;
    std::vector<uint8_t> scratch;
    if (use_source_key && overlapping_self_blit) {
        scratch.resize(static_cast<size_t>(copy_width) * static_cast<size_t>(copy_height));
        for (int row = 0; row < copy_height; ++row) {
            const uint8_t* src = src_surface->Pixels() + (src_bounds.top + row) * src_surface->Width() + src_bounds.left;
            std::memcpy(scratch.data() + static_cast<size_t>(row) * static_cast<size_t>(copy_width), src, static_cast<size_t>(copy_width));
        }
    }

    int row = 0;
    int row_end = copy_height;
    int row_step = 1;
    if (!use_source_key && overlapping_self_blit && dest.top > src_bounds.top) {
        row = copy_height - 1;
        row_end = -1;
        row_step = -1;
    }

    for (; row != row_end; row += row_step) {
        uint8_t* dst = pixels_.data() + (dest.top + row) * width_ + dest.left;
        const uint8_t* src = (use_source_key && overlapping_self_blit)
            ? scratch.data() + static_cast<size_t>(row) * static_cast<size_t>(copy_width)
            : src_surface->Pixels() + (src_bounds.top + row) * src_surface->Width() + src_bounds.left;
        if (use_source_key) {
            for (int col = 0; col < copy_width; ++col) {
                if (src[col] != 0) {
                    dst[col] = src[col];
                }
            }
        } else {
            if (overlapping_self_blit) {
                std::memmove(dst, src, static_cast<size_t>(copy_width));
            } else {
                std::memcpy(dst, src, static_cast<size_t>(copy_width));
            }
        }
    }
    if (primary_) {
        RECT dirty{dest.left, dest.top, dest.left + copy_width, dest.top + copy_height};
        MarkDirty(&dirty);
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
        MarkDirty(&dest);
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
    if (primary_) {
        MarkDirty();
    }
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
        MarkDirty();
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
void WWSurface::MarkDirty(const RECT* rect)
{
    const RECT dirty = normalize_rect(rect, width_, height_);
    if (is_rect_empty(dirty)) {
        return;
    }

    if (!has_dirty_rect_) {
        dirty_rect_ = dirty;
        has_dirty_rect_ = true;
        return;
    }

    dirty_rect_ = merge_rects(dirty_rect_, dirty);
}

bool WWSurface::ConsumeDirtyRect(RECT* rect)
{
    if (!has_dirty_rect_) {
        return false;
    }

    if (rect) {
        *rect = dirty_rect_;
    }
    has_dirty_rect_ = false;
    return true;
}

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
        int display_width = width_;
        int display_height = height_;
        window_->width = width_;
        window_->height = height_;
        RA_GetDefaultWindowSizeForRenderSize(width_, height_, &display_width, &display_height);
        SDL_SetWindowResizable(window_->sdl_window, true);
        SDL_SetWindowSize(window_->sdl_window, display_width, display_height);
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
