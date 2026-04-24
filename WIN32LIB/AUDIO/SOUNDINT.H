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
 *                    File Name : SOUNDINT.H                               *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : June 23, 1995                            *
 *                                                                         *
 *                  Last Update : June 23, 1995   [PWG]                    *
 *                                                                         *
 * This file is the include file for the Westwood Sound Sytem defines and  *
 * routines that are handled in an interrupt.
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "win32_compat.h"
#include "SDLAUDIOBACKEND.H"

#include "SOUND.H"
#include <stdint.h>

/*
** Defines for true and false.  These are included because we do not allow
** the sound int to include any of the westwood standard headers.  If we
** did, there might be too much temptation to call another library function.
** this would be bad, because then that function would not be locked.
*/
#define	FALSE		0
#define	TRUE		1

/*
** Define the different type of sound compression avaliable to the westwood
** library.
*/
typedef enum {
	SCOMP_NONE=0,			// No compression -- raw data.
	SCOMP_WESTWOOD=1,		// Special sliding window delta compression.
	SCOMP_SONARC=33,		// Sonarc frame compression.
	SCOMP_SOS=99			// SOS frame compression.
} SCompressType;

/*
**	This is the safety overrun margin for the sonarc compressed
** data frames.  This value should be equal the maximum 'order' times
**	the maximum number of bytes per sample.  It should be evenly divisible
**	by 16 to aid paragraph alignment.
*/
#define	SONARC_MARGIN				32


/*
** Define the sample control structure which helps us to handle feeding
** data to the sound interrupt.
*/
#pragma pack(push, 8)
typedef struct {
	/*
	**	This flags whether this sample structure is active or not.
	*/
	uint32_t Active;
	//unsigned Active:1;

	/*
	**	This flags whether the sample is loading or has been started.
	*/
	//unsigned Loading:1;
	uint32_t Loading;

	/*
	**	This semaphore ensures that simultaneous update of this structure won't
	**	occur.  This is necessary since both interrupt and regular code can modify
	**	this structure.
	*/
	//unsigned DontTouch:1;
	uint32_t DontTouch;

	/*
	**	If this sample is really to be considered a score rather than
	**	a sound effect, then special rules apply.  These largely fall into
	**	the area of volume control.
	*/
	//unsigned IsScore:1;
	uint32_t IsScore;

	/*
	**	This is the original sample pointer. It is used to control the sample based on
	**	pointer rather than handle. The handle method is necessary when more than one
	**	sample could be playing simultaneously. The pointer method is necessary when
	**	the dealing with a sample that may have stopped behind the programmer's back and
	**	this occurance is not otherwise determinable.  It is also used in
	** conjunction with original size to unlock a sample which has been DPMI
	** locked.
	*/
	void const *Original;
	int32_t OriginalSize;

	/*
	**	These are pointers to the double buffers.
	*/
	AudioBackendBuffer * PlayBuffer;

	/*
	** Variable to keep track of the playback rate of this buffer
	*/
	int32_t	PlaybackRate;

	/*
	** Variable to keep track of the sample type ( 8 or 16 bit ) of this buffer
	*/
	int32_t	BitSize;

	/*
	** Variable to keep track of the stereo ability of this buffer
	*/
	int32_t	Stereo;

	/*
	**	The number of bytes in the buffer that has been filled but is not
	**	yet playing.  This value is normally the size of the buffer,
	**	except for the case of the last bit of the sample.
	*/
	int32_t DataLength;

	/*
	**	This is the buffer index for the low buffer that
	**	has been filled with data but not yet being
	**	played.
	*/
//	short int Index;

	/*
	**	Byte offset into the play buffer for writing the next
	**  chunk of sample to.
	*/
	uint32_t DestPtr;

	/*
	**	This flag indicates that there is more source data
	**  to copy to the play buffer
	**
	*/
	int32_t MoreSource;

	/*
	**	This flag indicates that the entire sample fitted inside the
	** SDL audio backend secondary buffer
	**
	*/
	int32_t OneShot;

	/*
	**	Pointer to the sound data that has not yet been copied
	**	to the playback buffers.
	*/
	void *Source;

	/*
	**	This is the number of bytes remaining in the source data as
	**	pointed to by the "Source" element.
	*/
	int32_t Remainder;

	/*
	**	Samples maintain a priority which is used to determine
	**	which sounds live or die when the maximum number of
	**	sounds are being played.
	*/
	int32_t Priority;

	/*
	**	This is the handle as returned by sosDIGIStartSample function.
	*/
	int16_t Handle;

	/*
	**	This is the current volume of the sample as it is being played.
	*/
	int32_t Volume;
	int32_t Reducer;		// Amount to reduce volume per tick.

	/*
	**	This is the compression that the sound data is using.
	*/
	SCompressType Compression;
	int16_t TrailerLen;						// Number of trailer bytes in buffer.
	uint8_t Trailer[SONARC_MARGIN];		// Maximum number of 'order' samples needed.


	uint32_t Pitch;
	uint16_t Flags;

	/*
	**	This flag indicates whether this sample needs servicing.
	**	Servicing entails filling one of the empty low buffers.
	*/
	int16_t Service;

	/*
	**	This flag is TRUE when the sample has stopped playing,
	**	BUT there is more data available.  The sample must be
	**	restarted upon filling the low buffer.
	*/
	int32_t Restart;

	/*
	**	Streaming control handlers.
	*/
	int32_t (*Callback)(int16_t id, int16_t *odd, void **buffer, int32_t *size);
	void	*QueueBuffer;	// Pointer to continued sample data.
	int32_t	QueueSize;		// Size of queue buffer attached.
	int16_t	Odd;				// Block number tracker (0..StreamBufferCount-1).
	int32_t	FilePending;	// Number of buffers already filled ahead.
	int32_t	FilePendingSize;	// Number of bytes in last filled buffer.

	/*
	**	The file variables are used when streaming directly off of the
	**	hard drive.
	*/
	int32_t	FileHandle;		// Streaming file handle (ERROR = not in use).
	void	*FileBuffer;	// Temporary streaming buffer (allowed to be freed).
	/*
	** The following structure is used if the sample if compressed using
	** the sos 16 bit compression Codec.
	*/

	_SOS_COMPRESS_INFO sosinfo;


} SampleTrackerType;


typedef struct LockedData {
	int32_t 		DigiHandle; 			// = -1;
	int32_t 					ServiceSomething;		// = FALSE;
	uint32_t 			MagicNumber; 			// = 0xDEAF;
	void 					*UncompBuffer;			// = NULL;
	int32_t 			StreamBufferSize; 	// = (2*SECONDARY_BUFFER_SIZE)+128;
	int16_t 			StreamBufferCount; 	// = 32;
	SampleTrackerType SampleTracker[MAX_SFX];
	uint32_t		SoundVolume;
	uint32_t		ScoreVolume;
	int32_t					_int;
} LockedDataType;

extern LockedDataType LockedData;
#pragma pack(pop)

void Init_Locked_Data(void);
int32_t Simple_Copy(void ** source, int32_t * ssize, void ** alternate, int32_t * altsize, void **dest, int32_t size);
int32_t Sample_Copy(SampleTrackerType *st, void ** source, int32_t * ssize, void ** alternate, int32_t * altsize, void * dest, int32_t size, SCompressType scomp, void * trailer, int16_t *trailersize);
void   maintenance_callback(void);
void   DigiCallback(uint32_t driverhandle, uint32_t callsource, uint32_t sampleid);
void  HMI_TimerCallback(void);
void *Audio_Add_Long_To_Pointer(void const *ptr, intptr_t size);
void	DPMI_Unlock(void const *ptr, int32_t const size);
void  Audio_Mem_Set(void const *ptr, uint8_t value, intptr_t size);
//	void	Mem_Copy(void *source, void *dest, unsigned long bytes_to_copy);
int32_t   Decompress_Frame(void * source, void * dest, int32_t size);
int32_t	 Decompress_Frame_Lock(void);
int32_t	 Decompress_Frame_Unlock(void);
int32_t	 sosCODEC_Lock(void);
int32_t	 sosCODEC_Unlock(void);
void	__GETDS(void);
