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
 **      C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S      **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Westwood 32 bit Library                  *
 *                                                                         *
 *                    File Name : AUDIO.H                                  *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : March 10, 1995                           *
 *                                                                         *
 *                  Last Update : March 10, 1995   [PWG]                   *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "WWSTD.H"
#include <stdint.h>

/*=========================================================================*/
/* AUD file header type																		*/
/*=========================================================================*/
#define	AUD_FLAG_STEREO	1
#define	AUD_FLAG_16BIT		2

// PWG 3-14-95: This structure used to have bit fields defined for Stereo
//   and Bits. These were removed because the original toolchain packed them
//   into a 32-bit flag entry even though they could have fit in an 8-bit entry.
#pragma pack(push, 1)
typedef struct {
	uint16_t	Rate;				// Playback rate (hertz).
	int32_t	Size;				// Size of data (bytes).
	int32_t	UncompSize;				// Size of data (bytes).
	uint8_t Flags;	// Holds flags for info
								//  1: Is the sample stereo?
								//  2: Is the sample 16 bits?
	uint8_t Compression;	// What kind of compression for this sample?
} AUDHeaderType;
#pragma pack(pop)

#ifdef __cplusplus
static_assert(sizeof(AUDHeaderType) == 12, "AUDHeaderType must match the 12-byte on-disk layout");
#endif


/*=========================================================================*/
/* The following prototypes are for the file: SOUNDIO.CPP						*/
/*=========================================================================*/
int32_t File_Stream_Sample(char const *filename, int32_t real_time_start = FALSE);
int32_t File_Stream_Sample_Vol(char const *filename, int32_t volume, int32_t real_time_start = FALSE);
void  Sound_Callback(void);
void   maintenance_callback(void);
void *Load_Sample(char const *filename);
int32_t Load_Sample_Into_Buffer(char const *filename, void *buffer, int32_t size);
int32_t Sample_Read(int32_t fh, void *buffer, int32_t size);
void Free_Sample(void const *sample);
int32_t Audio_Init( RAWindow* window , int32_t bits_per_sample, int32_t stereo , int32_t rate , int32_t reverse_channels);
void Sound_End(void);
void Stop_Sample(int32_t handle);
int32_t Sample_Status(int32_t handle);
int32_t Is_Sample_Playing(void const * sample);
void Stop_Sample_Playing(void const * sample);
int32_t Play_Sample(void const *sample, int32_t priority=0xFF, int32_t volume=0xFF, int16_t panloc = 0x0);
int32_t Play_Sample_Handle(void const *sample, int32_t priority, int32_t volume, int16_t panloc, int32_t id);
int32_t Set_Sound_Vol(int32_t volume);
int32_t Set_Score_Vol(int32_t volume);
void Fade_Sample(int32_t handle, int32_t ticks);
int32_t Get_Free_Sample_Handle(int32_t priority);
int32_t Sample_Length(void const *sample);
void Restore_Sound_Buffers (void);
int32_t Set_Primary_Buffer_Format(void);
int32_t Start_Primary_Sound_Buffer (int32_t forced);
void Stop_Primary_Sound_Buffer (void);

/*
** Function to call if we detect focus loss
*/
extern	void (*Audio_Focus_Loss_Function)(void);


extern int32_t Misc;

extern int32_t StreamLowImpact;
