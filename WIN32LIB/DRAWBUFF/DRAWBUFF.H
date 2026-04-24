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


#ifndef DRAWBUFF_H
#define DRAWBUFF_H


#ifndef WWSTD_H
#include "WWSTD.H"
#endif

class GraphicViewPortClass;
class GraphicBufferClass;
/*=========================================================================*/
/* Define functions which have not under-gone name mangling						*/
/*=========================================================================*/

/*======================================================================*/
/* Externs for all of the common functions between the video buffer		*/
/*		class and the graphic buffer class.											*/
/*======================================================================*/
int32_t  Buffer_Size_Of_Region(void *thisptr, int32_t w, int32_t h);

void	 Buffer_Put_Pixel(void * thisptr, int32_t x, int32_t y, uint8_t color);
int32_t	 Buffer_Get_Pixel(void * thisptr, int32_t x, int32_t y);
void	 Buffer_Clear(void *thisptr, uint8_t color);
int32_t	 Buffer_To_Buffer(void *thisptr, int32_t x, int32_t y, int32_t w, int32_t h, void *buff, int32_t size);
int32_t  Buffer_To_Page(int32_t x, int32_t y, int32_t w, int32_t h, void *Buffer, void *view);
int32_t	 Linear_Blit_To_Linear(	void *thisptr, void * dest, int32_t x_pixel, int32_t y_pixel, int32_t dx_pixel,
											int32_t dy_pixel, int32_t pixel_width, int32_t pixel_height, int32_t trans);
int32_t  Linear_Scale_To_Linear(	void *, void *, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, char *);

int32_t  Buffer_Print(void *thisptr, const char *str, int32_t x, int32_t y, int32_t fcolor, int32_t bcolor);

/*======================================================================*/
/* Externs for all of the graphic buffer class only functions				*/
/*======================================================================*/
void  Buffer_Draw_Line(void *thisptr, int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color);
void  Buffer_Fill_Rect(void *thisptr, int32_t sx, int32_t sy, int32_t dx, int32_t dy, uint8_t color);
void  Buffer_Remap(void * thisptr, int32_t sx, int32_t sy, int32_t width, int32_t height, void *remap);
void  Buffer_Fill_Quad(void * thisptr, void *span_buff, int32_t x0, int32_t y0, int32_t x1, int32_t y1,
						int32_t x2, int32_t y2, int32_t x3, int32_t y3, int32_t color);
void  Buffer_Draw_Stamp(void const *thisptr, void const *icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const *remap);
void  Buffer_Draw_Stamp_Clip(void const *thisptr, void const *icondata, int32_t icon, int32_t x_pixel, int32_t y_pixel, void const *remap, int32_t ,int32_t,int32_t,int32_t);
void *  Get_Font_Palette_Ptr ( void );

extern GraphicViewPortClass *LogicPage;
#endif
