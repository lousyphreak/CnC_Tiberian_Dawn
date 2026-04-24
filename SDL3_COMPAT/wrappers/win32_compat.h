#pragma once

#pragma pack(push, 8)
#include <SDL3/SDL.h>
#pragma pack(pop)

#if defined(_WIN32)
#include <io.h>
#else
#include <unistd.h>
#endif

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

using BYTE = uint8_t;
using WORD = uint16_t;
using DWORD = uint32_t;
using LONG = int32_t;
using ULONG = uint32_t;
using UINT = uint32_t;
using INT = int32_t;
using BOOL = int32_t;
using HRESULT = LONG;
using LPARAM = intptr_t;
using WPARAM = uintptr_t;
using LRESULT = intptr_t;
using MCIDEVICEID = UINT;
using ATOM = WORD;
using HANDLE = void*;
using HGDIOBJ = void*;
using LPVOID = void*;
using LPCVOID = const void*;
using PBYTE = BYTE*;
using LPBYTE = BYTE*;
using LPCBYTE = const BYTE*;
using LPSTR = char*;
using LPCSTR = const char*;
using LPDWORD = DWORD*;
using LPWORD = WORD*;
using LPLONG = LONG*;
using LPBOOL = BOOL*;
using CHAR = char;
using UCHAR = uint8_t;
using TCHAR = char;
using LPTSTR = char*;
using LPCTSTR = const char*;
using SHORT = int16_t;
using USHORT = uint16_t;
using VOID = void;
using INT_PTR = intptr_t;

struct RAWindow;
using HWND = RAWindow*;
using DLGPROC = INT_PTR (*)(HWND, UINT, WPARAM, LPARAM);

using DWORD_PTR = uintptr_t;

#ifndef __cdecl
#define __cdecl
#endif

#ifndef WINAPI
#define WINAPI
#endif

#ifndef CALLBACK
#define CALLBACK
#endif

#ifndef APIENTRY
#define APIENTRY
#endif

#ifndef FAR
#define FAR
#endif

#ifndef PASCAL
#define PASCAL
#endif

#ifndef _export
#define _export
#endif

constexpr UINT WM_USER = 0x0400U;

struct POINT {
    LONG x;
    LONG y;
};

struct RECT {
    LONG left;
    LONG top;
    LONG right;
    LONG bottom;
};

struct FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

struct MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORD dwTotalPhys;
    DWORD dwAvailPhys;
    DWORD dwTotalPageFile;
    DWORD dwAvailPageFile;
    DWORD dwTotalVirtual;
    DWORD dwAvailVirtual;
};

#pragma pack(push, 1)
struct BITMAPFILEHEADER {
    WORD bfType;
    DWORD bfSize;
    WORD bfReserved1;
    WORD bfReserved2;
    DWORD bfOffBits;
};
#pragma pack(pop)

struct RGBQUAD {
    BYTE rgbBlue;
    BYTE rgbGreen;
    BYTE rgbRed;
    BYTE rgbReserved;
};

struct BITMAPINFOHEADER {
    DWORD biSize;
    LONG biWidth;
    LONG biHeight;
    WORD biPlanes;
    WORD biBitCount;
    DWORD biCompression;
    DWORD biSizeImage;
    LONG biXPelsPerMeter;
    LONG biYPelsPerMeter;
    DWORD biClrUsed;
    DWORD biClrImportant;
};

using LPBITMAPINFOHEADER = BITMAPINFOHEADER*;

struct PALETTEENTRY {
    BYTE peRed;
    BYTE peGreen;
    BYTE peBlue;
    BYTE peFlags;
};

struct RAWindow {
    SDL_Window* sdl_window;
    std::string title;
    int width;
    int height;
};

RAWindow* RA_CreateWindow(const char* title, int width, int height, SDL_WindowFlags flags);
void RA_DestroyWindow(RAWindow* window);
bool RA_GetPresentationRect(RAWindow* window, SDL_FRect* rect);
bool RA_GetRenderSourceRect(RAWindow* window, SDL_FRect* rect);
bool RA_WindowToGamePoint(RAWindow* window, float window_x, float window_y, int* game_x, int* game_y);
bool RA_GameRectToWindowRect(RAWindow* window, const RECT* game_rect, SDL_Rect* window_rect);


DWORD GetLastError(void);
UINT SetErrorMode(UINT mode);
DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds);
BOOL CloseHandle(HANDLE handle);
HANDLE CreateEvent(LPVOID attributes, BOOL manual_reset, BOOL initial_state, LPCSTR name);
BOOL SetEvent(HANDLE handle);
BOOL ResetEvent(HANDLE handle);
HANDLE CreateFile(LPCSTR file_name, DWORD desired_access, DWORD share_mode, LPVOID security_attributes,
    DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_file);
BOOL ReadFile(HANDLE handle, LPVOID buffer, DWORD number_of_bytes_to_read, LPDWORD number_of_bytes_read, LPVOID overlapped);
BOOL WriteFile(HANDLE handle, LPCVOID buffer, DWORD number_of_bytes_to_write, LPDWORD number_of_bytes_written, LPVOID overlapped);
DWORD SetFilePointer(HANDLE handle, LONG distance_to_move, LONG* distance_to_move_high, DWORD move_method);
UINT GetDriveType(LPCSTR root_path_name);
BOOL GetVolumeInformation(LPCSTR root_path_name, LPSTR volume_name_buffer, DWORD volume_name_size, DWORD* volume_serial_number,
    DWORD* maximum_component_length, DWORD* file_system_flags, LPSTR file_system_name_buffer, DWORD file_system_name_size);
void GlobalMemoryStatus(MEMORYSTATUS* memory_status);
int stricmp(const char* lhs, const char* rhs);
int strcmpi(const char* lhs, const char* rhs);
int strnicmp(const char* lhs, const char* rhs, size_t length);
char* strupr(char* text);
char* strlwr(char* text);
uint16_t htons(uint16_t value);
uint16_t ntohs(uint16_t value);
uint32_t htonl(uint32_t value);
uint32_t ntohl(uint32_t value);
