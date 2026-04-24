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

#include "MOUSE.H"
#include "IFF.H"
#include "SHAPE.H"
#include <stdint.h>
#include <new>
#include <string.h>


static inline uint8_t *View_Data(GraphicViewPortClass *view)
{
	return((uint8_t *)(uintptr_t)view->Get_Offset());
}


static inline int32_t View_Row_Bytes(GraphicViewPortClass const *view)
{
	return(view->Get_Width() + view->Get_XAdd() + view->Get_Pitch());
}


static uint8_t const *Prepare_Cursor_Data(void const *cursor, uint8_t **storage)
{
	Shape_Type const *shape = (Shape_Type const *)cursor;
	uint8_t *buffer;
	int32_t header_size;

	*storage = NULL;
	if (!cursor) {
		return(NULL);
	}

	header_size = (shape->ShapeType & MAKESHAPE_COMPACT) ? 26 : 10;
	if (shape->ShapeType & MAKESHAPE_NOCOMP) {
		return((uint8_t const *)cursor);
	}

	buffer = new(std::nothrow) uint8_t[header_size + Get_Shape_Uncomp_Size(cursor)];
	if (!buffer) {
		return(NULL);
	}

	memcpy(buffer, cursor, header_size);
	((Shape_Type *)buffer)->ShapeType = (uint16_t)(shape->ShapeType | MAKESHAPE_NOCOMP);
	LCW_Uncompress((void *)((uint8_t const *)cursor + header_size), buffer + header_size, (uint32_t)Get_Shape_Uncomp_Size(cursor));
	*storage = buffer;
	return(buffer);
}


void  Mouse_Shadow_Buffer(void *thisptr, GraphicViewPortClass *srcdst, void *buffer, int32_t x, int32_t y, int32_t hotx, int32_t hoty, int32_t store){
	WWMouseClass *mouse = (WWMouseClass *)thisptr;
	uint8_t *view_ptr;
	uint8_t *buffer_ptr;
	int32_t x0;
	int32_t y0;
	int32_t width;
	int32_t height;
	int32_t row_bytes;

	if (!mouse || !srcdst || !buffer || mouse->CursorWidth <= 0 || mouse->CursorHeight <= 0) {
		return;
	}

	x0 = x - hotx;
	y0 = y - hoty;
	if (x0 >= srcdst->Get_Width() || y0 >= srcdst->Get_Height() || (x0 + mouse->CursorWidth) <= 0 || (y0 + mouse->CursorHeight) <= 0) {
		return;
	}

	width = mouse->CursorWidth;
	height = mouse->CursorHeight;
	buffer_ptr = (uint8_t *)buffer;
	if (x0 < 0) {
		buffer_ptr += -x0;
		width += x0;
		x0 = 0;
	}
	if (y0 < 0) {
		buffer_ptr += (-y0) * mouse->CursorWidth;
		height += y0;
		y0 = 0;
	}
	if (x0 + width > srcdst->Get_Width()) {
		width = srcdst->Get_Width() - x0;
	}
	if (y0 + height > srcdst->Get_Height()) {
		height = srcdst->Get_Height() - y0;
	}
	if (width <= 0 || height <= 0) {
		return;
	}

	row_bytes = View_Row_Bytes(srcdst);
	view_ptr = View_Data(srcdst) + y0 * row_bytes + x0;
	for (int32_t row = 0; row < height; ++row) {
		if (store) {
			memcpy(buffer_ptr, view_ptr, width);
		} else {
			memcpy(view_ptr, buffer_ptr, width);
		}
		view_ptr += row_bytes;
		buffer_ptr += mouse->CursorWidth;
	}
}


void  Draw_Mouse(void *thisptr, GraphicViewPortClass *srcdst, int32_t x, int32_t y){
	WWMouseClass *mouse = (WWMouseClass *)thisptr;
	uint8_t const *cursor_ptr;
	uint8_t *view_ptr;
	int32_t x0;
	int32_t y0;
	int32_t width;
	int32_t height;
	int32_t row_bytes;

	if (!mouse || !srcdst || mouse->CursorWidth <= 0 || mouse->CursorHeight <= 0 || !mouse->MouseCursor) {
		return;
	}

	x0 = x - mouse->MouseXHot;
	y0 = y - mouse->MouseYHot;
	if (x0 >= srcdst->Get_Width() || y0 >= srcdst->Get_Height() || (x0 + mouse->CursorWidth) <= 0 || (y0 + mouse->CursorHeight) <= 0) {
		return;
	}

	width = mouse->CursorWidth;
	height = mouse->CursorHeight;
	cursor_ptr = (uint8_t const *)mouse->MouseCursor;
	if (x0 < 0) {
		cursor_ptr += -x0;
		width += x0;
		x0 = 0;
	}
	if (y0 < 0) {
		cursor_ptr += (-y0) * mouse->CursorWidth;
		height += y0;
		y0 = 0;
	}
	if (x0 + width > srcdst->Get_Width()) {
		width = srcdst->Get_Width() - x0;
	}
	if (y0 + height > srcdst->Get_Height()) {
		height = srcdst->Get_Height() - y0;
	}
	if (width <= 0 || height <= 0) {
		return;
	}

	row_bytes = View_Row_Bytes(srcdst);
	view_ptr = View_Data(srcdst) + y0 * row_bytes + x0;
	for (int32_t row = 0; row < height; ++row) {
		for (int32_t column = 0; column < width; ++column) {
			uint8_t pixel = cursor_ptr[column];
			if (pixel != 0) {
				view_ptr[column] = pixel;
			}
		}
		cursor_ptr += mouse->CursorWidth;
		view_ptr += row_bytes;
	}
}


void *  ASM_Set_Mouse_Cursor(void *thisptr, int32_t hotspotx, int32_t hotspoty, void *cursor){
	WWMouseClass *mouse = (WWMouseClass *)thisptr;
	char *previous;
	uint8_t *storage;
	uint8_t const *shape_data;
	Shape_Type const *shape;
	uint8_t const *cursor_data;
	int32_t width;
	int32_t height;
	int32_t remaining;

	if (!mouse) {
		return(NULL);
	}

	previous = mouse->PrevCursor;
	mouse->PrevCursor = (char *)cursor;
	if (!cursor) {
		return(previous);
	}

	width = Get_Shape_Width(cursor);
	height = Get_Shape_Original_Height(cursor);
	if (width > mouse->MaxWidth || height > mouse->MaxHeight || width <= 0 || height <= 0) {
		return(previous);
	}

	shape_data = Prepare_Cursor_Data(cursor, &storage);
	if (!shape_data) {
		return(previous);
	}

	shape = (Shape_Type const *)shape_data;
	cursor_data = shape_data + ((shape->ShapeType & MAKESHAPE_COMPACT) ? 26 : 10);
	remaining = width * height;
	if (shape->ShapeType & MAKESHAPE_COMPACT) {
		uint8_t const *lookup = shape->Colortable;
		uint8_t *dest = (uint8_t *)mouse->MouseCursor;

		while (remaining > 0) {
			uint8_t value = *cursor_data++;
			if (value != 0) {
				*dest++ = lookup[value];
				--remaining;
			} else {
				int32_t zeros = *cursor_data++;
				memset(dest, 0, zeros);
				dest += zeros;
				remaining -= zeros;
			}
		}
	} else {
		uint8_t *dest = (uint8_t *)mouse->MouseCursor;

		while (remaining > 0) {
			uint8_t value = *cursor_data++;
			if (value != 0) {
				*dest++ = value;
				--remaining;
			} else {
				int32_t zeros = *cursor_data++;
				memset(dest, 0, zeros);
				dest += zeros;
				remaining -= zeros;
			}
		}
	}

	mouse->MouseXHot = hotspotx;
	mouse->MouseYHot = hotspoty;
	mouse->CursorWidth = width;
	mouse->CursorHeight = height;

	delete[] storage;
	return(previous);
}
