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

#include <stdint.h>
#include <string.h>

static uint8_t TopLine[640];
static uint8_t BottomLine[640];
static uint8_t LineBuffer[640];

static void Interpolate_Single_Line(uint8_t const* source_ptr, uint8_t* dest_ptr, int source_width)
{
	if (source_width <= 0) {
		return;
	}

	for (int index = 0; index < source_width - 1; ++index) {
		dest_ptr[index * 2] = source_ptr[index];
		dest_ptr[index * 2 + 1] = PaletteInterpolationTable[source_ptr[index + 1]][source_ptr[index]];
	}

	dest_ptr[(source_width - 1) * 2] = source_ptr[source_width - 1];
	dest_ptr[(source_width - 1) * 2 + 1] = 0;
}

static void Interpolate_Between_Lines(uint8_t const* source1, uint8_t const* source2, uint8_t* destination, int source_width)
{
	for (int index = 0; index < source_width * 2; ++index) {
		destination[index] = PaletteInterpolationTable[source2[index]][source1[index]];
	}
}

extern "C" void Stop_Execution(void)
{
}

extern "C" void __cdecl Asm_Create_Palette_Interpolation_Table(void)
{
	int first_r;
	int first_g;
	int first_b;
	int second_r;
	int second_g;
	int second_b;
	int diff_r;
	int diff_g;
	int diff_b;
	int dest_r;
	int dest_g;
	int dest_b;
	int distance;
	int closest_distance;
	int index_of_closest_color;
	unsigned char* first_palette_ptr = InterpolationPalette;

	for (int i = 0; i < SIZE_OF_PALETTE; ++i) {
		first_r = *first_palette_ptr++;
		first_g = *first_palette_ptr++;
		first_b = *first_palette_ptr++;

		unsigned char* second_palette_ptr = InterpolationPalette;
		for (int j = 0; j < SIZE_OF_PALETTE; ++j) {
			second_r = *second_palette_ptr++;
			second_g = *second_palette_ptr++;
			second_b = *second_palette_ptr++;

			dest_r = (first_r + second_r) >> 1;
			dest_g = (first_g + second_g) >> 1;
			dest_b = (first_b + second_b) >> 1;

			index_of_closest_color = 0;
			closest_distance = 500000;
			unsigned char* match_pal_ptr = InterpolationPalette;
			for (int p = 0; p < SIZE_OF_PALETTE; ++p) {
				diff_r = ((int)(*match_pal_ptr++)) - dest_r;
				diff_g = ((int)(*match_pal_ptr++)) - dest_g;
				diff_b = ((int)(*match_pal_ptr++)) - dest_b;
				distance = (diff_r * diff_r) + (diff_g * diff_g) + (diff_b * diff_b);
				if (distance < closest_distance) {
					closest_distance = distance;
					index_of_closest_color = p;
				}
			}

			PaletteInterpolationTable[i][j] = (unsigned char)index_of_closest_color;
		}
	}
}

extern "C" void __cdecl Asm_Interpolate(uint8_t* src_ptr, uint8_t* dest_ptr, int lines, int source_width, int dest_width)
{
	const int row_bytes = dest_width / 2;

	for (int row = 0; row < lines; ++row) {
		uint8_t* dest_row = dest_ptr + row * row_bytes * 2;
		Interpolate_Single_Line(src_ptr + row * source_width, LineBuffer, source_width);
		memcpy(dest_row, LineBuffer, (size_t)(source_width * 2));
		memcpy(dest_row + row_bytes, LineBuffer, (size_t)(source_width * 2));
	}
}

extern "C" void __cdecl Asm_Interpolate_Line_Double(uint8_t* src_ptr, uint8_t* dest_ptr, int lines, int source_width, int dest_width)
{
	const int row_bytes = dest_width / 2;

	for (int row = 0; row < lines; ++row) {
		uint8_t* dest_row = dest_ptr + row * row_bytes * 2;
		Interpolate_Single_Line(src_ptr + row * source_width, LineBuffer, source_width);
		memcpy(dest_row, LineBuffer, (size_t)(source_width * 2));
		memcpy(dest_row + row_bytes, LineBuffer, (size_t)(source_width * 2));
	}
}

extern "C" void __cdecl Asm_Interpolate_Line_Interpolate(uint8_t* src_ptr, uint8_t* dest_ptr, int source_lines, int source_width, int dest_width)
{
	const int row_bytes = dest_width / 2;
	uint8_t* last_line = TopLine;
	uint8_t* next_line = BottomLine;

	if (source_lines <= 0) {
		return;
	}

	Interpolate_Single_Line(src_ptr, next_line, source_width);
	memcpy(dest_ptr, next_line, (size_t)(source_width * 2));
	memcpy(dest_ptr + row_bytes, next_line, (size_t)(source_width * 2));
	src_ptr += source_width;
	dest_ptr += row_bytes * 2;
	--source_lines;

	while (source_lines > 0) {
		uint8_t* swap_line = last_line;
		last_line = next_line;
		next_line = swap_line;
		Interpolate_Single_Line(src_ptr, next_line, source_width);
		Interpolate_Between_Lines(last_line, next_line, LineBuffer, source_width);
		memcpy(dest_ptr, last_line, (size_t)(source_width * 2));
		memcpy(dest_ptr + row_bytes, LineBuffer, (size_t)(source_width * 2));
		dest_ptr += row_bytes * 2;
		src_ptr += source_width;
		--source_lines;
	}
}
