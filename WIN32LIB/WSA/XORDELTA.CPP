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

#include "WSA.H"
#include <stdint.h>

namespace {

enum {
	DO_XOR = 0,
	DO_COPY = 1
};

template<typename Writer>
void Apply_XOR_Delta_Core(uint8_t* target, const uint8_t* delta, Writer writer)
{
	while (true) {
		uint32_t opcode = *delta++;

		if (opcode == 0) {
			uint32_t count = *delta++;
			const uint8_t value = *delta++;
			while (count-- != 0) {
				writer(target, value);
				++target;
			}
			continue;
		}

		if ((opcode & 0x80U) == 0U) {
			uint32_t count = opcode;
			while (count-- != 0) {
				writer(target, *delta++);
				++target;
			}
			continue;
		}

		opcode -= 0x80U;
		if (opcode != 0U) {
			target += opcode;
			continue;
		}

		uint32_t count = (uint32_t)delta[0] | ((uint32_t)delta[1] << 8);
		delta += 2;
		if (count == 0U) {
			return;
		}

		if ((count & 0x8000U) == 0U) {
			target += count;
			continue;
		}

		count -= 0x8000U;
		if (count & 0x4000U) {
			count -= 0x4000U;
			const uint8_t value = *delta++;
			while (count-- != 0) {
				writer(target, value);
				++target;
			}
			continue;
		}

		while (count-- != 0) {
			writer(target, *delta++);
			++target;
		}
	}
}

template<typename Writer>
void Apply_XOR_Delta_2D_Core(uint8_t* target, const uint8_t* delta, int32_t width, int32_t nextrow, Writer writer)
{
	if (width <= 0 || nextrow <= 0) {
		return;
	}

	uint8_t* row_start = target;
	int32_t column = 0;

	auto advance = [&](void) {
		++column;
		++target;
		if (column >= width) {
			row_start += nextrow;
			target = row_start;
			column = 0;
		}
	};

	auto skip = [&](uint32_t count) {
		while (count != 0U) {
			const uint32_t available = (uint32_t)(width - column);
			const uint32_t step = (count < available) ? count : available;
			target += step;
			column += (int32_t)step;
			count -= step;
			if (column >= width) {
				row_start += nextrow;
				target = row_start;
				column = 0;
			}
		}
	};

	while (true) {
		uint32_t opcode = *delta++;

		if (opcode == 0) {
			uint32_t count = *delta++;
			const uint8_t value = *delta++;
			while (count-- != 0U) {
				writer(target, value);
				advance();
			}
			continue;
		}

		if ((opcode & 0x80U) == 0U) {
			uint32_t count = opcode;
			while (count-- != 0U) {
				writer(target, *delta++);
				advance();
			}
			continue;
		}

		opcode -= 0x80U;
		if (opcode != 0U) {
			skip(opcode);
			continue;
		}

		uint32_t count = (uint32_t)delta[0] | ((uint32_t)delta[1] << 8);
		delta += 2;
		if (count == 0U) {
			return;
		}

		if ((count & 0x8000U) == 0U) {
			skip(count);
			continue;
		}

		count -= 0x8000U;
		if (count & 0x4000U) {
			count -= 0x4000U;
			const uint8_t value = *delta++;
			while (count-- != 0U) {
				writer(target, value);
				advance();
			}
			continue;
		}

		while (count-- != 0U) {
			writer(target, *delta++);
			advance();
		}
	}
}

static inline void Xor_Byte(uint8_t* target, uint8_t value)
{
	*target ^= value;
}

static inline void Copy_Byte(uint8_t* target, uint8_t value)
{
	*target = value;
}

}

uint32_t  Apply_XOR_Delta(char *source_ptr, char *delta_ptr){
	if (source_ptr == 0 || delta_ptr == 0) {
		return 0;
	}

	Apply_XOR_Delta_Core((uint8_t*)source_ptr, (const uint8_t*)delta_ptr, Xor_Byte);
	return 0;
}

void  Apply_XOR_Delta_To_Page_Or_Viewport(void *target, void *delta, int32_t width, int32_t nextrow, int32_t copy){
	if (target == 0 || delta == 0) {
		return;
	}

	if (copy == DO_XOR) {
		Apply_XOR_Delta_2D_Core((uint8_t*)target, (const uint8_t*)delta, width, nextrow, Xor_Byte);
	} else {
		Apply_XOR_Delta_2D_Core((uint8_t*)target, (const uint8_t*)delta, width, nextrow, Copy_Byte);
	}
}
