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

#include "MISC.H"
#include <stdint.h>
#include <string.h>

uint32_t RandNumb = 0x12349876u;


static int32_t Signed_Shift_Right_8(int32_t value)
{
if (value >= 0) {
return(value >> 8);
}

return(-(((-value) + 0xFF) >> 8));
}


uint8_t  Random(void)
{
uint8_t byte0 = (uint8_t)(RandNumb & 0xFFu);
uint8_t byte1 = (uint8_t)((RandNumb >> 8) & 0xFFu);
uint8_t byte2 = (uint8_t)((RandNumb >> 16) & 0xFFu);
uint8_t byte3 = (uint8_t)((RandNumb >> 24) & 0xFFu);
uint8_t al = (uint8_t)(byte0 >> 2);
uint32_t carry = (byte0 >> 1) & 1u;
uint32_t next_carry = (byte2 >> 7) & 1u;

byte2 = (uint8_t)((byte2 << 1) | carry);
carry = next_carry;
next_carry = (byte1 >> 7) & 1u;
byte1 = (uint8_t)((byte1 << 1) | carry);
carry = next_carry ^ 1u;

{
int32_t tmp = (int32_t)al - (int32_t)byte0 - (int32_t)carry;
al = (uint8_t)tmp;
carry = (tmp < 0) ? 1u : 0u;
}

carry = al & 1u;
al >>= 1;
byte0 = (uint8_t)((byte0 >> 1) | (carry << 7));
al = (uint8_t)(byte0 ^ byte1);

RandNumb = ((uint32_t)byte3 << 24) | ((uint32_t)byte2 << 16) | ((uint32_t)byte1 << 8) | (uint32_t)byte0;
return(al);
}


int32_t  Get_Random_Mask(int32_t maxval)
{
uint32_t value;
uint32_t mask;

if (maxval <= 0) {
return(1);
}

value = (uint32_t)maxval;
mask = 1;
while (value >>= 1) {
mask = (mask << 1) | 1u;
}

return((int32_t)mask);
}


void *  Build_Fading_Table(void const *palette, void const *dest, int32_t color, int32_t frac)
{
uint8_t const *source;
uint8_t *table;
int32_t target;
int32_t fraction;

if (!palette || !dest) {
return((void *)dest);
}

source = (uint8_t const *)palette;
table = (uint8_t *)dest;
target = (int32_t)color;
fraction = (int32_t)frac;

if (target < 0) {
target = 0;
}
if (target >= 256) {
target = 255;
}
if (fraction < 0) {
fraction = 0;
}
if (fraction > 255) {
fraction = 255;
}

table[0] = 0;

for (int32_t source_index = 1; source_index < 256; ++source_index) {
int32_t best_color = target;
	int32_t best_value = INT32_MAX;
int32_t base_offset = source_index * 3;
int32_t target_offset = target * 3;
int32_t ideal_red = (int32_t)source[base_offset + 0] - Signed_Shift_Right_8(((int32_t)source[base_offset + 0] - (int32_t)source[target_offset + 0]) * fraction);
int32_t ideal_green = (int32_t)source[base_offset + 1] - Signed_Shift_Right_8(((int32_t)source[base_offset + 1] - (int32_t)source[target_offset + 1]) * fraction);
int32_t ideal_blue = (int32_t)source[base_offset + 2] - Signed_Shift_Right_8(((int32_t)source[base_offset + 2] - (int32_t)source[target_offset + 2]) * fraction);

for (int32_t candidate = 1; candidate < 256; ++candidate) {
int32_t match_value;
int32_t candidate_offset;
int32_t red_diff;
int32_t green_diff;
int32_t blue_diff;

if (candidate == source_index) {
continue;
}

candidate_offset = candidate * 3;
red_diff = (int32_t)source[candidate_offset + 0] - ideal_red;
green_diff = (int32_t)source[candidate_offset + 1] - ideal_green;
blue_diff = (int32_t)source[candidate_offset + 2] - ideal_blue;
match_value = (int32_t)red_diff * red_diff;
match_value += (int32_t)green_diff * green_diff;
match_value += (int32_t)blue_diff * blue_diff;

if (match_value <= best_value) {
best_value = match_value;
best_color = candidate;
if (match_value == 0) {
break;
}
}
}

table[source_index] = (uint8_t)best_color;
}

return((void *)dest);
}


int32_t  Clip_Rect(int32_t *x, int32_t *y, int32_t *dw, int32_t *dh, int32_t width, int32_t height)
{
int32_t x0;
int32_t y0;
int32_t x1;
int32_t y1;
uint8_t code0 = 0;
uint8_t code1 = 0;

if (!x || !y || !dw || !dh) {
return(-1);
}

x0 = *x;
y0 = *y;
x1 = x0 + *dw;
y1 = y0 + *dh;

if (x0 < 0) {
code0 |= 0x08;
}
if (x0 > width) {
code0 |= 0x04;
}
if (y0 < 0) {
code0 |= 0x02;
}
if (y0 > height) {
code0 |= 0x01;
}

if (x1 < 0) {
code1 |= 0x08;
}
if (x1 > width) {
code1 |= 0x04;
}
if (y1 < 0) {
code1 |= 0x02;
}
if (y1 > height) {
code1 |= 0x01;
}

if (code0 & code1) {
return(-1);
}

if ((code0 | code1) == 0) {
return(0);
}

if (code0 & 0x08) {
*dw += *x;
*x = 0;
}
if (code0 & 0x02) {
*dh += *y;
*y = 0;
}
if (code1 & 0x04) {
*dw = width - *x;
if (*dw <= 0) {
return(-1);
}
}
if (code1 & 0x01) {
*dh = height - *y;
if (*dh <= 0) {
return(-1);
}
}

return(1);
}


int32_t  Confine_Rect(int32_t *x, int32_t *y, int32_t dw, int32_t dh, int32_t width, int32_t height)
{
	int32_t changed = 0;

if (!x || !y) {
return(0);
}

if (*x < 0) {
*x = 0;
changed = 1;
} else if ((*x + dw) > width) {
*x = width - dw;
changed = 1;
}

if (*y < 0) {
*y = 0;
changed = 1;
} else if ((*y + dh) > height) {
*y = height - dh;
changed = 1;
}

	return(changed);
}


void  Force_VM_Page_In(void *buffer, int32_t length){
	volatile uint8_t sink = 0;
	volatile uint8_t const *bytes;

	if (!buffer || length <= 0) {
		return;
	}

	bytes = (volatile uint8_t const *)buffer;
	for (int32_t offset = 0; offset < length; offset += 4096) {
		sink ^= bytes[offset];
	}
	sink ^= bytes[length - 1];
}


int32_t  Reverse_Long(int32_t number)
{
	uint32_t value = (uint32_t)number;

	value = ((value & 0x000000FFu) << 24) |
		((value & 0x0000FF00u) << 8) |
		((value & 0x00FF0000u) >> 8) |
		((value & 0xFF000000u) >> 24);
	return((int32_t)value);
}
