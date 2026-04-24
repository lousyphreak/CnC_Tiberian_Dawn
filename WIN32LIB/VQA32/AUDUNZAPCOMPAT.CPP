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

#include <VQM32/COMPRESS.H>

namespace {

static const int8_t kDecode2Bit[4] = {-2, -1, 0, 1};
static const int8_t kDecode4Bit[16] = {-9, -8, -6, -5, -4, -3, -2, -1,
		0, 1, 2, 3, 4, 5, 6, 8};

static uint8_t Saturating_Add(uint8_t sample, int delta)
{
	int value = (int)sample + delta;

	if (value < 0) {
		return (0);
	}

	if (value > 255) {
		return (255);
	}

	return ((uint8_t)value);
}

} /* namespace */

int32_t  AudioUnzap(void *source, void *dest, int32_t count){
	uint8_t *src;
	uint8_t *dst;
	uint8_t previous;
	int32_t remaining;
	int32_t incount;

	if (source == NULL || dest == NULL || count <= 0) {
		return (0);
	}

	src = (uint8_t *)source;
	dst = (uint8_t *)dest;
	previous = 0x80U;
	remaining = count;
	incount = 0;

	while (remaining > 0) {
		const uint8_t codebyte = *src++;
		const uint8_t code = (uint8_t)(codebyte >> 6);
		const uint8_t value = (uint8_t)(codebyte & 0x3FU);

		++incount;

		if (code == 2U) {
			if ((value & 0x20U) != 0) {
				const int8_t delta = (int8_t)((int8_t)(value << 3) >> 3);
				previous = (uint8_t)(previous + delta);
				*dst++ = previous;
				--remaining;
			} else {
				const int32_t raw_count = (int32_t)value + 1L;

				memcpy(dst, src, (size_t)raw_count);
				src += raw_count;
				dst += raw_count;
				incount += raw_count;
				remaining -= raw_count;
				previous = dst[-1];
			}
			continue;
		}

		if (code == 1U) {
			int32_t packed_count = (int32_t)value + 1L;

			while (packed_count-- > 0) {
				const uint8_t packed = *src++;

				++incount;
				previous = Saturating_Add(previous, kDecode4Bit[packed & 0x0F]);
				*dst++ = previous;
				previous = Saturating_Add(previous, kDecode4Bit[(packed >> 4) & 0x0F]);
				*dst++ = previous;
				remaining -= 2;
			}
			continue;
		}

		if (code == 0U) {
			int32_t packed_count = (int32_t)value + 1L;

			while (packed_count-- > 0) {
				const uint8_t packed = *src++;

				++incount;
				previous = Saturating_Add(previous, kDecode2Bit[packed & 0x03]);
				*dst++ = previous;
				previous = Saturating_Add(previous, kDecode2Bit[(packed >> 2) & 0x03]);
				*dst++ = previous;
				previous = Saturating_Add(previous, kDecode2Bit[(packed >> 4) & 0x03]);
				*dst++ = previous;
				previous = Saturating_Add(previous, kDecode2Bit[(packed >> 6) & 0x03]);
				*dst++ = previous;
				remaining -= 4;
			}
			continue;
		}

		memset(dst, previous, (size_t)((int32_t)value + 1L));
		dst += (int32_t)value + 1L;
		remaining -= (int32_t)value + 1L;
	}

	return (incount);
}
