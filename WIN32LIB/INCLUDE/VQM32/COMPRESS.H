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

#ifndef VQMCOMP_H
#include <stdint.h>
#define VQMCOMP_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     compress.h
*
* DESCRIPTION
*     Compression definitions. (32-Bit protected mode)
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     January 26, 1995
*
 ****************************************************************************/

/* Prototypes */


uint32_t LCW_Compress(char const *source, char *dest,
		uint32_t length);
uint32_t LCW_Uncompress(void const *source, void *dest,
		uint32_t length);
int32_t AudioZap(void *source, void *dest, int32_t size);
int32_t AudioUnzap(void *source, void *dest, int32_t);


#endif /* VQMCOMP_H */
