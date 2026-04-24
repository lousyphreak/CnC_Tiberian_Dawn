#include "win32_compat.h"
#include "sdl_fs.h"
#include "SDLINPUT.H"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cmath>
#include <ctime>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace {

constexpr DWORD kErrorSuccess = 0;
constexpr DWORD kErrorFileNotFound = 2;
constexpr DWORD kWaitObject0 = 0x00000000UL;
constexpr DWORD kWaitTimeout = 0x00000102UL;
constexpr DWORD kWaitFailed = 0xFFFFFFFFU;
constexpr DWORD kInfiniteWait = 0xFFFFFFFFU;
constexpr DWORD kGenericRead = 0x80000000U;
constexpr DWORD kGenericWrite = 0x40000000U;
constexpr DWORD kCreateAlways = 2U;
constexpr DWORD kOpenExisting = 3U;
constexpr DWORD kOpenAlways = 4U;
constexpr DWORD kFileCurrent = 1U;
constexpr DWORD kFileEnd = 2U;
constexpr UINT kDriveNoRootDir = 1U;
constexpr UINT kDriveFixed = 3U;

HANDLE invalid_handle_value()
{
    return reinterpret_cast<HANDLE>(static_cast<intptr_t>(-1));
}
enum class HandleKind {
    None,
    File,
    Event,
};

struct HandleBase {
    explicit HandleBase(HandleKind kind) : kind(kind) {}
    virtual ~HandleBase() = default;
    HandleKind kind;
};

struct FileHandle final : HandleBase {
    FileHandle(SDL_IOStream* io_stream, std::string file_path)
        : HandleBase(HandleKind::File), io(io_stream), path(std::move(file_path)) {}
    SDL_IOStream* io;
    std::string path;
};

struct EventHandle final : HandleBase {
    EventHandle(bool manual_reset, bool initial_state) : HandleBase(HandleKind::Event), manual(manual_reset), signaled(initial_state) {}
    std::mutex mutex;
    std::condition_variable condition;
    bool manual;
    bool signaled;
};

std::mutex g_last_error_mutex;
DWORD g_last_error = kErrorSuccess;
std::atomic<UINT> g_error_mode{0};

} // end anonymous namespace

namespace {

constexpr int kStretchedGameWidth = 640;
constexpr int kStretchedGameHeight = 400;
constexpr int kDisplayHeightForStretchedGame = 480;

bool uses_stretched_640x400_presentation(RAWindow* window)
{
    return window != nullptr && window->width == kStretchedGameWidth && window->height == kDisplayHeightForStretchedGame;
}

bool get_render_source_rect(RAWindow* window, SDL_FRect* rect)
{
    if (!window || !rect || window->width <= 0 || window->height <= 0) {
        return false;
    }

    rect->x = 0.0f;
    rect->y = 0.0f;
    rect->w = static_cast<float>(window->width);
    rect->h = static_cast<float>(window->height);

    if (uses_stretched_640x400_presentation(window)) {
        rect->h = static_cast<float>(kStretchedGameHeight);
        rect->y = (static_cast<float>(window->height) - rect->h) * 0.5f;
    }

    return true;
}

void set_last_error(DWORD value)
{
    std::scoped_lock lock(g_last_error_mutex);
    g_last_error = value;
}

std::string create_file_mode(DWORD desired_access, DWORD creation_disposition)
{
    const bool can_read = (desired_access & kGenericRead) != 0;
    const bool can_write = (desired_access & kGenericWrite) != 0;

    switch (creation_disposition) {
        case kCreateAlways:
            return can_read ? "w+b" : "wb";
        case kOpenAlways:
            return can_write ? (can_read ? "a+b" : "ab") : "rb";
        case kOpenExisting:
        default:
            if (can_read && can_write) return "r+b";
            if (can_write) return "wb";
            return "rb";
    }
}

void set_window_icon(SDL_Window* sdl_window)
{
    if (!sdl_window) {
        return;
    }

    const char* base_path = SDL_GetBasePath();
    if (!base_path || base_path[0] == '\0') {
        return;
    }

    std::string icon_path = base_path;
    if (icon_path.back() != '/' && icon_path.back() != '\\') {
        icon_path.push_back('/');
    }
    icon_path += "resources/icons/redalert-window-icon.png";

    SDL_Surface* icon_surface = SDL_LoadPNG(icon_path.c_str());
    if (!icon_surface) {
        return;
    }

    SDL_SetWindowIcon(sdl_window, icon_surface);
    SDL_DestroySurface(icon_surface);
}

} // namespace

RAWindow* RA_CreateWindow(const char* title, int width, int height, SDL_WindowFlags flags)
{
    auto* window = new RAWindow{};
    window->title = title ? title : "Red Alert";
    window->width = width > 0 ? width : 640;
    window->height = height > 0 ? height : 480;
    window->sdl_window = SDL_CreateWindow(window->title.c_str(), window->width, window->height, flags);
    if (!window->sdl_window) {
        delete window;
        return nullptr;
    }
    set_window_icon(window->sdl_window);
    return window;
}

void RA_DestroyWindow(RAWindow* window)
{
    if (!window) {
        return;
    }
    if (window->sdl_window) {
        SDL_DestroyWindow(window->sdl_window);
        window->sdl_window = nullptr;
    }
    delete window;
}

bool RA_GetPresentationRect(RAWindow* window, SDL_FRect* rect)
{
    if (!window || !window->sdl_window || !rect || window->width <= 0 || window->height <= 0) {
        return false;
    }

    int window_width = 0;
    int window_height = 0;
    if (!SDL_GetWindowSize(window->sdl_window, &window_width, &window_height) || window_width <= 0 || window_height <= 0) {
        return false;
    }

    const float logical_width = static_cast<float>(window->width);
    const float logical_height = static_cast<float>(window->height);
    const float scale = std::min(static_cast<float>(window_width) / logical_width, static_cast<float>(window_height) / logical_height);
    rect->w = std::max(1.0f, logical_width * scale);
    rect->h = std::max(1.0f, logical_height * scale);
    rect->x = (static_cast<float>(window_width) - rect->w) * 0.5f;
    rect->y = (static_cast<float>(window_height) - rect->h) * 0.5f;
    return true;
}

bool RA_GetRenderSourceRect(RAWindow* window, SDL_FRect* rect)
{
    return get_render_source_rect(window, rect);
}

bool RA_WindowToGamePoint(RAWindow* window, float window_x, float window_y, int* game_x, int* game_y)
{
    if (!game_x || !game_y) {
        return false;
    }

    SDL_FRect presentation{};
    if (!RA_GetPresentationRect(window, &presentation) || presentation.w <= 0.0f || presentation.h <= 0.0f || !window
        || window->width <= 0 || window->height <= 0) {
        *game_x = static_cast<int>(window_x);
        *game_y = static_cast<int>(window_y);
        return false;
    }

    SDL_FRect source{};
    if (!get_render_source_rect(window, &source) || source.w <= 0.0f || source.h <= 0.0f) {
        *game_x = static_cast<int>(window_x);
        *game_y = static_cast<int>(window_y);
        return false;
    }

    const float normalized_x = (window_x - presentation.x) / presentation.w;
    const float normalized_y = (window_y - presentation.y) / presentation.h;
    const int source_left = static_cast<int>(source.x);
    const int source_top = static_cast<int>(source.y);
    const int source_right = source_left + static_cast<int>(source.w) - 1;
    const int source_bottom = source_top + static_cast<int>(source.h) - 1;
    int mapped_x = static_cast<int>(source.x + normalized_x * source.w);
    int mapped_y = static_cast<int>(source.y + normalized_y * source.h);
    mapped_x = std::clamp(mapped_x, source_left, source_right);
    mapped_y = std::clamp(mapped_y, source_top, source_bottom);
    *game_x = mapped_x;
    *game_y = mapped_y;
    return true;
}

bool RA_GameRectToWindowRect(RAWindow* window, const RECT* game_rect, SDL_Rect* window_rect)
{
    if (!game_rect || !window_rect || !window || window->width <= 0 || window->height <= 0) {
        return false;
    }

    SDL_FRect presentation{};
    if (!RA_GetPresentationRect(window, &presentation) || presentation.w <= 0.0f || presentation.h <= 0.0f) {
        return false;
    }

    SDL_FRect source{};
    if (!get_render_source_rect(window, &source) || source.w <= 0.0f || source.h <= 0.0f) {
        return false;
    }

    const float scale_x = presentation.w / source.w;
    const float scale_y = presentation.h / source.h;
    const float left = presentation.x + (static_cast<float>(game_rect->left) - source.x) * scale_x;
    const float top = presentation.y + (static_cast<float>(game_rect->top) - source.y) * scale_y;
    const float right = presentation.x + (static_cast<float>(game_rect->right) - source.x) * scale_x;
    const float bottom = presentation.y + (static_cast<float>(game_rect->bottom) - source.y) * scale_y;
    window_rect->x = static_cast<int>(std::floor(left));
    window_rect->y = static_cast<int>(std::floor(top));
    window_rect->w = std::max(0, static_cast<int>(std::ceil(right)) - window_rect->x);
    window_rect->h = std::max(0, static_cast<int>(std::ceil(bottom)) - window_rect->y);
    return true;
}


DWORD GetLastError(void)
{
    std::scoped_lock lock(g_last_error_mutex);
    return g_last_error;
}

UINT SetErrorMode(UINT mode)
{
    return g_error_mode.exchange(mode);
}

DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds)
{
    if (!handle) return kWaitFailed;
    auto* base = static_cast<HandleBase*>(handle);
    switch (base->kind) {
        case HandleKind::Event: {
            auto* event = static_cast<EventHandle*>(base);
            std::unique_lock lock(event->mutex);
            if (!event->signaled) {
                if (milliseconds == kInfiniteWait) {
                    event->condition.wait(lock, [event]() { return event->signaled; });
                } else if (!event->condition.wait_for(lock, std::chrono::milliseconds(milliseconds), [event]() { return event->signaled; })) {
                    return kWaitTimeout;
                }
            }
            if (!event->manual) {
                event->signaled = false;
            }
            return kWaitObject0;
        }
        default:
            return kWaitFailed;
    }
}

BOOL CloseHandle(HANDLE handle)
{
    if (!handle || handle == invalid_handle_value()) return 1;
    auto* base = static_cast<HandleBase*>(handle);
    if (base->kind == HandleKind::File) {
        auto* file = static_cast<FileHandle*>(base);
        if (file->io) {
            SDL_CloseIO(file->io);
        }
    }
    delete base;
    return 1;
}

HANDLE CreateEvent(LPVOID, BOOL manual_reset, BOOL initial_state, LPCSTR name)
{
    return new EventHandle(manual_reset != 0, initial_state != 0);
}

BOOL SetEvent(HANDLE handle)
{
    if (!handle) return 0;
    auto* event = static_cast<EventHandle*>(handle);
    std::scoped_lock lock(event->mutex);
    event->signaled = true;
    event->condition.notify_all();
    return 1;
}

BOOL ResetEvent(HANDLE handle)
{
    if (!handle) return 0;
    auto* event = static_cast<EventHandle*>(handle);
    std::scoped_lock lock(event->mutex);
    event->signaled = false;
    return 1;
}

HANDLE CreateFile(LPCSTR file_name, DWORD desired_access, DWORD, LPVOID, DWORD creation_disposition, DWORD, HANDLE)
{
    auto mode = create_file_mode(desired_access, creation_disposition);
    const std::string normalized_path = WWFS_NormalizePath(file_name);
    SDL_IOStream* io = WWFS_OpenFile(normalized_path.c_str(), mode.c_str());
    if (!io) {
        set_last_error(kErrorFileNotFound);
        return invalid_handle_value();
    }
    return new FileHandle(io, normalized_path);
}

BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD number_of_bytes_to_read, LPDWORD number_of_bytes_read, LPVOID)
{
    if (number_of_bytes_read) *number_of_bytes_read = 0;
    if (!handle || handle == invalid_handle_value()) return 0;
    auto* file = static_cast<FileHandle*>(handle);
    const size_t read = SDL_ReadIO(file->io, buffer, number_of_bytes_to_read);
    if (number_of_bytes_read) {
        *number_of_bytes_read = static_cast<DWORD>(read);
    }
    return 1;
}

BOOL WriteFile(HANDLE handle, LPCVOID buffer, DWORD number_of_bytes_to_write, LPDWORD number_of_bytes_written, LPVOID)
{
    if (number_of_bytes_written) *number_of_bytes_written = 0;
    if (!handle || handle == invalid_handle_value()) return 0;
    auto* file = static_cast<FileHandle*>(handle);
    const size_t written = SDL_WriteIO(file->io, buffer, number_of_bytes_to_write);
    if (number_of_bytes_written) {
        *number_of_bytes_written = static_cast<DWORD>(written);
    }
    return written == number_of_bytes_to_write;
}

DWORD SetFilePointer(HANDLE handle, LONG distance_to_move, LONG*, DWORD move_method)
{
    if (!handle || handle == invalid_handle_value()) return 0xffffffffu;
    auto* file = static_cast<FileHandle*>(handle);
    SDL_IOWhence whence = SDL_IO_SEEK_SET;
    if (move_method == kFileCurrent) whence = SDL_IO_SEEK_CUR;
    if (move_method == kFileEnd) whence = SDL_IO_SEEK_END;
    Sint64 result = SDL_SeekIO(file->io, distance_to_move, whence);
    return result < 0 ? 0xffffffffu : static_cast<DWORD>(result);
}

UINT GetDriveType(LPCSTR root_path_name)
{
    if (!root_path_name || !*root_path_name) {
        return kDriveNoRootDir;
    }

    const std::string normalized_path = WWFS_NormalizePath(root_path_name);
    if (!WWFS_GetPathInfo(normalized_path.c_str(), nullptr)) {
        return kDriveNoRootDir;
    }

    return kDriveFixed;
}

BOOL GetVolumeInformation(LPCSTR root_path_name, LPSTR volume_name_buffer, DWORD volume_name_size, DWORD* volume_serial_number,
    DWORD* maximum_component_length, DWORD* file_system_flags, LPSTR file_system_name_buffer, DWORD file_system_name_size)
{
    if (!root_path_name || !*root_path_name) {
        return 0;
    }

    std::string normalized_path = WWFS_NormalizePath(root_path_name);
    if (!WWFS_GetPathInfo(normalized_path.c_str(), nullptr)) {
        return 0;
    }

    // Extract the last path component as the "volume name"
    std::string volume_name;
    {
        size_t pos = normalized_path.find_last_of('/');
        if (pos != std::string::npos && pos + 1 < normalized_path.size()) {
            volume_name = normalized_path.substr(pos + 1);
        }
    }
    if (volume_name.empty()) {
        volume_name = normalized_path;
    }

    if (volume_name_buffer && volume_name_size > 0) {
        std::snprintf(volume_name_buffer, volume_name_size, "%s", volume_name.c_str());
    }
    if (volume_serial_number) {
        *volume_serial_number = 0;
    }
    if (maximum_component_length) {
        *maximum_component_length = 255;
    }
    if (file_system_flags) {
        *file_system_flags = 0;
    }
    if (file_system_name_buffer && file_system_name_size > 0) {
        std::snprintf(file_system_name_buffer, file_system_name_size, "%s", "SDLFS");
    }

    return 1;
}
