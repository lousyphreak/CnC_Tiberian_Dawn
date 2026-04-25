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

int32_t Desired_Facing8(int32_t x1, int32_t y1, int32_t x2, int32_t y2)
{
	int index = 0;

	int xdiff = x2 - x1;
	if (xdiff < 0) {
		index |= 0x00C0;
		xdiff = -xdiff;
	}

	int ydiff = y1 - y2;
	if (ydiff < 0) {
		index ^= 0x0040;
		ydiff = -ydiff;
	}

	unsigned bigger;
	unsigned smaller;
	if (xdiff < ydiff) {
		smaller = static_cast<unsigned>(xdiff);
		bigger = static_cast<unsigned>(ydiff);
	} else {
		smaller = static_cast<unsigned>(ydiff);
		bigger = static_cast<unsigned>(xdiff);
	}

	if (((bigger + 1U) / 2U) <= smaller) {
		index += 0x0020;
		return index;
	}

	int adder = (index & 0x0040);
	if (static_cast<unsigned>(xdiff) == bigger) {
		adder ^= 0x0040;
	}
	index += adder;

	return index;
}

int32_t Desired_Facing256(int32_t srcx, int32_t srcy, int32_t dstx, int32_t dsty)
{
	int composite = 0;

	int xdiff = dstx - srcx;
	if (xdiff < 0) {
		composite |= 0x00C0;
		xdiff = -xdiff;
	}

	int ydiff = srcy - dsty;
	if (ydiff < 0) {
		composite ^= 0x0040;
		ydiff = -ydiff;
	}

	if (xdiff == 0 && ydiff == 0) {
		return 0xFF;
	}

	unsigned bigger;
	unsigned smaller;
	if (xdiff < ydiff) {
		smaller = static_cast<unsigned>(xdiff);
		bigger = static_cast<unsigned>(ydiff);
	} else {
		smaller = static_cast<unsigned>(ydiff);
		bigger = static_cast<unsigned>(xdiff);
	}

	int frac = static_cast<int>((smaller * 32U) / bigger);
	int adder = (composite & 0x0040);
	if (xdiff > ydiff) {
		adder ^= 0x0040;
	}
	if (adder) {
		frac = (adder - frac) - 1;
	}

	composite += frac;
	return composite;
}
