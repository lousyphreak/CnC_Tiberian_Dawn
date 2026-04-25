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
#include "SHAPE.H"

#include <cstdarg>
#include <cstdint>

namespace {

constexpr int BfShapeTrans = 0x0040;
constexpr uint8_t PredatorPositiveOffsets[8] = {1, 3, 2, 5, 2, 3, 4, 1};
constexpr int8_t PredatorNegativeOffsets[8] = {-1, -3, -2, -5, -2, -4, -3, -1};

static inline uint8_t Apply_Ghost(uint8_t source, uint8_t dest, const uint8_t* is_translucent, const uint8_t* translucent)
{
	if (is_translucent == nullptr || translucent == nullptr) {
		return source;
	}

	const uint8_t table_index = is_translucent[source];
	if (table_index == 0xFFU) {
		return source;
	}

	return translucent[(static_cast<uint32_t>(table_index) << 8) + dest];
}

static inline uint8_t Apply_Fading(uint8_t color, const uint8_t* fading_table, int fading_count)
{
	while (fading_count-- > 0) {
		color = fading_table[color];
	}
	return color;
}

static inline bool Should_Draw_Predator_Pixel(int& partial_count, int partial_pred)
{
	const int combined = partial_count + partial_pred;
	if ((combined & 0xFF00) == 0) {
		partial_count = combined;
		return false;
	}

	partial_count = combined & 0xFF;
	return true;
}

static inline uint8_t Sample_Predator_Pixel(const uint8_t* view_base,
	int view_width,
	int row_bytes,
	int x,
	int y,
	bool use_negative_offsets,
	uint8_t& pred_index)
{
	const int offset = use_negative_offsets ? PredatorNegativeOffsets[pred_index >> 1] : PredatorPositiveOffsets[pred_index >> 1];
	pred_index = static_cast<uint8_t>((pred_index + 2) & 0x0E);

	int sample_x = x + offset;
	if (sample_x < 0) {
		sample_x = 0;
	} else if (sample_x >= view_width) {
		sample_x = view_width - 1;
	}

	return view_base[(y * row_bytes) + sample_x];
}

}

int32_t Buffer_Frame_To_Page(int x,
	int y,
	int w,
	int h,
	void* buffer,
	GraphicViewPortClass& view,
	int flags,
	...)
{
	if (buffer == nullptr || w <= 0 || h <= 0) {
		return 0;
	}

	const uint8_t* is_translucent = nullptr;
	const uint8_t* translucent = nullptr;
	const uint8_t* fading_table = nullptr;
	int fading_count = 0;
	int partial_pred = 0x100;
	int predator_offset = 0;

	va_list args;
	va_start(args, flags);
	if (flags & SHAPE_GHOST) {
		is_translucent = static_cast<const uint8_t*>(va_arg(args, void*));
		if (is_translucent != nullptr) {
			translucent = is_translucent + 256;
		}
	}
	if (flags & SHAPE_FADING) {
		fading_table = static_cast<const uint8_t*>(va_arg(args, void*));
		fading_count = va_arg(args, int) & 0x3F;
		if (fading_count == 0) {
			flags &= ~SHAPE_FADING;
		}
	}
	if (flags & SHAPE_PREDATOR) {
		predator_offset = va_arg(args, int);
	}
	if (flags & SHAPE_PARTIAL) {
		partial_pred = va_arg(args, int) & 0xFF;
	}
	va_end(args);

	if (flags & SHAPE_CENTER) {
		x -= w / 2;
		y -= h / 2;
	}

	int src_x = 0;
	int src_y = 0;
	int dst_x = x;
	int dst_y = y;
	int dst_x1 = x + w;
	int dst_y1 = y + h;

	if (dst_x < 0) {
		src_x = -dst_x;
		dst_x = 0;
	}
	if (dst_y < 0) {
		src_y = -dst_y;
		dst_y = 0;
	}
	if (dst_x1 > view.Get_Width()) {
		dst_x1 = view.Get_Width();
	}
	if (dst_y1 > view.Get_Height()) {
		dst_y1 = view.Get_Height();
	}

	const int draw_width = dst_x1 - dst_x;
	const int draw_height = dst_y1 - dst_y;
	if (draw_width <= 0 || draw_height <= 0) {
		return 0;
	}

	const int row_bytes = view.Get_Width() + view.Get_XAdd() + view.Get_Pitch();
	const uint8_t* src = static_cast<const uint8_t*>(buffer) + (src_y * w) + src_x;
	uint8_t* dst = reinterpret_cast<uint8_t*>(view.Get_Offset()) + (dst_y * row_bytes) + dst_x;
	const uint8_t* view_base = reinterpret_cast<const uint8_t*>(view.Get_Offset());
	const int src_adjust = w - draw_width;
	const bool predator_negative = (predator_offset < 0);
	uint8_t pred_index = static_cast<uint8_t>(((predator_offset < 0 ? -predator_offset : predator_offset) << 1) & 0x0E);
	int partial_count = 0;

	for (int row = 0; row < draw_height; ++row) {
		uint8_t* row_dst = dst;
		const int view_y = dst_y + row;

		for (int col = 0; col < draw_width; ++col) {
			const uint8_t source = *src++;
			if ((flags & BfShapeTrans) && source == 0) {
				++row_dst;
				continue;
			}

			uint8_t color = source;
			if (flags & SHAPE_PREDATOR) {
				if (!Should_Draw_Predator_Pixel(partial_count, partial_pred)) {
					++row_dst;
					continue;
				}
				color = Sample_Predator_Pixel(view_base, view.Get_Width(), row_bytes, dst_x + col, view_y, predator_negative, pred_index);
			}

			if (flags & SHAPE_GHOST) {
				color = Apply_Ghost(color, *row_dst, is_translucent, translucent);
			}

			if (flags & SHAPE_FADING) {
				color = Apply_Fading(color, fading_table, fading_count);
			}

			*row_dst++ = color;
		}

		src += src_adjust;
		dst += row_bytes;
	}

	return 1;
}
