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

/* $Header: g:/library/wwlib32/file/rcs/writelbm.cpp 1.1 1994/04/20 14:38:57 scott_bowen Exp $ */
/***************************************************************************
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Write LBM File                           *
 *                                                                         *
 *                    File Name : writelbm.c                               *
 *                                                                         *
 *                   Programmer : Scott Bowen                              *
 *                                                                         *
 *                   Start Date :  November 18, 1991                       *
 *                                                                         *
 *                  Last Update : November 19, 1991   [SB]                 *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Get_Line -- convert one plane of one row to a packed plane            *
 *   Write_BMHD -- writes out the bit map header (LocalHeader)             *
 *   Write_Body -- writes out compressed data in an LBM file               *
 *   Write_CMAP -- Writes out CMAP (palette) information                   *
 *   Write_LBM_File -- Writes out a file in LBM format                     *
 *   Write_Row -- compresses and writes a row plane to .lbm file           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */


// At the end of this file there is an IFF definition for a .LBM file.

#include "IFF.H"
#include "FILE.H"
#include <WWSTD.H>
#include <stdio.h>
#include <string.h>


	// A BitMapHeader is stored in a BMHD chunk.  This structure MUST be an even size
typedef struct {
    uint16_t w, h;							// raster width & height in pixels 
    uint16_t x, y;							// position for this image
    uint8_t planes;							// # source bitplanes 
    uint8_t masking;							// masking technique 
    uint8_t compression;					// compression algoithm
    uint8_t pad1;								// UNUSED.  For consistency, put 0 here.
    uint16_t transcolor;					// transparent "color number"
    uint8_t xaspect, yaspect;				// aspect ratio, a rational number x/y
    uint16_t pagewidth, pageheight;		// source "page" size in pixels
} BitMapHeaderType;


// All values in LocalHeader are always the same except planes.  This is set in Write_BMHD
// the short values must be in low-high order for compatibility.

PRIVATE BitMapHeaderType LocalHeader = {
	0x4001, 0xc800, 0, 0, 0, 0,		// width, height, x, y, planes, mask
	1,   0, 0xFF00, 5, 6, 				// compress, pad1, transcolor, xasptect, yaspect
	0x4001,	0xC800 };					// pagewidth, pageheight

	
	// Used to verify that the write of the header was valid
#define BM_HEADER_SIZE		(((sizeof(BitMapHeaderType) + 1) & 0xFFFE) + 8L)


/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

PRIVATE int32_t Write_BMHD(int32_t lbmhandle, int32_t bitplanes);
PRIVATE int32_t Write_CMAP(int32_t lbmhandle, uint8_t * palette, int32_t bitplanes);
PRIVATE int32_t Write_BODY(int32_t lbmhandle, BufferClass& buff, int32_t bitplanes);
PRIVATE int32_t Write_Row(int32_t lbmhandle, uint8_t *buffer);


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * WRITE_LBM_FILE -- Writes out a file in LBM format                       *
 *                                                                         *
 * INPUT:  int lbmhandle -- lbm file handle already opened by caller      *
 *         BufferClass buff -- buff where MCGA picture is                     *
 *         int bitplane -- number of bitplanes to convert to              *
 *         char *palette -- pointer to palette for buff                    *
 *                                                                         *
 * OUTPUT: Returns BOOL -- successfull or not                              *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/18/1991  SB : Created.                                             *
 *=========================================================================*/

PUBLIC int32_t Write_LBM_File(int32_t lbmhandle, BufferClass& buff, int32_t bitplanes, uint8_t *palette)
{
	int32_t filesize;


	Seek_File(lbmhandle, 0L, SEEK_SET);						// goto beginning of file 
																
	Write_File(lbmhandle, "FORM????ILBM", 12L);			// First 12 bytes of all .lbm files 
																		// size is unkown so write ????
	filesize = 12L; 												// 4 bytes for "ILBM" 

	filesize += Write_BMHD(lbmhandle, bitplanes);		// write out BMHD (fixed size)
	filesize += Write_CMAP(lbmhandle, palette, bitplanes);	// write out CMAP 

		// Write out the body, or compressed picture image.  This size will depend
		// on the compression, but the value passed back is what the compressor
		// assumed was written to file

	filesize += Write_BODY(lbmhandle, buff, bitplanes);

		// Verify that we were able to write out the file without running out of space
	if (Seek_File(lbmhandle, 0L, SEEK_END) != static_cast<uint32_t>(filesize)) {
		return(FALSE);
	}

	Seek_File(lbmhandle, 4L, SEEK_SET);						// goto beginning of file 
	filesize = Reverse_Long(filesize - 8L);				// - 8 because of "FORM" + WORD (size)
	Write_File(lbmhandle, (char *) &filesize, 4L);		// patch in filesize 

	return(TRUE);
}


/***************************************************************************
 * WRITE_BMHD -- writes out the bit map header (LocalHeader)               *
 *                                                                         *
 * INPUT:  int lbmhandle -- file handle for lbm file                      *
 *         int pitplanes -- number of bitplanes to write out              *
 *                                                                         *
 * OUTPUT: long number of bytes hopefully written out to .LBM file         *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/19/1991  SB : Created.                                             *
 *=========================================================================*/
PRIVATE int32_t Write_BMHD(int32_t lbmhandle, int32_t bitplanes)
{
	int32_t size;
	
	Write_File(lbmhandle, "BMHD", 4L);					// write out chunk title 
	size = Reverse_Long(sizeof(LocalHeader));			// write out size of LocalHeader chunk
	Write_File(lbmhandle, (char *) &size, 4L);

	LocalHeader.planes	= bitplanes;					// only nonconstant value in LocalHeader

		// Make sure size is even. Return 8 = "BMHD" + size of the bitmap header structure

	return(Write_File(lbmhandle, (char *) &LocalHeader,
	                 (sizeof(LocalHeader) + 1) & 0xFFFE) + 8L);
}


/***************************************************************************
 * WRITE_CMAP -- Writes out CMAP (palette) information                     *
 *                                                                         *
 *                                                                         *
 * INPUT:  int lbmhandle -- file handle of lbm file                       *
 *         char * palette -- pointer to paletter information       			*
 *         int bitplanes -- used to figure out size of palette            *
 *                                                                         *
 * OUTPUT: long number of bytes that should have been written out to .LBM. *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/19/1991  SB : Created.                                             *
 *=========================================================================*/

PRIVATE int32_t Write_CMAP(int32_t lbmhandle, uint8_t * palette, int32_t bitplanes)
{
	int32_t color, r, g, b, colors;
	int32_t size;
	uint8_t *pal_ptr;
	char rgb[3];


	Write_File(lbmhandle, "CMAP", 4L);						// write out palette info
	colors = 1 << bitplanes;									// colors = 2 to the bitplanes
	size = Reverse_Long(colors * 3L);						// size = colors * 3 guns

 	Write_File(lbmhandle, (char *) &size, 4L);

	for (pal_ptr = palette, color = 0; color < colors; color++) { // for each color

		if ((r = *pal_ptr++) != 0) {			// DPaint changes allows 0 - 100 for gun values		
			r = (r << 2) | 0x03;					// this must be converted to 0 - 256 for LBM
		}												// so LBM_val = (DP_val * 4) | 3 if DP_val != 0
		if ((g = *pal_ptr++) != 0) {			
			g = (g << 2) | 0x03;
		}
		if ((b = *pal_ptr++) != 0) {
			b = (b << 2) | 0x03;
		}
		rgb[0] = r;									// assign gun values to an array to write out
		rgb[1] = g;
		rgb[2] = b;
		
		Write_File(lbmhandle, rgb, 3L);
	}
														// size = colors * 3
	return(((colors << 1) + colors) + 8L);	// total size of CMAP 8 = "CMAP" + WORD (size)
}


/***************************************************************************
 * WRITE_BODY -- writes out compressed data in an LBM file                 *
 *                                                                         *
 * INPUT: int lbmhandle -- file handle of lbm file                        *
 *                                                                         *
 * OUTPUT: long - number of byte written                                   *
 *  	                                                                     *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/19/1991  SB : Created.                                             *
 *=========================================================================*/

PRIVATE int32_t Write_BODY(int32_t lbmhandle, BufferClass& buff, int32_t bitplanes)
{
	int32_t bodysize = 0;
	int32_t actualsize;
	int32_t size;
	int32_t planebit;
	int32_t line, plane;
	uint8_t buffer[40];
	uint8_t *buffptr;

	Write_File(lbmhandle, "BODY????", 8L);		// BODY chunk ID, ???? reserved for chuncksize 

	buffptr = (uint8_t *) buff.Get_Buffer();						// point to beginning of buff

	for (line = 0; line < 200; line++) {
		planebit = 1;											// start with bit 1 set 

		for (plane = 0; plane < bitplanes; plane++) {
			Pack_2_Plane(buffer, buffptr, planebit);	// convert to planar
			bodysize += Write_Row(lbmhandle, buffer); // write to to the BODY in the LBM

			planebit <<= 1;									// set next bit
		}

		buffptr += 320;										// row size is 320
	}

	actualsize = bodysize + (bodysize&0x01);

	if (actualsize != bodysize) {
		Write_File(lbmhandle, buffer, 1);		// Padd the block.
	}

	Seek_File(lbmhandle, -(actualsize + 4L), SEEK_CUR);		// Patch in chunksize
	size = Reverse_Long(bodysize);
	Write_File(lbmhandle, (char *) &size ,4L);			

	return(actualsize + 8L);		// total size of BODY,  "BODY????" = 8 bytes
}

/***************************************************************************
 * WRITE_ROW -- compresses and writes a row plane to .lbm file             *
 *                                                                         *
 * INPUT:  int lbmhandle -- lbm file handle                               *
 *         unsigned char *buffer -- pointer to buffer to be written out            *
 *                                                                         *
 * OUTPUT: long size of chunk that should have been written out            *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   11/19/1991  SB : Created.                                             *
 *=========================================================================*/
// this algorithm was taken from WILBM.c written by EA that was in the
// 1985 yearbook.  This is the compression method that DP.EXE uses.
// Change only if DP.EXE changes.

PRIVATE int32_t Write_Row(int32_t lbmhandle, uint8_t *buffer)
{
	int32_t i;
	int32_t chunksize = 0;
	int32_t dataLength = 40;	  					// 320 rows / 8 ( 1 plane per row)
	uint8_t repCode, current, curr_plus_2;
	uint8_t *buffptr;

	while (dataLength) {

  		// If at least 2 more bytes and they are equal, then replicate 

		if ((dataLength >= 2) && (buffer[0] == buffer[1])) {
			buffptr = buffer;
			for (i = 0; (i <= 128) &&  (i < (dataLength - 1)); i++) {
				if (*buffptr != buffptr[1]) {
					break;
				}
				buffptr++;
			}
			i++;
			repCode = -i + 1;
			Write_File(lbmhandle, &repCode, 1L);			// Write count as -count+1
			Write_File(lbmhandle, buffer,   1L);			// Write byte to replicate
			buffer += i;
			dataLength -= i;
			chunksize  += 2;

		}
		else { // Copy literally till 3 byte run or two 2 byte runs found 

			for (i = 0; (i <= 128) && (i < dataLength); i++) {
				current     = buffer[i];
				curr_plus_2 = buffer[i + 2];

				if (i == dataLength - 1)
					continue;
				if (current != buffer[i + 1])
					continue;
				if (i == dataLength - 2)
					continue;
				if (current == curr_plus_2)
					break;
				if (i == dataLength - 3)
					continue;
				if (curr_plus_2 == buffer[i + 3])
					 break;
			}
			repCode = i - 1;
			Write_File(lbmhandle, &repCode, 1L);			// Write count as count-1 
			Write_File(lbmhandle, buffer, (int32_t) i);		// Write 'count' bytes
			buffer += i;
			dataLength -= i;
			chunksize += i + 1;
		}
	}  // end while

	return(chunksize);
}


#if(FALSE)

This is a definition of a DPII .LBM file.
Below this definition are differences in DPIIe .LMB files.

Created by : Scott K. Bowen Nov 18, 1991
	
Start with .LBM to read definition :

.LBM		-> "FORM" + FILESIZE + "ILMB" + CHUNKS

BITPLANES	-> (word) // number of bit planes used
BLUE			-> (byte) // blue color gun value
BMHD 			-> "BMHD" + CHUNKSIZE + CONTEXT
BODY			->

CHUNKS		-> BMHD | BODY | CMAP | DPPV | CRNG | ????
CHUNKSIZE	-> (int32_t) // size of chunk not including header or size.
CMAP			-> "CMAP" + CHUNKSIZE + PALETTE_INFO
COMPRESS_METHOD -> (byte) // compression method used
CONTEXT		-> WIDTH + HEIGHT + POSX + POSY + #_BITPLANES + MASKING +
				   COMPRESS_METHOD + PAD + TRANS_COL + XASPECT + YASPECT +
					PAGEWIDTH + PAGEHEIGHT
CRNG			-> // we do not use

DPPV			-> // we do not use

FILESIZE		-> (int32_t)   //size of file minus (sizeof(FORM) + sizeof(FILESIZE)

GREEN			-> (byte) // green color gun value

HEIGHT		-> (word) // of picture

MASKING		-> (byte) // masking type ?

NUM_COLORS	-> //number of colors used depending on format

PAGE_WIDTH	-> (word) // width of page
PAGE_HEIGHT	-> (word) // height of page
PALETTE_INFO-> (RED+GREEN+BLUE) @ NUM_COLORS
PAD 			-> (byte) // not used.  used as a padding
POSX 			-> (word) // staring position
POSY			-> (word) // staring position

RED			-> (byte) // red color gun value

TRANS_COL	-> (word) // transparrent color

WIDTH			-> (word) // of picture 

XASPECT		-> (byte) // x aspect ratio

YASPECT		-> (byte) // y aspect ratio

????			-> // other possibilities


Differences in DPII enhance version

.LBM 		-> "FORM" + FILESIZE + "PBM " + CHUNKS
DPPV		-> DPPS		// uses dpps instead of dppv
CHUNKS		-> + TINY	// add these to old definition
 



#endif
