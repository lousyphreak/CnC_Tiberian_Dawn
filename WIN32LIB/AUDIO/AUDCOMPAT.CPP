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

#include "SOUNDINT.H"
#include <stdint.h>
#include <string.h>

namespace {

enum SCodeType {
	CODE_2BIT,
	CODE_4BIT,
	CODE_RAW,
	CODE_SILENCE
};

static const int32_t Decode2Bit[4] = {-2, -1, 0, 1};
static const int32_t Decode4Bit[16] = {-9, -8, -6, -5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 8};

static inline uint8_t Clamp_Unsigned_Byte(int32_t value)
{
	if (value < 0) {
		return 0;
	}
	if (value > 255) {
		return 255;
	}
	return (uint8_t)value;
}

}

int32_t  Decompress_Frame(void *source, void *dest, int32_t size){
	if (source == 0 || dest == 0 || size <= 0) {
		return 0;
	}

	uint32_t previous = 0x80U;
	int8_t* s = (int8_t*)source;
	uint8_t* d = (uint8_t*)dest;
	int32_t incount = 0;

	while (size > 0) {
		const int8_t code = *s++;
		int32_t counter = (code & 0x3F) + 1;
		++incount;

		switch ((code >> 6) & 0x03) {
			case CODE_RAW:
				if ((counter - 1) & 0x20) {
					counter = (counter - 1) & 0x1F;
					if (counter & 0x10) {
						counter |= ~0x1F;
					}
					previous = Clamp_Unsigned_Byte((int32_t)previous + counter);
					*d++ = (uint8_t)previous;
					--size;
				} else {
					memcpy(d, s, (size_t)counter);
					incount += counter;
					size -= counter;
					d += counter - 1;
					s += counter;
					previous = *d++;
				}
				break;

			case CODE_4BIT:
				while (counter > 0 && size > 0) {
					const uint8_t delta = (uint8_t)*s++;
					++incount;

					previous = Clamp_Unsigned_Byte((int32_t)previous + Decode4Bit[delta & 0x0F]);
					*d++ = (uint8_t)previous;
					--size;

					if (size == 0) {
						break;
					}

					previous = Clamp_Unsigned_Byte((int32_t)previous + Decode4Bit[(delta >> 4) & 0x0F]);
					*d++ = (uint8_t)previous;
					--size;
					--counter;
				}
				break;

			case CODE_2BIT:
				while (counter > 0 && size > 0) {
					const uint8_t delta = (uint8_t)*s++;
					++incount;

					for (int32_t shift = 0; shift < 8 && size > 0; shift += 2) {
						previous = Clamp_Unsigned_Byte((int32_t)previous + Decode2Bit[(delta >> shift) & 0x03]);
						*d++ = (uint8_t)previous;
						--size;
					}
					--counter;
				}
				break;

			case CODE_SILENCE:
			default:
				memset(d, (int32_t)previous, (size_t)counter);
				d += counter;
				size -= counter;
				break;
		}
	}

	return incount;
}

int32_t  Decompress_Frame_Lock(void){
	return 1;
}

int32_t  Decompress_Frame_Unlock(void){
	return 1;
}

int32_t  sosCODEC_Lock(void){
	return 1;
}

int32_t  sosCODEC_Unlock(void){
	return 1;
}
