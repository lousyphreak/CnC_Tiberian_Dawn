/*
**	Command & Conquer Red Alert(tm)
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

#include <stdint.h>
#include <string.h>
#include "UNVQ.H"

void  UnVQ_4x2(uint8_t *codebook, uint8_t *pointers,		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth)
{
	const uint32_t entry_count = blocksperrow * numrows;
	uint8_t *lo_ptr = pointers;
	uint8_t *hi_ptr = pointers + entry_count;
	uint8_t *row_ptr = buffer;

	if (codebook == NULL || pointers == NULL || buffer == NULL) {
		return;
	}

	for (uint32_t row = 0; row < numrows; ++row) {
		uint8_t *row0 = row_ptr;
		uint8_t *row1 = row_ptr + bufwidth;

		for (uint32_t column = 0; column < blocksperrow; ++column) {
			const uint8_t low = *lo_ptr++;
			const uint8_t high = *hi_ptr++;

			if (high == 0x0F) {
				memset(row0, low, 4);
				memset(row1, low, 4);
			} else {
				uint8_t *entry = codebook + ((((uint32_t)high << 8) | low) * 8);
				memcpy(row0, entry, 4);
				memcpy(row1, entry + 4, 4);
			}

			row0 += 4;
			row1 += 4;
		}

		row_ptr += bufwidth * 2;
	}
}

void  UnVQ_4x4(uint8_t *codebook, uint8_t *pointers,		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth)
{
	static const uint16_t SKIP_PTR = 0x8000u;
	uint8_t *row_ptr = buffer;

	if (codebook == NULL || pointers == NULL || buffer == NULL) {
		return;
	}

	for (uint32_t row = 0; row < numrows; ++row) {
		uint8_t *row0 = row_ptr;
		uint8_t *row1 = row_ptr + bufwidth;
		uint8_t *row2 = row_ptr + (bufwidth * 2);
		uint8_t *row3 = row_ptr + (bufwidth * 3);

		for (uint32_t column = 0; column < blocksperrow; ++column) {
			const uint16_t code = (uint16_t)pointers[0] | ((uint16_t)pointers[1] << 8);
			pointers += 2;

			if ((code & 0x8000u) != 0) {
				if (code != SKIP_PTR) {
					const uint8_t color = (uint8_t)(~code & 0xFFu);
					memset(row0, color, 4);
					memset(row1, color, 4);
					memset(row2, color, 4);
					memset(row3, color, 4);
				}
			} else {
				uint8_t *entry = codebook + code - 4;
				memcpy(row0, entry, 4);
				memcpy(row1, entry + 4, 4);
				memcpy(row2, entry + 8, 4);
				memcpy(row3, entry + 12, 4);
			}

			row0 += 4;
			row1 += 4;
			row2 += 4;
			row3 += 4;
		}

		row_ptr += bufwidth * 4;
	}
}

void  UnVQ_4x4_V2(uint8_t *codebook, uint8_t *pointers,		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth)
{
	const uint32_t entry_count = blocksperrow * numrows;
	uint8_t *lo_ptr = pointers;
	uint8_t *hi_ptr = pointers + entry_count;
	uint8_t *row_ptr = buffer;

	if (codebook == NULL || pointers == NULL || buffer == NULL) {
		return;
	}

	for (uint32_t row = 0; row < numrows; ++row) {
		uint8_t *row0 = row_ptr;
		uint8_t *row1 = row_ptr + bufwidth;
		uint8_t *row2 = row_ptr + (bufwidth * 2);
		uint8_t *row3 = row_ptr + (bufwidth * 3);

		for (uint32_t column = 0; column < blocksperrow; ++column) {
			const uint8_t low = *lo_ptr++;
			const uint8_t high = *hi_ptr++;

			if (high == 0xFFu) {
				memset(row0, low, 4);
				memset(row1, low, 4);
				memset(row2, low, 4);
				memset(row3, low, 4);
			} else {
				uint8_t *entry = codebook + ((((uint32_t)high << 8) | low) * 16);
				memcpy(row0, entry, 4);
				memcpy(row1, entry + 4, 4);
				memcpy(row2, entry + 8, 4);
				memcpy(row3, entry + 12, 4);
			}

			row0 += 4;
			row1 += 4;
			row2 += 4;
			row3 += 4;
		}

		row_ptr += bufwidth * 4;
	}
}
