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

#ifndef VQMMIXFILE_H
#include <stdint.h>
#define VQMMIXFILE_H
/****************************************************************************
*
*         C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     mixfile.h
*
* DESCRIPTION
*     A mix file is basically a group of files concatinated together
*     proceeded by a header describing where in the file each individual
*     entry is located. These definitions are provided to simplify the access
*     to these file entries.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     January 26, 1995
*
****************************************************************************/

/* Disable structure alignment.*/
#pragma pack(push, 1)

/*---------------------------------------------------------------------------
 * STRUCTURE DEFINITIONS
 *-------------------------------------------------------------------------*/

/* MIXHeader: Mix file data header.
 *
 * Count - Number of entries contained in this mix file.
 * Size  - Size of Mix file.
 */
typedef struct _MIXHeader {
	int16_t Count;
	int32_t  Size;
} MIXHeader;

/* MIXSubBlock: Mix file entry descriptor.
 *
 * CRC    - Unique entry identifier.
 * Offset - Offset from beginning of data segment to entry.
 * Size   - Size of entry.
 */
typedef struct _MIXSubBlock {
	int32_t CRC;
	int32_t Offset;
	int32_t Size;
} MIXSubBlock;

#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(MIXHeader) == 6, "MIXHeader must match the 6-byte on-disk layout");
static_assert(sizeof(MIXSubBlock) == 12, "MIXSubBlock must match the 12-byte on-disk layout");
#endif

/* MIXHandle: Mix file handle.
 *
 * Name    - Pointer to the name of the mix file this handle is for.
 * Size    - Size of entire mix file.
 * FH      - DOS file handle of opened entry.
 * Count   - Number of files contained in this mix.
 * Entries - Array of 'Count' MIXSubBlock structure entries.
 */
typedef struct _MIXHandle {
	char        *Name;
	int32_t        Size;
	int32_t        FH;
	int32_t        Count;
	MIXSubBlock Entries[];
} MIXHandle;

/*---------------------------------------------------------------------------
 * PROTOTYPES
 *-------------------------------------------------------------------------*/

MIXHandle *OpenMix(char *name);
void CloseMix(MIXHandle *mix);
int32_t OpenMixEntry(MIXHandle *mix, char *name);

#endif /* VQMMIXFILE_H */
