#ifndef RA_SDL_FS_H
#define RA_SDL_FS_H

#pragma pack(push, 8)
#include <SDL3/SDL.h>
#pragma pack(pop)

#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <unordered_map>

std::string WWFS_NormalizePath(const char* windows_path);
void WWFS_SetBaseDirectory(const char* path);
std::string WWFS_GetBaseDirectoryPath();
void WWFS_SetMainMixOverride(int index);
int WWFS_GetMainMixOverride();
void WWFS_SplitPath(const char* path, char* drive, char* dir, char* fname, char* ext);
bool WWFS_InitializeEmscriptenAssetCache();
bool WWFS_SetCurrentDirectory(const char* path);
char* WWFS_GetCurrentDirectory(char* buffer, int max_length);
bool WWFS_CreateDirectory(const char* path);
bool WWFS_GetPathInfo(const char* path, SDL_PathInfo* info);
bool WWFS_RemovePath(const char* path);
bool WWFS_RenamePath(const char* old_path, const char* new_path);
char** WWFS_GlobDirectory(const char* path, const char* pattern, SDL_GlobFlags flags, int* count);
SDL_IOStream* WWFS_OpenFile(const char* path, const char* mode);
SDL_Storage* WWFS_OpenFileStorage(const char* path);
unsigned WWFS_GetCurrentDriveNumber();
unsigned WWFS_GetDriveCount();
void WWFS_ChangeToDrive(unsigned drive);
void WWFS_MakePath(char* path, const char* drive, const char* dir, const char* fname, const char* ext);

#ifndef _MAX_FNAME
#define _MAX_FNAME 256
#endif

#ifndef _MAX_DRIVE
#define _MAX_DRIVE 3
#endif

#ifndef _MAX_DIR
#define _MAX_DIR 256
#endif

#ifndef _MAX_EXT
#define _MAX_EXT 256
#endif

constexpr int WWFS_OPEN_ACCESS_MASK = 0x0003;
constexpr int WWFS_OPEN_READ_ONLY = 0x0000;
constexpr int WWFS_OPEN_WRITE_ONLY = 0x0001;
constexpr int WWFS_OPEN_READ_WRITE = 0x0002;
constexpr int WWFS_OPEN_APPEND = 0x0008;
constexpr int WWFS_OPEN_CREATE = 0x0100;
constexpr int WWFS_OPEN_TRUNCATE = 0x0200;
constexpr int WWFS_OPEN_BINARY = 0x8000;

inline bool WWFS_GetSeekWhence(int whence, SDL_IOWhence* sdl_whence)
{
    if (sdl_whence == nullptr) {
        return false;
    }

    switch (whence) {
    case SEEK_SET:
        *sdl_whence = SDL_IO_SEEK_SET;
        return true;
    case SEEK_CUR:
        *sdl_whence = SDL_IO_SEEK_CUR;
        return true;
    case SEEK_END:
        *sdl_whence = SDL_IO_SEEK_END;
        return true;
    default:
        return false;
    }
}

inline std::mutex& WWFS_FDMutex()
{
    static std::mutex mutex;
    return mutex;
}

inline std::unordered_map<int, SDL_IOStream*>& WWFS_FDTable()
{
    static std::unordered_map<int, SDL_IOStream*> table;
    return table;
}

inline int& WWFS_NextFD()
{
    static int next_fd = 3;
    return next_fd;
}

inline int WWFS_RegisterFD(SDL_IOStream* stream)
{
    std::scoped_lock lock(WWFS_FDMutex());
    int fd = WWFS_NextFD();
    while (WWFS_FDTable().find(fd) != WWFS_FDTable().end()) {
        ++fd;
    }
    WWFS_FDTable()[fd] = stream;
    WWFS_NextFD() = fd + 1;
    return fd;
}

inline SDL_IOStream* WWFS_FindFD(int fd)
{
    std::scoped_lock lock(WWFS_FDMutex());
    const auto it = WWFS_FDTable().find(fd);
    return (it != WWFS_FDTable().end()) ? it->second : nullptr;
}

inline SDL_IOStream* WWFS_TakeFD(int fd)
{
    std::scoped_lock lock(WWFS_FDMutex());
    const auto it = WWFS_FDTable().find(fd);
    if (it == WWFS_FDTable().end()) {
        return nullptr;
    }
    SDL_IOStream* stream = it->second;
    WWFS_FDTable().erase(it);
    return stream;
}

inline const char* WWFS_ModeFromOpenFlags(int flags, bool create_fallback)
{
    const int access = flags & WWFS_OPEN_ACCESS_MASK;
    const bool append = (flags & WWFS_OPEN_APPEND) != 0;
    const bool trunc = (flags & WWFS_OPEN_TRUNCATE) != 0;
    const bool create = (flags & WWFS_OPEN_CREATE) != 0;

    switch (access) {
    case WWFS_OPEN_READ_ONLY:
        return "rb";

    case WWFS_OPEN_WRITE_ONLY:
        if (append) {
            return "ab";
        }
        return "wb";

    case WWFS_OPEN_READ_WRITE:
        if (append) {
            return "a+b";
        }
        if (trunc) {
            return "w+b";
        }
        if (create) {
            return create_fallback ? "w+b" : "r+b";
        }
        return "r+b";

    default:
        return nullptr;
    }
}

inline SDL_IOStream* WWFS_OpenWithFlags(const char* path, int flags)
{
    for (int attempt = 0; attempt < 2; ++attempt) {
        const char* mode = WWFS_ModeFromOpenFlags(flags, attempt != 0);
        if (!mode) {
            errno = EINVAL;
            return nullptr;
        }

        SDL_IOStream* stream = WWFS_OpenFile(path, mode);
        if (stream) {
            return stream;
        }

        const bool can_retry_create = ((flags & WWFS_OPEN_ACCESS_MASK) == WWFS_OPEN_READ_WRITE)
            && (flags & WWFS_OPEN_CREATE) && !(flags & WWFS_OPEN_TRUNCATE);
        if (!can_retry_create) {
            break;
        }
    }

    errno = ENOENT;
    return nullptr;
}

inline int WWFS_Open(const char* path, int flags, int = 0)
{
    SDL_IOStream* stream = WWFS_OpenWithFlags(path, flags);
    if (!stream) {
        return -1;
    }
    return WWFS_RegisterFD(stream);
}

inline int WWFS_Close(int fd)
{
    SDL_IOStream* stream = WWFS_TakeFD(fd);
    if (!stream) {
        errno = EBADF;
        return -1;
    }
    return SDL_CloseIO(stream) ? 0 : -1;
}

inline int WWFS_Read(int fd, void* buffer, unsigned int count)
{
    SDL_IOStream* stream = WWFS_FindFD(fd);
    if (!stream) {
        errno = EBADF;
        return -1;
    }

    const size_t actual = SDL_ReadIO(stream, buffer, static_cast<size_t>(count));
    if (actual > static_cast<size_t>(INT_MAX)) {
        errno = EOVERFLOW;
        return -1;
    }
    return static_cast<int>(actual);
}

inline int WWFS_Write(int fd, const void* buffer, unsigned int count)
{
    SDL_IOStream* stream = WWFS_FindFD(fd);
    if (!stream) {
        errno = EBADF;
        return -1;
    }

    const size_t actual = SDL_WriteIO(stream, buffer, static_cast<size_t>(count));
    if (actual > static_cast<size_t>(INT_MAX)) {
        errno = EOVERFLOW;
        return -1;
    }
    return static_cast<int>(actual);
}

inline int64_t WWFS_Seek(int fd, int64_t offset, int whence)
{
    SDL_IOStream* stream = WWFS_FindFD(fd);
    if (!stream) {
        errno = EBADF;
        return -1;
    }

    SDL_IOWhence sdl_whence = SDL_IO_SEEK_SET;
    if (!WWFS_GetSeekWhence(whence, &sdl_whence)) {
        errno = EINVAL;
        return -1;
    }

    const Sint64 position = SDL_SeekIO(stream, static_cast<Sint64>(offset), sdl_whence);
    if (position < 0) {
        errno = EOVERFLOW;
        return -1;
    }
    return static_cast<int64_t>(position);
}

inline int WWFS_Unlink(const char* path)
{
    return WWFS_RemovePath(path) ? 0 : -1;
}

inline int WWFS_Remove(const char* path)
{
    return WWFS_Unlink(path);
}

inline int WWFS_ChangeDirectory(const char* path)
{
    return WWFS_SetCurrentDirectory(path) ? 0 : -1;
}


inline int WWFS_MakeDirectory(const char* path)
{
    return WWFS_CreateDirectory(path) ? 0 : -1;
}

inline SDL_IOStream* WWFS_FOpen(const char* path, const char* mode)
{
    return WWFS_OpenFile(path, mode);
}

inline int WWFS_FClose(SDL_IOStream* stream)
{
    if (!stream) {
        errno = EBADF;
        return EOF;
    }
    return SDL_CloseIO(stream) ? 0 : EOF;
}

inline size_t WWFS_FRead(void* buffer, size_t size, size_t count, SDL_IOStream* stream)
{
    if (!stream || size == 0 || count == 0) {
        return 0;
    }
    return SDL_ReadIO(stream, buffer, size * count) / size;
}

inline size_t WWFS_FWrite(const void* buffer, size_t size, size_t count, SDL_IOStream* stream)
{
    if (!stream || size == 0 || count == 0) {
        return 0;
    }
    return SDL_WriteIO(stream, buffer, size * count) / size;
}

inline int WWFS_FSeek(SDL_IOStream* stream, int64_t offset, int whence)
{
    if (!stream) {
        errno = EBADF;
        return -1;
    }

    SDL_IOWhence sdl_whence = SDL_IO_SEEK_SET;
    if (!WWFS_GetSeekWhence(whence, &sdl_whence)) {
        errno = EINVAL;
        return -1;
    }

    return (SDL_SeekIO(stream, static_cast<Sint64>(offset), sdl_whence) < 0) ? -1 : 0;
}

inline int64_t WWFS_FTell(SDL_IOStream* stream)
{
    if (!stream) {
        errno = EBADF;
        return -1;
    }

    const Sint64 position = SDL_TellIO(stream);
    if (position < 0) {
        errno = EOVERFLOW;
        return -1;
    }
    return static_cast<int64_t>(position);
}

inline void WWFS_Rewind(SDL_IOStream* stream)
{
    if (stream) {
        SDL_SeekIO(stream, 0, SDL_IO_SEEK_SET);
    }
}

inline char* WWFS_FGets(char* buffer, int max_length, SDL_IOStream* stream)
{
    if (!buffer || max_length <= 0 || !stream) {
        return nullptr;
    }

    int index = 0;
    while (index < max_length - 1) {
        char value = '\0';
        if (SDL_ReadIO(stream, &value, 1) != 1) {
            break;
        }
        buffer[index++] = value;
        if (value == '\n') {
            break;
        }
    }

    if (index == 0) {
        return nullptr;
    }

    buffer[index] = '\0';
    return buffer;
}

inline int WWFS_FFlush(SDL_IOStream* stream)
{
    if (!stream) {
        errno = EBADF;
        return EOF;
    }
    return SDL_FlushIO(stream) ? 0 : EOF;
}

inline int WWFS_VFPrintf(SDL_IOStream* stream, const char* format, va_list arguments)
{
    if (!stream || !format) {
        errno = EINVAL;
        return -1;
    }

    char* text = nullptr;
    const int length = SDL_vasprintf(&text, format, arguments);
    if (length < 0 || !text) {
        return -1;
    }

    const size_t written = SDL_WriteIO(stream, text, static_cast<size_t>(length));
    SDL_free(text);
    return (written == static_cast<size_t>(length)) ? length : -1;
}

inline int WWFS_FPrintf(SDL_IOStream* stream, const char* format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    const int result = WWFS_VFPrintf(stream, format, arguments);
    va_end(arguments);
    return result;
}

#endif
