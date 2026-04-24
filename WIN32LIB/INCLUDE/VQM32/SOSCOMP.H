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

/****************************************************************************
*
*  File              : soscomp.h
*  Date Created      : 6/1/94
*  Description       : 
*
*  Programmer(s)     : Nick Skrepetos
*  Last Modification : 10/1/94 - 11:37:9 AM
*  Additional Notes  : Modified by Denzil E. Long, Jr.
*
*****************************************************************************
*            Copyright (c) 1994,  HMI, Inc.  All Rights Reserved            *
****************************************************************************/

#ifndef  _SOS_COMPRESS
#include <stdint.h>
#define  _SOS_COMPRESS

/* compression types */
enum {
	_ADPCM_TYPE_1,
	};

/* define compression structure */
typedef struct _tagCOMPRESS_INFO {
	char       *lpSource;
	char       *lpDest;
	uint32_t dwCompSize;
	uint32_t dwUnCompSize;
	uint32_t dwSampleIndex;
	int32_t          dwPredicted;
	int32_t          dwDifference;
	int16_t         wCodeBuf;
	int16_t         wCode;
	int16_t         wStep;
	int16_t         wIndex;

	uint32_t dwSampleIndex2; //added BP for channel 2
	int32_t          dwPredicted2;   //added BP for channel 2
	int32_t          dwDifference2; 	//added BP for channel 2
	int16_t         wCodeBuf2; 		//added BP for channel 2
	int16_t         wCode2; 			//added BP for channel 2
	int16_t         wStep2; 			//added BP for channel 2
	int16_t         wIndex2; 			//added BP for channel 2
	int16_t         wBitSize;
	int16_t			    wChannels;
	} _SOS_COMPRESS_INFO;

/* compressed file type header */
typedef struct _tagCOMPRESS_HEADER {
	uint32_t dwType;              // type of compression
	uint32_t dwCompressedSize;    // compressed file size
	uint32_t dwUnCompressedSize;  // uncompressed file size
	uint32_t dwSourceBitSize;     // original bit size
	char          szName[16];          // file type, for error checking
	} _SOS_COMPRESS_HEADER;

/* Prototypes */


void sosCODECInitStream(_SOS_COMPRESS_INFO *);
uint32_t sosCODECCompressData(_SOS_COMPRESS_INFO *,uint32_t);
uint32_t sosCODECDecompressData(_SOS_COMPRESS_INFO *,uint32_t);


#endif
