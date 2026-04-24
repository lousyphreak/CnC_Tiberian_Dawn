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

#ifndef VQAUNVQ_H
#define VQAUNVQ_H
/****************************************************************************
*
*         C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* PROJECT
*     VQAPlay32 library. (32-Bit protected mode)
*
* FILE
*     unvq.h
*
* DESCRIPTION
*     VQ frame decompress definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     Feburary 8, 1995
*
****************************************************************************/
#include <stdint.h>
#ifdef PHARLAP_TNT
#include <pltypes.h>
#endif

/*---------------------------------------------------------------------------
 * FUNCTION PROTOTYPES
 *-------------------------------------------------------------------------*/


/* unvqbuff.asm */
#ifndef PHARLAP_TNT
void  UnVQ_2x2(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

void  UnVQ_2x3(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

void  UnVQ_4x2(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

void  UnVQ_4x4(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

void  UnVQ_4x4_V2(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

void  UnVQ_4x2_Woofer(uint8_t *codebook, uint8_t *pointers,
		uint8_t *buffer, uint32_t blocksperrow,
		uint32_t numrows, uint32_t bufwidth);

#else /* PHARLAP_TNT */

void  UnVQ_2x2(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);

void  UnVQ_2x3(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);

void  UnVQ_4x2(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);

void  UnVQ_4x4(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);

void  UnVQ_4x4_V2(uint8_t *codebook, uint8_t *pointers,
		char * buffer, uint32_t blocksperrow, uint32_t numrows,
		uint32_t bufwidth);

#endif /* PHARLAP_TNT */

#endif /* VQAUNVQ_H */
