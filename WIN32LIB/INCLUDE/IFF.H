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

/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Part of the FILEIO Library               *
 *                                                                         *
 *                    File Name : IFF.H                                    *
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : April 20, 1994                           *
 *                                                                         *
 *                  Last Update : April 20, 1994   [SKB]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef IFF_H
#define IFF_H

#include <cstdint>

#ifndef GBUFFER_H
#include <GBUFFER.H>
#endif

#ifndef MISC_H
#include <MISC.H>
// This is needed fro Reverse_WORD and _LONG
#endif

#ifndef MEMFLAGS_H
#include <MEMFLAG.H>
// This is needed for MemoryFlagType.
#endif

#define LZW_SUPPORTED			FALSE

/*=========================================================================*/
/* Iff and Load Picture system defines and enumerations							*/
/*=========================================================================*/

#define 	MAKE_ID(a,b,c,d)			((int32_t) ((int32_t) d << 24) | ((int32_t) c << 16) | ((int32_t) b <<  8) | (int32_t)(a))
#define	IFFize_WORD(a)			Reverse_Word(a)
#define	IFFize_LONG(a)			Reverse_Long(a)


//lint -strong(AJX,PicturePlaneType)
typedef enum {
	BM_AMIGA,	// Bit plane format (8K per bitplane).
	BM_MCGA,		// Byte per pixel format (64K).

	BM_DEFAULT=BM_MCGA	// Default picture format.
} PicturePlaneType;

/*
**	This is the compression type code.  This value is used in the compressed
**	file header to indicate the method of compression used.  Note that the
**	LZW method may not be supported.
*/
//lint -strong(AJX,CompressionType)
typedef enum {
	NOCOMPRESS,		// No compression (raw data).
	LZW12,			// LZW 12 bit codes.
	LZW14,			// LZW 14 bit codes.
	HORIZONTAL,		// Run length encoding (RLE).
	LCW				// Westwood proprietary compression.
} CompressionType;

/*
**	Compressed blocks of data must start with this header structure.
**	Note that disk based compressed files have an additional two
**	leading bytes that indicate the size of the entire file.
*/
//lint -strong(AJX,CompHeaderType)
#pragma pack(push, 1)
typedef struct {
	uint8_t		Method;		// Compression method (CompressionType).
	uint8_t		pad;			// Reserved pad byte (always 0).
	uint32_t	Size;			// Size of the uncompressed data.
	uint16_t	Skip;			// Number of bytes to skip before data.
} CompHeaderType;
#pragma pack(pop)
static_assert(sizeof(CompHeaderType) == 8, "CompHeaderType must stay 8 bytes");


/*=========================================================================*/
/* The following prototypes are for the file: IFF.CPP								*/
/*=========================================================================*/

int32_t  Open_Iff_File(char const *filename);
void  Close_Iff_File(int32_t fh);
uint32_t  Get_Iff_Chunk_Size(int32_t fh, int32_t id);
uint32_t  Read_Iff_Chunk(int32_t fh, int32_t id, void *buffer, uint32_t maxsize);
void  Write_Iff_Chunk(int32_t file, int32_t id, void *buffer, int32_t length);


/*=========================================================================*/
/* The following prototypes are for the file: LOADPICT.CPP						*/
/*=========================================================================*/

int32_t  Load_Picture(char const *filename, BufferClass& scratchbuf, BufferClass& destbuf, uint8_t *palette=NULL, PicturePlaneType format=BM_DEFAULT);


/*=========================================================================*/
/* The following prototypes are for the file: LOAD.CPP							*/
/*=========================================================================*/

uint32_t  Load_Data(char const *name, void *ptr, uint32_t size);
uint32_t  Write_Data(char const *name, void *ptr, uint32_t size);
void *  Load_Alloc_Data(char const *name, MemoryFlagType flags);
uint32_t  Load_Uncompress(char const *file, BufferClass& uncomp_buff, BufferClass& dest_buff, void *reserved_data=NULL);
uint32_t  Uncompress_Data(void const *src, void *dst);
void  Set_Uncomp_Buffer(int32_t buffer_segment, int32_t size_of_buffer);

/*=========================================================================*/
/* The following prototypes are for the file: WRITELBM.CPP						*/
/*=========================================================================*/

PUBLIC int32_t Write_LBM_File(int32_t lbmhandle, BufferClass& buff, int32_t bitplanes, uint8_t *palette);



/*========================= Assembly Functions ============================*/


/*=========================================================================*/
/* The following prototypes are for the file: PACK2PLN.ASM						*/
/*=========================================================================*/

extern void  Pack_2_Plane(void *buffer, void * pageptr, int32_t planebit);

/*=========================================================================*/
/* The following prototypes are for the file: LCWCOMP.ASM						*/
/*=========================================================================*/

extern uint32_t  LCW_Compress(void *source, void *dest, uint32_t length);

/*=========================================================================*/
/* The following prototypes are for the file: LCWUNCMP.ASM						*/
/*=========================================================================*/

extern uint32_t  LCW_Uncompress(void const *source, void *dest, uint32_t length);

/*=========================================================================*/



#endif //IFF_H
