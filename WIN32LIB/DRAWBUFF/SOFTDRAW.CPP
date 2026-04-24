/*
**Command & Conquer Red Alert(tm)
**Copyright 2025 Electronic Arts Inc.
**
**This program is free software: you can redistribute it and/or modify
**it under the terms of the GNU General Public License as published by
**the Free Software Foundation, either version 3 of the License, or
**(at your option) any later version.
**
**This program is distributed in the hope that it will be useful,
**but WITHOUT ANY WARRANTY; without even the implied warranty of
**MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**GNU General Public License for more details.
**
**You should have received a copy of the GNU General Public License
**along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "DRAWBUFF.H"
#include "FONT.H"
#include "GBUFFER.H"
#include "ICONCACH.H"
#include "TILE.H"
#include "WWSTD.H"
#include <stdint.h>
#include <string.h>

uint8_t ColorXlat[256];

static int32_t FontPaletteInitialized = 0;

static void Ensure_Font_Palette(void) {
  if (!FontPaletteInitialized) {
    memset(ColorXlat, 0, sizeof(ColorXlat));
    for (int32_t index = 0; index < 16; ++index) {
      ColorXlat[index] = (uint8_t)index;
      ColorXlat[index << 4] = (uint8_t)index;
    }
    FontPaletteInitialized = 1;
  }
}

static inline uint8_t *View_Data(GraphicViewPortClass *view) {
  return ((uint8_t *)(uintptr_t)view->Get_Offset());
}

static inline int32_t View_Row_Bytes(GraphicViewPortClass const *view) {
  return (view->Get_Width() + view->Get_XAdd() + view->Get_Pitch());
}

static uint16_t Read_Font_Word(uint8_t const *ptr) {
  uint16_t value;
  memcpy(&value, ptr, sizeof(value));
  return (value);
}

static int32_t Clip_Copy_Rect(int32_t src_width, int32_t src_height,
                              int32_t dst_width, int32_t dst_height,
                              int32_t *src_x, int32_t *src_y, int32_t *dst_x,
                              int32_t *dst_y, int32_t *copy_width,
                              int32_t *copy_height) {
  if (*src_x < 0) {
    *dst_x -= *src_x;
    *copy_width += *src_x;
    *src_x = 0;
  }
  if (*src_y < 0) {
    *dst_y -= *src_y;
    *copy_height += *src_y;
    *src_y = 0;
  }
  if (*dst_x < 0) {
    *src_x -= *dst_x;
    *copy_width += *dst_x;
    *dst_x = 0;
  }
  if (*dst_y < 0) {
    *src_y -= *dst_y;
    *copy_height += *dst_y;
    *dst_y = 0;
  }

  if (*src_x + *copy_width > src_width) {
    *copy_width = src_width - *src_x;
  }
  if (*src_y + *copy_height > src_height) {
    *copy_height = src_height - *src_y;
  }
  if (*dst_x + *copy_width > dst_width) {
    *copy_width = dst_width - *dst_x;
  }
  if (*dst_y + *copy_height > dst_height) {
    *copy_height = dst_height - *dst_y;
  }

  return (*copy_width > 0 && *copy_height > 0);
}

static int32_t Regions_Overlap(uint8_t const *src, int32_t src_row_bytes,
                               uint8_t const *dst, int32_t dst_row_bytes,
                               int32_t width, int32_t height) {
  uint8_t const *src_end;
  uint8_t const *dst_end;

  if (width <= 0 || height <= 0) {
    return (0);
  }

  src_end = src + (height - 1) * src_row_bytes + width;
  dst_end = dst + (height - 1) * dst_row_bytes + width;
  return (src < dst_end && dst < src_end);
}

static int32_t Resolve_Icon(IControl_Type const *control, int32_t icon) {
  if (!control) {
    return (-1);
  }

  if (control->Map) {
    icon = *(((uint8_t const *)control) + control->Map + icon);
  }

  if (icon < 0 || icon >= control->Count) {
    return (-1);
  }

  return (icon);
}

int32_t Buffer_Size_Of_Region(void *thisptr, int32_t w, int32_t h) {  (void)thisptr;
  return ((int32_t)w * (int32_t)h);
}

void Set_Font_Palette_Range(void const *palette, int32_t start_idx,                                       int32_t end_idx) {
  uint8_t const *colors;
  int32_t start;
  int32_t end;

  Ensure_Font_Palette();
  if (!palette) {
    return;
  }

  colors = (uint8_t const *)palette;
  start = start_idx & 0x0F;
  end = end_idx & 0x0F;
  if (end < start) {
    return;
  }

  for (int32_t index = start; index <= end; ++index) {
    uint8_t color = colors[index - start];
    ColorXlat[index] = color;
    ColorXlat[index << 4] = color;
  }
}

void *Get_Font_Palette_Ptr(void) {  Ensure_Font_Palette();
  return (ColorXlat);
}

void Buffer_Put_Pixel(void *thisptr, int32_t x, int32_t y,                                 uint8_t color) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;

  if (!view || x < 0 || y < 0 || x >= view->Get_Width() ||
      y >= view->Get_Height()) {
    return;
  }

  View_Data(view)[y * View_Row_Bytes(view) + x] = color;
}

int32_t Buffer_Get_Pixel(void *thisptr, int32_t x, int32_t y) {  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;

  if (!view || x < 0 || y < 0 || x >= view->Get_Width() ||
      y >= view->Get_Height()) {
    return (0);
  }

  return (View_Data(view)[y * View_Row_Bytes(view) + x]);
}

void Buffer_Clear(void *thisptr, uint8_t color) {  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  uint8_t *dest;
  int32_t row_bytes;

  if (!view) {
    return;
  }

  dest = View_Data(view);
  row_bytes = View_Row_Bytes(view);
  for (int32_t row = 0; row < view->Get_Height(); ++row) {
    memset(dest, color, view->Get_Width());
    dest += row_bytes;
  }
}

void Buffer_Fill_Rect(void *thisptr, int32_t sx, int32_t sy,                                 int32_t dx, int32_t dy, uint8_t color) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  uint8_t *dest;
  int32_t x0;
  int32_t y0;
  int32_t x1;
  int32_t y1;
  int32_t row_bytes;

  if (!view) {
    return;
  }

  x0 = MIN(sx, dx);
  y0 = MIN(sy, dy);
  x1 = MAX(sx, dx);
  y1 = MAX(sy, dy);
  if (x1 < 0 || y1 < 0 || x0 >= view->Get_Width() || y0 >= view->Get_Height()) {
    return;
  }

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 >= view->Get_Width())
    x1 = view->Get_Width() - 1;
  if (y1 >= view->Get_Height())
    y1 = view->Get_Height() - 1;
  if (x1 < x0 || y1 < y0) {
    return;
  }

  row_bytes = View_Row_Bytes(view);
  dest = View_Data(view) + y0 * row_bytes + x0;
  for (int32_t row = y0; row <= y1; ++row) {
    memset(dest, color, x1 - x0 + 1);
    dest += row_bytes;
  }
}

void Buffer_Remap(void *thisptr, int32_t sx, int32_t sy,                             int32_t width, int32_t height, void *remap) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  uint8_t *dest;
  uint8_t const *table = (uint8_t const *)remap;
  int32_t x0 = sx;
  int32_t y0 = sy;
  int32_t x1 = sx + width;
  int32_t y1 = sy + height;
  int32_t row_bytes;

  if (!view || !table || width <= 0 || height <= 0) {
    return;
  }

  if (x1 <= 0 || y1 <= 0 || x0 >= view->Get_Width() ||
      y0 >= view->Get_Height()) {
    return;
  }

  if (x0 < 0)
    x0 = 0;
  if (y0 < 0)
    y0 = 0;
  if (x1 > view->Get_Width())
    x1 = view->Get_Width();
  if (y1 > view->Get_Height())
    y1 = view->Get_Height();
  if (x1 <= x0 || y1 <= y0) {
    return;
  }

  row_bytes = View_Row_Bytes(view);
  dest = View_Data(view) + y0 * row_bytes + x0;
  for (int32_t row = y0; row < y1; ++row) {
    for (int32_t column = 0; column < (x1 - x0); ++column) {
      dest[column] = table[dest[column]];
    }
    dest += row_bytes;
  }
}

int32_t Buffer_To_Buffer(void *thisptr, int32_t x, int32_t y,                                    int32_t w, int32_t h, void *buff,
                                    int32_t size) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  uint8_t const *src;
  uint8_t *dest;
  int32_t source_x = x;
  int32_t source_y = y;
  int32_t dest_x = 0;
  int32_t dest_y = 0;
  int32_t buffer_width = w;
  int32_t buffer_height = h;
  int32_t row_bytes;

  if (!view || !buff || w <= 0 || h <= 0) {
    return (0);
  }

  if ((int32_t)buffer_width * (int32_t)buffer_height > size) {
    return (0);
  }

  if (source_x < 0) {
    dest_x = -source_x;
    w += source_x;
    source_x = 0;
  }
  if (source_y < 0) {
    dest_y = -source_y;
    h += source_y;
    source_y = 0;
  }
  if (source_x + w > view->Get_Width()) {
    w = view->Get_Width() - source_x;
  }
  if (source_y + h > view->Get_Height()) {
    h = view->Get_Height() - source_y;
  }
  if (w <= 0 || h <= 0) {
    return (0);
  }

  row_bytes = View_Row_Bytes(view);
  src = View_Data(view) + source_y * row_bytes + source_x;
  dest = (uint8_t *)buff + dest_y * buffer_width + dest_x;
  for (int32_t row = 0; row < h; ++row) {
    memcpy(dest, src, w);
    dest += buffer_width;
    src += row_bytes;
  }

  return ((int32_t)w * (int32_t)h);
}

int32_t Buffer_To_Page(int32_t x, int32_t y, int32_t w, int32_t h,                                  void *Buffer, void *viewptr) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)viewptr;
  uint8_t const *src;
  uint8_t *dest;
  int32_t src_x = 0;
  int32_t src_y = 0;
  int32_t dest_x = x;
  int32_t dest_y = y;
  int32_t source_width = w;
  int32_t row_bytes;

  if (!view || !Buffer || w <= 0 || h <= 0) {
    return (0);
  }

  if (dest_x < 0) {
    src_x = -dest_x;
    w += dest_x;
    dest_x = 0;
  }
  if (dest_y < 0) {
    src_y = -dest_y;
    h += dest_y;
    dest_y = 0;
  }
  if (dest_x + w > view->Get_Width()) {
    w = view->Get_Width() - dest_x;
  }
  if (dest_y + h > view->Get_Height()) {
    h = view->Get_Height() - dest_y;
  }
  if (w <= 0 || h <= 0) {
    return (0);
  }

  row_bytes = View_Row_Bytes(view);
  src = (uint8_t const *)Buffer + src_y * source_width + src_x;
  dest = View_Data(view) + dest_y * row_bytes + dest_x;
  for (int32_t row = 0; row < h; ++row) {
    memcpy(dest, src, w);
    dest += row_bytes;
    src += source_width;
  }

  return ((int32_t)w * (int32_t)h);
}

int32_t Linear_Blit_To_Linear(void *thisptr, void *destptr,                                         int32_t x_pixel, int32_t y_pixel,
                                         int32_t dx_pixel, int32_t dy_pixel,
                                         int32_t pixel_width,
                                         int32_t pixel_height, int32_t trans) {
  GraphicViewPortClass *source_view = (GraphicViewPortClass *)thisptr;
  GraphicViewPortClass *dest_view = (GraphicViewPortClass *)destptr;
  uint8_t const *source;
  uint8_t *dest;
  uint8_t *scratch = NULL;
  int32_t source_x = x_pixel;
  int32_t source_y = y_pixel;
  int32_t dest_x = dx_pixel;
  int32_t dest_y = dy_pixel;
  int32_t width = pixel_width;
  int32_t height = pixel_height;
  int32_t source_row_bytes;
  int32_t dest_row_bytes;
  int32_t overlapped;

  if (!source_view || !dest_view) {
    return (FALSE);
  }

  if (!Clip_Copy_Rect(source_view->Get_Width(), source_view->Get_Height(),
                      dest_view->Get_Width(), dest_view->Get_Height(),
                      &source_x, &source_y, &dest_x, &dest_y, &width,
                      &height)) {
    return (FALSE);
  }

  source_row_bytes = View_Row_Bytes(source_view);
  dest_row_bytes = View_Row_Bytes(dest_view);
  source = View_Data(source_view) + source_y * source_row_bytes + source_x;
  dest = View_Data(dest_view) + dest_y * dest_row_bytes + dest_x;
  overlapped = Regions_Overlap(source, source_row_bytes, dest, dest_row_bytes,
                               width, height);

  if (overlapped) {
    scratch = new uint8_t[width * height];
    if (scratch) {
      for (int32_t row = 0; row < height; ++row) {
        memcpy(scratch + row * width, source + row * source_row_bytes, width);
      }
      source = scratch;
      source_row_bytes = width;
    }
  }

  if (!trans) {
    for (int32_t row = 0; row < height; ++row) {
      memmove(dest + row * dest_row_bytes, source + row * source_row_bytes,
              width);
    }
  } else {
    for (int32_t row = 0; row < height; ++row) {
      uint8_t const *src_row = source + row * source_row_bytes;
      uint8_t *dst_row = dest + row * dest_row_bytes;
      for (int32_t column = 0; column < width; ++column) {
        uint8_t value = src_row[column];
        if (value != 0) {
          dst_row[column] = value;
        }
      }
    }
  }

  delete[] scratch;
  return (TRUE);
}

int32_t Linear_Scale_To_Linear(void *thisptr, void *destptr,                                          int32_t src_x, int32_t src_y,
                                          int32_t dst_x, int32_t dst_y,
                                          int32_t src_width, int32_t src_height,
                                          int32_t dst_width, int32_t dst_height,
                                          int32_t trans, char *remap) {
  GraphicViewPortClass *source_view = (GraphicViewPortClass *)thisptr;
  GraphicViewPortClass *dest_view = (GraphicViewPortClass *)destptr;
  double source_x0 = (double)src_x;
  double source_y0 = (double)src_y;
  double source_x1 = (double)(src_x + src_width);
  double source_y1 = (double)(src_y + src_height);
  double dest_x0 = (double)dst_x;
  double dest_y0 = (double)dst_y;
  double dest_x1 = (double)(dst_x + dst_width);
  double dest_y1 = (double)(dst_y + dst_height);
  uint8_t const *source_data;
  uint8_t *dest_data;
  int32_t source_row_bytes;
  int32_t dest_row_bytes;
  int32_t start_x;
  int32_t start_y;
  int32_t end_x;
  int32_t end_y;
  double clipped_dest_width;
  double clipped_dest_height;
  double clipped_source_width;
  double clipped_source_height;

  if (!source_view || !dest_view || src_width <= 0 || src_height <= 0 ||
      dst_width <= 0 || dst_height <= 0) {
    return (FALSE);
  }

  if (source_x0 < 0.0) {
    double ratio = (0.0 - source_x0) / (source_x1 - source_x0);
    dest_x0 += (dest_x1 - dest_x0) * ratio;
    source_x0 = 0.0;
  }
  if (source_y0 < 0.0) {
    double ratio = (0.0 - source_y0) / (source_y1 - source_y0);
    dest_y0 += (dest_y1 - dest_y0) * ratio;
    source_y0 = 0.0;
  }
  if (source_x1 > source_view->Get_Width()) {
    double ratio =
        (source_x1 - source_view->Get_Width()) / (source_x1 - source_x0);
    dest_x1 -= (dest_x1 - dest_x0) * ratio;
    source_x1 = (double)source_view->Get_Width();
  }
  if (source_y1 > source_view->Get_Height()) {
    double ratio =
        (source_y1 - source_view->Get_Height()) / (source_y1 - source_y0);
    dest_y1 -= (dest_y1 - dest_y0) * ratio;
    source_y1 = (double)source_view->Get_Height();
  }
  if (dest_x0 < 0.0) {
    double ratio = (0.0 - dest_x0) / (dest_x1 - dest_x0);
    source_x0 += (source_x1 - source_x0) * ratio;
    dest_x0 = 0.0;
  }
  if (dest_y0 < 0.0) {
    double ratio = (0.0 - dest_y0) / (dest_y1 - dest_y0);
    source_y0 += (source_y1 - source_y0) * ratio;
    dest_y0 = 0.0;
  }
  if (dest_x1 > dest_view->Get_Width()) {
    double ratio = (dest_x1 - dest_view->Get_Width()) / (dest_x1 - dest_x0);
    source_x1 -= (source_x1 - source_x0) * ratio;
    dest_x1 = (double)dest_view->Get_Width();
  }
  if (dest_y1 > dest_view->Get_Height()) {
    double ratio = (dest_y1 - dest_view->Get_Height()) / (dest_y1 - dest_y0);
    source_y1 -= (source_y1 - source_y0) * ratio;
    dest_y1 = (double)dest_view->Get_Height();
  }

  clipped_dest_width = dest_x1 - dest_x0;
  clipped_dest_height = dest_y1 - dest_y0;
  clipped_source_width = source_x1 - source_x0;
  clipped_source_height = source_y1 - source_y0;
  if (clipped_dest_width <= 0.0 || clipped_dest_height <= 0.0 ||
      clipped_source_width <= 0.0 || clipped_source_height <= 0.0) {
    return (FALSE);
  }

  start_x = (int32_t)dest_x0;
  start_y = (int32_t)dest_y0;
  end_x = (int32_t)dest_x1;
  end_y = (int32_t)dest_y1;
  if (start_x < 0)
    start_x = 0;
  if (start_y < 0)
    start_y = 0;
  if (end_x > dest_view->Get_Width())
    end_x = dest_view->Get_Width();
  if (end_y > dest_view->Get_Height())
    end_y = dest_view->Get_Height();
  if (end_x <= start_x || end_y <= start_y) {
    return (FALSE);
  }

  source_data = View_Data(source_view);
  dest_data = View_Data(dest_view);
  source_row_bytes = View_Row_Bytes(source_view);
  dest_row_bytes = View_Row_Bytes(dest_view);

  for (int32_t y = start_y; y < end_y; ++y) {
    double source_row_position =
        source_y0 +
        ((double)(y - dest_y0) * clipped_source_height / clipped_dest_height);
    int32_t source_row_index = (int32_t)source_row_position;
    uint8_t const *source_row;
    uint8_t *dest_row;

    if (source_row_index < 0) {
      source_row_index = 0;
    }
    if (source_row_index >= source_view->Get_Height()) {
      source_row_index = source_view->Get_Height() - 1;
    }

    source_row = source_data + source_row_index * source_row_bytes;
    dest_row = dest_data + y * dest_row_bytes;
    for (int32_t x = start_x; x < end_x; ++x) {
      double source_column_position =
          source_x0 +
          ((double)(x - dest_x0) * clipped_source_width / clipped_dest_width);
      int32_t source_column_index = (int32_t)source_column_position;
      uint8_t value;

      if (source_column_index < 0) {
        source_column_index = 0;
      }
      if (source_column_index >= source_view->Get_Width()) {
        source_column_index = source_view->Get_Width() - 1;
      }

      value = source_row[source_column_index];
      if (remap) {
        value = ((uint8_t *)remap)[value];
      }
      if (!trans || value != 0) {
        dest_row[x] = value;
      }
    }
  }

  return (TRUE);
}

int32_t Buffer_Print(void *thisptr, const char *str, int32_t x,                                int32_t y, int32_t fcolor, int32_t bcolor) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  uint8_t const *font;
  uint8_t const *info_block;
  uint8_t const *width_block;
  uint8_t const *height_block;
  uint8_t const *offset_block;
  uint8_t *buffer;
  uint8_t *current_draw;
  uint8_t *current_line;
  int32_t row_bytes;
  int32_t max_height;
  int32_t current_x;
  int32_t current_y;
  int32_t original_x;

  if (!view || !str || !FontPtr) {
    return (0);
  }

  font = (uint8_t const *)FontPtr;
  info_block = font + Read_Font_Word(font + FONTINFOBLOCK);
  width_block = font + Read_Font_Word(font + FONTWIDTHBLOCK);
  height_block = font + Read_Font_Word(font + FONTHEIGHTBLOCK);
  offset_block = font + Read_Font_Word(font + FONTOFFSETBLOCK);
  max_height = info_block[FONTINFOMAXHEIGHT];
  if (y + max_height > view->Get_Height()) {
    return (0);
  }

  Ensure_Font_Palette();
  ColorXlat[1] = (uint8_t)fcolor;
  ColorXlat[16] = (uint8_t)fcolor;
  ColorXlat[0] = (uint8_t)bcolor;

  buffer = View_Data(view);
  row_bytes = View_Row_Bytes(view);
  current_x = x;
  current_y = y;
  original_x = x;
  current_line = buffer + current_y * row_bytes;
  current_draw = current_line + current_x;

  while (*str) {
    uint8_t ch = (uint8_t)*str++;

    if (ch == '\n' || ch == '\r') {
      int32_t next_y = current_y + max_height + FontYSpacing;
      if (next_y + max_height > view->Get_Height()) {
        return (0);
      }
      current_y = next_y;
      current_line = buffer + current_y * row_bytes;
      current_x = (ch == '\n') ? 0 : original_x;
      current_draw = current_line + current_x;
      continue;
    }

    {
      int32_t width = width_block[ch];
      int32_t advance = width + FontXSpacing;
      uint8_t const *data_ptr;
      uint8_t const *char_height_ptr;
      uint8_t top_blank;
      uint8_t char_height;
      int32_t bottom_blank;
      uint8_t *draw_ptr;

      if (current_x + advance > view->Get_Width()) {
        int32_t next_y = current_y + max_height + FontYSpacing;
        if (next_y + max_height > view->Get_Height()) {
          return (0);
        }
        current_y = next_y;
        current_line = buffer + current_y * row_bytes;
        current_x = 0;
        current_draw = current_line;
      }

      data_ptr = font + Read_Font_Word(offset_block + ch * 2);
      char_height_ptr = height_block + ch * 2;
      top_blank = char_height_ptr[0];
      char_height = char_height_ptr[1];
      bottom_blank = max_height - top_blank - char_height;
      draw_ptr = current_draw;

      if (ColorXlat[0] != 0) {
        for (int32_t row = 0; row < top_blank; ++row) {
          memset(draw_ptr, ColorXlat[0], width);
          draw_ptr += row_bytes;
        }
      } else {
        draw_ptr += top_blank * row_bytes;
      }

      for (int32_t row = 0; row < char_height; ++row) {
        uint8_t *pixel_ptr = draw_ptr;
        int32_t column = 0;
        while (column < width) {
          uint8_t pair = *data_ptr++;
          uint8_t lo = ColorXlat[pair & 0x0F];
          if (lo != 0) {
            *pixel_ptr = lo;
          }
          ++pixel_ptr;
          ++column;
          if (column < width) {
            uint8_t hi = ColorXlat[(pair >> 4) & 0x0F];
            if (hi != 0) {
              *pixel_ptr = hi;
            }
            ++pixel_ptr;
            ++column;
          }
        }
        draw_ptr += row_bytes;
      }

      if (ColorXlat[0] != 0) {
        for (int32_t row = 0; row < bottom_blank; ++row) {
          memset(draw_ptr, ColorXlat[0], width);
          draw_ptr += row_bytes;
        }
      }

      current_x += advance;
      current_draw += advance;
    }
  }

  return ((int32_t)(current_draw - buffer));
}

void Buffer_Draw_Stamp(void const *thisptr, void const *icondata,                                  int32_t icon, int32_t x_pixel,
                                  int32_t y_pixel, void const *remap) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;

  if (!view) {
    return;
  }

  Buffer_Draw_Stamp_Clip(thisptr, icondata, icon, x_pixel, y_pixel, remap, 0, 0,
                         view->Get_Width(), view->Get_Height());
}

void Buffer_Draw_Stamp_Clip(void const *thisptr,                                       void const *icondata, int32_t icon,
                                       int32_t x_pixel, int32_t y_pixel,
                                       void const *remap, int32_t min_x,
                                       int32_t min_y, int32_t max_x,
                                       int32_t max_y) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  IControl_Type const *control = (IControl_Type const *)icondata;
  uint8_t const *icon_base;
  uint8_t const *trans_table;
  uint8_t *dest_base;
  int32_t draw_icon;
  int32_t icon_width;
  int32_t icon_height;
  int32_t icon_size;
  int32_t clip_x0;
  int32_t clip_y0;
  int32_t clip_x1;
  int32_t clip_y1;
  int32_t dest_x;
  int32_t dest_y;
  int32_t source_x = 0;
  int32_t source_y = 0;
  int32_t draw_width;
  int32_t draw_height;
  int32_t row_bytes;
  int32_t transparent;

  if (!view || !control) {
    return;
  }

  draw_icon = Resolve_Icon(control, icon);
  if (draw_icon < 0) {
    return;
  }

  icon_width = control->Width;
  icon_height = control->Height;
  icon_size = icon_width * icon_height;
  icon_base =
      ((uint8_t const *)control) + control->Icons + draw_icon * icon_size;
  trans_table = control->TransFlag
                    ? (((uint8_t const *)control) + control->TransFlag)
                    : NULL;
  transparent = trans_table ? (trans_table[draw_icon] != 0) : TRUE;

  clip_x0 = min_x;
  clip_y0 = min_y;
  clip_x1 = min_x + max_x;
  clip_y1 = min_y + max_y;
  dest_x = x_pixel + min_x;
  dest_y = y_pixel + min_y;
  draw_width = icon_width;
  draw_height = icon_height;

  if (dest_x >= clip_x1 || dest_y >= clip_y1 ||
      dest_x + draw_width <= clip_x0 || dest_y + draw_height <= clip_y0) {
    return;
  }

  if (dest_x < clip_x0) {
    source_x += clip_x0 - dest_x;
    draw_width -= clip_x0 - dest_x;
    dest_x = clip_x0;
  }
  if (dest_y < clip_y0) {
    source_y += clip_y0 - dest_y;
    draw_height -= clip_y0 - dest_y;
    dest_y = clip_y0;
  }
  if (dest_x + draw_width > clip_x1) {
    draw_width = clip_x1 - dest_x;
  }
  if (dest_y + draw_height > clip_y1) {
    draw_height = clip_y1 - dest_y;
  }

  if (dest_x < 0) {
    source_x += -dest_x;
    draw_width += dest_x;
    dest_x = 0;
  }
  if (dest_y < 0) {
    source_y += -dest_y;
    draw_height += dest_y;
    dest_y = 0;
  }
  if (dest_x + draw_width > view->Get_Width()) {
    draw_width = view->Get_Width() - dest_x;
  }
  if (dest_y + draw_height > view->Get_Height()) {
    draw_height = view->Get_Height() - dest_y;
  }
  if (draw_width <= 0 || draw_height <= 0) {
    return;
  }

  row_bytes = View_Row_Bytes(view);
  dest_base = View_Data(view) + dest_y * row_bytes + dest_x;
  for (int32_t row = 0; row < draw_height; ++row) {
    uint8_t const *src_row =
        icon_base + (source_y + row) * icon_width + source_x;
    uint8_t *dst_row = dest_base + row * row_bytes;

    if (remap) {
      uint8_t const *remap_table = (uint8_t const *)remap;
      for (int32_t column = 0; column < draw_width; ++column) {
        uint8_t value = remap_table[src_row[column]];
        if (value != 0) {
          dst_row[column] = value;
        }
      }
    } else if (!transparent) {
      memcpy(dst_row, src_row, draw_width);
    } else {
      for (int32_t column = 0; column < draw_width; ++column) {
        uint8_t value = src_row[column];
        if (value != 0) {
          dst_row[column] = value;
        }
      }
    }
  }
}

void Buffer_Draw_Line(void *thisptr, int32_t sx, int32_t sy,                                 int32_t dx, int32_t dy, uint8_t color) {
  GraphicViewPortClass *view = (GraphicViewPortClass *)thisptr;
  int32_t delta_x;
  int32_t delta_y;
  int32_t step_x;
  int32_t step_y;
  int32_t error;

  if (!view) {
    return;
  }

  delta_x = ABS(dx - sx);
  step_x = (sx < dx) ? 1 : -1;
  delta_y = -ABS(dy - sy);
  step_y = (sy < dy) ? 1 : -1;
  error = delta_x + delta_y;

  for (;;) {
    if (sx >= 0 && sy >= 0 && sx < view->Get_Width() &&
        sy < view->Get_Height()) {
      View_Data(view)[sy * View_Row_Bytes(view) + sx] = color;
    }
    if (sx == dx && sy == dy) {
      break;
    }

    {
      int32_t twice_error = error + error;
      if (twice_error >= delta_y) {
        error += delta_y;
        sx += step_x;
      }
      if (twice_error <= delta_x) {
        error += delta_x;
        sy += step_y;
      }
    }
  }
}

int32_t Is_Icon_Cached(void const *, int32_t) {
  return (-1);
}

void Cache_Copy_Icon(void const *icon_ptr, void *surface,                                int32_t pitch) {
  uint8_t const *source = (uint8_t const *)icon_ptr;
  uint8_t *dest = (uint8_t *)surface;

  if (!source || !dest || pitch < 24) {
    return;
  }

  for (int32_t row = 0; row < 24; ++row) {
    memcpy(dest, source, 24);
    source += 24;
    dest += pitch;
  }
}
