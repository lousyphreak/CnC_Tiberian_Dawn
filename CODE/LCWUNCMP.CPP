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

#include <stdint.h>

uint32_t LCW_Uncompress(void const* source, void* dest, uint32_t length)
{
	uint8_t* source_ptr = (uint8_t*)source;
	uint8_t* dest_ptr = (uint8_t*)dest;
	uint8_t* dest_start = (uint8_t*)dest;
	uint8_t* dest_end = dest_start + length;
	uint8_t* copy_ptr;
	uint8_t op_code;
	uint8_t data;
	uint32_t count;

	if (source == nullptr || dest == nullptr || length == 0U) {
		return 0;
	}

	while (true) {
		op_code = *source_ptr++;

		if (!(op_code & 0x80U)) {
			count = (uint32_t)(op_code >> 4) + 3U;
			copy_ptr = dest_ptr - ((uint32_t)(*source_ptr++) + (((uint32_t)op_code & 0x0FU) << 8));
			if (copy_ptr < dest_start || copy_ptr >= dest_ptr) {
				return (uint32_t)(dest_ptr - dest_start);
			}
			if (dest_ptr + count > dest_end) {
				count = (uint32_t)(dest_end - dest_ptr);
			}

			while (count-- != 0U) {
				*dest_ptr++ = *copy_ptr++;
			}
		} else if (!(op_code & 0x40U)) {
			if (op_code == 0x80U) {
				return (uint32_t)(dest_ptr - (uint8_t*)dest);
			}

			count = (uint32_t)(op_code & 0x3FU);
			if (dest_ptr + count > dest_end) {
				count = (uint32_t)(dest_end - dest_ptr);
			}
			while (count-- != 0U) {
				*dest_ptr++ = *source_ptr++;
			}
		} else if (op_code == 0xFEU) {
			count = (uint32_t)source_ptr[0] + ((uint32_t)source_ptr[1] << 8);
			data = source_ptr[2];
			source_ptr += 3;
			if (dest_ptr + count > dest_end) {
				count = (uint32_t)(dest_end - dest_ptr);
			}

			while (count-- != 0U) {
				*dest_ptr++ = data;
			}
		} else if (op_code == 0xFFU) {
			count = (uint32_t)source_ptr[0] + ((uint32_t)source_ptr[1] << 8);
			copy_ptr = (uint8_t*)dest + source_ptr[2] + ((uint32_t)source_ptr[3] << 8);
			source_ptr += 4;
			if (copy_ptr < dest_start || copy_ptr >= dest_ptr) {
				return (uint32_t)(dest_ptr - dest_start);
			}
			if (dest_ptr + count > dest_end) {
				count = (uint32_t)(dest_end - dest_ptr);
			}

			while (count-- != 0U) {
				*dest_ptr++ = *copy_ptr++;
			}
		} else {
			count = (uint32_t)(op_code & 0x3FU) + 3U;
			copy_ptr = (uint8_t*)dest + source_ptr[0] + ((uint32_t)source_ptr[1] << 8);
			source_ptr += 2;
			if (copy_ptr < dest_start || copy_ptr >= dest_ptr) {
				return (uint32_t)(dest_ptr - dest_start);
			}
			if (dest_ptr + count > dest_end) {
				count = (uint32_t)(dest_end - dest_ptr);
			}

			while (count-- != 0U) {
				*dest_ptr++ = *copy_ptr++;
			}
		}
	}
}
