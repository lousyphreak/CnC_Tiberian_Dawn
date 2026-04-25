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

#include "FUNCTION.H"

#include <cstdarg>
#include <cmath>
#include <cstdio>

char* __nheapbeg = nullptr;
char CPUType = 0;

static int Approx_Multiply_Shift7(int factor, int distance)
{
	return (factor * distance) >> 7;
}

int Get_Bit(void const* array, int bit)
{
	if (array == nullptr || bit < 0) {
		return 0;
	}

	const unsigned char* bytes = static_cast<const unsigned char*>(array);
	return (bytes[bit >> 3] >> (bit & 7)) & 1;
}

void Set_Bit(void* array, int bit, int value)
{
	if (array == nullptr || bit < 0) {
		return;
	}

	unsigned char* bytes = static_cast<unsigned char*>(array);
	const unsigned char mask = static_cast<unsigned char>(1U << (bit & 7));
	if (value) {
		bytes[bit >> 3] |= mask;
	} else {
		bytes[bit >> 3] &= static_cast<unsigned char>(~mask);
	}
}

int First_False_Bit(void const* array)
{
	if (array == nullptr) {
		return -1;
	}

	const unsigned char* bytes = static_cast<const unsigned char*>(array);
	for (int byte_index = 0; ; ++byte_index) {
		if (bytes[byte_index] != 0xFFU) {
			for (int bit = 0; bit < 8; ++bit) {
				if (((bytes[byte_index] >> bit) & 1U) == 0U) {
					return (byte_index << 3) + bit;
				}
			}
		}
	}
}

int Distance_Coord(COORDINATE coord1, COORDINATE coord2)
{
	const int dx = Coord_X(coord1) - Coord_X(coord2);
	const int dy = Coord_Y(coord1) - Coord_Y(coord2);
	return static_cast<int>(std::sqrt(static_cast<double>(dx * dx + dy * dy)));
}

int calcx(signed short factor, short distance)
{
	return Approx_Multiply_Shift7(factor, distance);
}

int calcy(signed short factor, short distance)
{
	return -Approx_Multiply_Shift7(factor, distance);
}

void* __cdecl Conquer_Build_Fading_Table(void const* palette, void* dest, int color, int frac)
{
	return Build_Fading_Table(palette, dest, color, frac);
}

void __cdecl Fat_Put_Pixel(int x, int y, int color, int size, GraphicViewPortClass& page)
{
	if (size <= 0 || x < 0 || y < 0 || x >= page.Get_Width() || y >= page.Get_Height()) {
		return;
	}

	const int clipped_width = Bound(0, size, page.Get_Width() - x);
	const int clipped_height = Bound(0, size, page.Get_Height() - y);
	for (int row = 0; row < clipped_height; ++row) {
		for (int col = 0; col < clipped_width; ++col) {
			page.Put_Pixel(x + col, y + row, static_cast<uint8_t>(color));
		}
	}
}

void Smart_Printf(char* format, ...)
{
	if (format == nullptr) {
		return;
	}

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void Hex_Dump_Data(char* buffer, int length)
{
	if (buffer == nullptr || length <= 0) {
		return;
	}

	for (int offset = 0; offset < length; offset += 16) {
		printf("%05X  ", offset);
		for (int index = 0; index < 16; ++index) {
			if (offset + index < length) {
				printf("%02X ", static_cast<unsigned char>(buffer[offset + index]));
			} else {
				printf("   ");
			}
		}
		printf(" ");
		for (int index = 0; index < 16 && offset + index < length; ++index) {
			const unsigned char c = static_cast<unsigned char>(buffer[offset + index]);
			putchar((c >= 32 && c < 127) ? c : '.');
		}
		putchar('\n');
	}
}

extern "C" bool __cdecl Detect_MMX_Availability(void)
{
#if defined(__MMX__) || defined(_M_IX86_FP) || defined(__x86_64__) || defined(_M_X64)
	return true;
#else
	return false;
#endif
}

extern "C" void __cdecl Init_MMX(void)
{
}

long __cdecl Get_EAX(void)
{
	return 0;
}

extern "C" void ModeX_Blit(GraphicBufferClass* source)
{
	if (source != nullptr) {
		source->Blit(SeenBuff);
	}
}
