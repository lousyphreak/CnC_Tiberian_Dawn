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

/* $Id: soundio.cpp 1.41 1994/06/20 15:01:39 joe_bostic Exp $ */
#include <cstdint>
/***********************************************************************************************
 **             C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S             **
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Sound Library                                                *
 *                                                                                             *
 *                    File Name : SOUND.CPP                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : July 22, 1991                                                *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   Load_Long_Sample -- Loads a sample into XMS for double buffer system.                     *
 *   Read_Long_Sample -- Loads/Processes/Formats/Builds offset.                                             *
 *   Save_Table_Entry -- Put an entry in the offset table.                                     *
 *   Play_Long_Sample -- Calls Init_Long_Sample and Start_Long_Sample.                         *
 *   Start_Long_Sample -- Starts a sample playing that has be initialized.                     *
 *   Get_Table_Entry -- Gets next entry in table.                                              *
 *   Long_Sample_Ticks -- Gets number of ticks in sample if in header.                         *
 *   Long_Sample_Status -- Returns the status of the sample.                                   *
 *   Find_Table_Entry -- Finds next entry in table that matches mask.                          *
 *   Get_Table_Start -- Returns a pointer to first entry in table.                             *
 *   Long_Sample_Ticks_Played -- Number of ticks since sample started.                         *
 *   Install_Sample_Driver_Callback -- Pokes callback function into JM driver                  *
 *   Stop_Long_Sample -- Stops current long sample from playing.                               *
 *   Long_Sample_Loaded_Size -- Max buffer size to load a long sample.                         *
 *   Sound_Callback -- Audio driver callback function.                                         *
 *   DigiCallback -- Low level double buffering handler.                                       *
 *   Load_Sample_Into_Buffer -- Loads a digitized sample into a buffer.                        *
 *   Stream_Sample -- Streams a sample directly from a file.                                   *
 *   Sample_Read -- Reads sample data from an openned file.                                    *
 *   Continue_Sample -- Tags another block of data onto the currently playing.                 *
 *   Sample_Copy -- Copies sound data from source format to raw format.                        *
 *   File_Stream_Preload -- Handles initial proload of a streaming samples bu*
 *   Sample_Length -- returns length of a sample in ticks                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

extern	void Colour_Debug (int32_t call_number);
#pragma pack(push, 4)

#include "ra_messagebox.h"
#include "win32_compat.h"
#include	"SDLAUDIOBACKEND.H"

#include	<cstdarg>
#include <string.h>
#include	<WWMEM.H>
#include	"SOUNDINT.H"
#include	<stdio.h>
#include	<string.h>
#include <climits>
#include "sdl_fs.h"
#include	<stdlib.h>
#include	<KEYBOARD.H>
#include	<FILE.H>
#include	<TIMER.H>
#include	<math.h>
#pragma pack(push, 1)
#include	"AUDIO.H"
#pragma pack(pop)
AudioBackendBuffer *	DumpBuffer = NULL;

/*
**      If this is defined, then the streaming audio buffer will be filled
**      to maximum whenever filling is to occur. If undefined, it will fill
**      the streaming buffer in smaller chunks.
*/
#define SIMPLE_FILLING

/*
**      This is the number of times per sec that the maintenance callback gets called.
*/
#define MAINTENANCE_RATE                        40	//30 times per sec plus a safety margin

/*
**      Size of the temporary buffer in XMS/EMS that direct file
**      streaming of sounds will allocate.
*/
//#define STREAM_BUFFER_SIZE              (128L*1024L)
#define STREAM_BUFFER_SIZE              (128L*1024L)

/*
** Define the number of "StreamBufferSize" blocks that are read in
** at a minimum when the streaming sample load callback routine
** is called.  We will IGNORE loads that are less that this in order
** to avoid constant seeking on the CD.
*/
#define STREAM_CUSHION_BLOCKS   4


/*
**      This is the maximum size that a sonarc block can be.  All sonarc blocks
**      must be either a multiple of this value or a binary root of this value.
*/
#define LARGEST_SONARC_BLOCK            2048


//////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////// structs ///////////////////////////////////////

//void						*DigiBuffer = NULL;
static int32_t				StartingFileStream = FALSE;
int32_t						StreamLowImpact = FALSE;
MemoryFlagType			StreamBufferFlag = MEM_NORMAL;
int32_t							Misc;
int32_t							ReverseChannels = FALSE;
AudioBackendDevice *			SoundObject = NULL;
AudioBackendBuffer *	PrimaryBufferPtr = NULL;
AudioBufferFormat				DsBuffFormat;
AudioBufferFormat				PrimaryBuffFormat;
void						*FileStreamBuffer = NULL;	//Buffer for streaming audio from CD
/*
** Function to call if we detect focus loss
*/
void (*Audio_Focus_Loss_Function)(void) = NULL;


/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/

static int32_t File_Callback(int16_t id, int16_t *odd, void **buffer, int32_t *size);
static int32_t  Stream_Sample_Vol(void *buffer, int32_t size, int32_t (*callback)(int16_t id, int16_t *odd, void **buffer, int32_t *size), int32_t volume, int32_t handle);
static void Pump_Sound_Service(void);
volatile int32_t AudioDone = FALSE;
/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

static bool Sound_Trace_Enabled(void)
{
	static int32_t enabled = -1;
	if (enabled == -1) {
		char const* env = getenv("RA_TRACE_STARTUP");
		enabled = (env != NULL && env[0] != '\0') ? 1 : 0;
	}
	return(enabled != 0);
}


static void Sound_Trace(char const* format, ...)
{
	if (!Sound_Trace_Enabled()) {
		return;
	}

	va_list arguments;
	va_start(arguments, format);
	fprintf(stderr, "[sound] ");
	vfprintf(stderr, format, arguments);
	fputc('\n', stderr);
	fflush(stderr);
	va_end(arguments);
}



// This callback is called whenever the queue buffer playback has begun
// and another buffer is needed for queuing up.  Returns TRUE if there
// is more data to read from the file.
static int32_t File_Callback(int16_t id, int16_t *odd, void **buffer, int32_t *size)
{
	SampleTrackerType       *st;            // Pointer to sample playback control struct.
	void                    *ptr;           // Pointer to working portion of file buffer.

	if (id != -1) {
		st = &LockedData.SampleTracker[id];
	 	ptr = st->FileBuffer;
	 	if (ptr) {

	 	  	/*
	 	  	** Move the next pending block into the primary
	 	  	** position.  Do this only if the queue pointer is
	 	  	** null.
	 	  	*/
			st->DontTouch = TRUE;

			if (!*buffer && st->FilePending) {
	 	  		*buffer = Add_Long_To_Pointer(ptr, (intptr_t)(*odd % LockedData.StreamBufferCount) * (intptr_t)LockedData.StreamBufferSize);
	 	  		st->FilePending--;
	 	  		*odd = (int16_t)(*odd + 1);
	 	  		if (!st->FilePending) {
	 	  			*size = st->FilePendingSize;
	 	  		} else {
	 	  			*size = LockedData.StreamBufferSize;
	 	  		}
	 	  	}
	 	  	st->DontTouch = FALSE;
			Pump_Sound_Service();

	 	  	/*
	 	  	** If the file handle is still valid, then read in the next
	 	  	** block and add it to the next pending slot available.
	 	  	*/
	 	  	if (st->FilePending <
	 	  		(StreamLowImpact ? (LockedData.StreamBufferCount>>1) : ((LockedData.StreamBufferCount-3))) && st->FileHandle != WW_ERROR) {


	 	  		int32_t num_empty_buffers;

#ifdef SIMPLE_FILLING
				num_empty_buffers = (LockedData.StreamBufferCount-2) - st->FilePending;
#else

				//
				// num_empty_buffers will be from 1 to StreamBufferCount
				//
				if (StreamLowImpact) {
					num_empty_buffers = MIN((LockedData.StreamBufferCount >> 1)+STREAM_CUSHION_BLOCKS, (LockedData.StreamBufferCount - 2) - st->FilePending);
				}
				else {
					num_empty_buffers = (LockedData.StreamBufferCount - 2) - st->FilePending;
				}
#endif

				while (num_empty_buffers && (st->FileHandle != WW_ERROR)) {
					int32_t     tofill;
					int32_t psize;

					tofill = (*odd + st->FilePending) % LockedData.StreamBufferCount;

					ptr = Add_Long_To_Pointer(st->FileBuffer, (intptr_t)tofill * (intptr_t)LockedData.StreamBufferSize);
					psize = Read_File(st->FileHandle, ptr, LockedData.StreamBufferSize);

				 	/*
				 	**      If less than the requested amount of data was read, this
				 	**      indicates that the source file is exhausted.  Flag the source
				 	**      file as closed so that no further reading is attempted.
				 	*/
				 	if (psize != LockedData.StreamBufferSize) {
				 		Close_File(st->FileHandle);
						st->FileHandle = WW_ERROR;
				 	}

                /*
				 	**      If any real data went into the pending buffer, then flag
				 	**      that this buffer is valid.
                */
				 	if (psize) {
						st->DontTouch = TRUE;
				 	  	st->FilePendingSize = psize;
				 	  	st->FilePending++;
				 	  	st->DontTouch = FALSE;
						Pump_Sound_Service();
				 	}
				 	num_empty_buffers--;
				}

				/*
             **  After filling all pending buffers, check to see if the queue buffer
				**  is empty.  If so, then assign the first available pending buffer to the
             **  queue.
				*/
				st->DontTouch = TRUE;
              if (!st->QueueBuffer && st->FilePending) {
					st->QueueBuffer = Add_Long_To_Pointer(st->FileBuffer, (intptr_t)(st->Odd % LockedData.StreamBufferCount) * (intptr_t)LockedData.StreamBufferSize);
					st->FilePending--;
                st->Odd++;
                if (!st->FilePending) {
						st->QueueSize = st->FilePendingSize;
					} else {
                	st->QueueSize = LockedData.StreamBufferSize;
					}
				}
				st->DontTouch = FALSE;
				Pump_Sound_Service();
			}

			/*
          **      If there are no more buffers that the callback routine
			**      can slot into the primary position, then signal that
			**      no furthur callbacks are needed.
			*/
			if (st->FilePending) {
				return(TRUE);
			}
		}

	}
  return(FALSE);
}





// Generic streaming sample playback initialization.
static int32_t  Stream_Sample_Vol(void *buffer, int32_t size, int32_t (*callback)(int16_t id, int16_t *odd, void **buffer, int32_t *size), int32_t volume, int32_t handle)
{
	int32_t								playid=-1;      // Sample play ID.
	SampleTrackerType               *st;                    // Working pointer to sample control structure.
	int32_t							oldsize;                // Copy of original sound size.
	AUDHeaderType                   header;

	Sound_Trace("Stream_Sample_Vol start size=%ld volume=%d handle=%d audio_done=%d digi=%d callback=%p",
		size,
		volume,
		handle,
		AudioDone ? 1 : 0,
		LockedData.DigiHandle,
		callback);

	if (!AudioDone && buffer && size && LockedData.DigiHandle != -1) {

		/*
	 	** Start the first section of the sound playing.
	 	*/
	 	Mem_Copy(buffer, &header, sizeof(header));
	 	oldsize = header.Size;
	 	header.Size = size-sizeof(header);
	 	Mem_Copy(&header, buffer, sizeof(header));
	 	playid = Play_Sample_Handle(buffer, 0xFF, volume, 0x0, handle);
	 	header.Size = oldsize;
	 	Mem_Copy(&header, buffer, sizeof(header));

	 	/*
	 	** If the sample actually started playing, then flag this
	 	** sample as a streaming type and signal for a callback
	 	** to occur.
	 	*/
	 	if (playid != -1) {
	 		st = &LockedData.SampleTracker[playid];

	 		st->Callback = callback;
	 		st->Odd = 0;
//             ServiceSomething = TRUE;
    	}
	}
	Sound_Trace("Stream_Sample_Vol result playid=%d", playid);
		  return (playid);
}



#if (0)
static int32_t  Stream_Sample(void *buffer, int32_t size, int32_t (*callback)(int16_t id, int16_t *odd, void **buffer, int32_t *size), int32_t handle)
{
	return Stream_Sample_Vol(buffer, size, callback, 0xFF, handle);
}
#endif


/***********************************************************************************************
 * File_Stream_Sample -- Streams a sample directly from a file.                                *
 *                                                                                             *
 *    This will take the file specified and play it directly from disk.                        *
 *    It performs this by allocating a temporary buffer in XMS/EMS and                         *
 *    then keeping this buffer filled by the Sound_Callback() routine.                         *
 *                                                                                             *
 * INPUT:   filename -- The name of the file to play.                                          *
 *                                                                                             *
 * OUTPUT:  Returns the handle to the sound -- just like Play_Sample().                        *
 *                                                                                             *
 * WARNINGS:   The temporary buffer is allocated when this routine is                          *
 *             called and then freed when the sound is finished.  Keep                         *
 *             this in mind.                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int32_t File_Stream_Sample(char const *filename, int32_t real_time_start)
{
	return File_Stream_Sample_Vol(filename, 0xFF, real_time_start);
}

/***************************************************************************
 * FILE_STREAM_PRELOAD -- Handles initial proload of streaming samples     *
 *                                                                         *
 * This function is called before a sample which streams from disk is      *
 * started.  It can be called to either fill the buffer in small chunks    *
 * from the call back routine or to fill the entire buffer at once.  This  *
 * is wholely dependant on whether the Loading bit is set within the       *
 * sample tracker.                                                         *
 *                                                                         *
 * INPUT:LockedData.SampleTracker * to the header which tracks this samples*
 *                                                              processing.*
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/05/1995 PWG : Created.                                             *
 *=========================================================================*/

void File_Stream_Preload(int32_t handle)
{
	SampleTrackerType	*st		= &LockedData.SampleTracker[handle];
	int32_t					fh			= st->FileHandle;
	int32_t					maxnum	= (LockedData.StreamBufferCount >> 1) + STREAM_CUSHION_BLOCKS;
	void					*buffer	= st->FileBuffer;
	int32_t					num;

	/*
    ** Figure just how much we need to load.  If we are doing the load in progress
	** then we will only load two blocks.
	*/
    if (st->Loading) {
    	num = st->FilePending + 2;
       num = MIN(num, maxnum);
    } else {
		num = maxnum;
	}


    /*
    ** Loop through the blocks and load up the number we need.
    */
	int32_t index;
    for (index = st->FilePending; index < num; index++) {
		int32_t s = Read_File(fh, Add_Long_To_Pointer(buffer, (intptr_t)index * (intptr_t)LockedData.StreamBufferSize), LockedData.StreamBufferSize);
       if (s) {
         	st->FilePendingSize = s;
	  		st->FilePending++;
		}
		if (s < LockedData.StreamBufferSize) break;
    }

	Pump_Sound_Service();


	/*
	** If the last block was incomplete (ie. it didn't completely fill the buffer) or
	** we have now filled up as much of the Streaming Buffer as we need to, then now is
	** the time to kick off the sample.
	*/
	if (st->FilePendingSize < LockedData.StreamBufferSize || index == maxnum) {

		/*
		** Actually start the sample playing, and don't worry about the file callback
		** it won't be called for a while.
		*/
 		int32_t old						= LockedData.SoundVolume;
		int32_t size						= (st->FilePending == 1) ? st->FilePendingSize : LockedData.StreamBufferSize;
		LockedData.SoundVolume	= LockedData.ScoreVolume;
		StartingFileStream		= TRUE;

		Stream_Sample_Vol(buffer, size, File_Callback, st->Volume, handle);
		StartingFileStream		= FALSE;
       LockedData.SoundVolume	= old;

		/*
		** The Sample is finished loading (if it was loading in small pieces) so record that
		** so that it will now use the active logic in the file call back.
		*/
		st->Loading					= FALSE;

		/*
		** Decrement the file pending because the first block is already playing thanks
		** to the play sample call above.
		*/
		st->FilePending--;

		/*
		** If File pending is now a zero, then we only preloaded one block and there
		** is nothing more to play.  So clear the sample tracing structure of the
		** information it no longer needs.
		*/
		if (!st->FilePending) {
			st->Odd					= 0;
          st->QueueBuffer		= 0;
			st->QueueSize			= 0;
			st->FilePendingSize	= 0;
          st->Callback			= NULL;
			Close_File(fh);
		} else {
			/*
			** The QueueBuffer counts as an already played block so remove it from the total.
          ** Note: We didn't remove it before because there might not have been one.
          */
          st->FilePending--;

          /*
          ** When we start loading we need to start past the first two blocks.  Why this
          ** is called Odd, I haven't got the slightest.
			*/
          st->Odd = 2;

          /*
          ** If the file pending size is less than the stream buffer, then the last block
          ** we loaded was the last block period.  So close the file and reset the handle.
          */
          if (st->FilePendingSize != LockedData.StreamBufferSize) {
				Close_File(fh);
             st->FileHandle = WW_ERROR;
          }

          /*
          ** The Queue buffer needs to point at the next block to be processed.  The size
          ** of the queue is dependant on how many more blocks there are.
          */
			st->QueueBuffer = Add_Long_To_Pointer(buffer, LockedData.StreamBufferSize);
			if (!st->FilePending) {
          	st->QueueSize = st->FilePendingSize;
          } else {
          	st->QueueSize = LockedData.StreamBufferSize;
          }
		}
	}

}







/***********************************************************************************************
 * File_Stream_Sample_Vol -- Streams a sample directly from a file.                            *
 *                                                                                             *
 *    This will take the file specified and play it directly from disk.                        *
 *    It performs this by allocating a temporary buffer in XMS/EMS and                         *
 *    then keeping this buffer filled by the Sound_Callback() routine.                         *
 *                                                                                             *
 * INPUT:   filename -- The name of the file to play.                                          *
 *                                                                                             *
 * OUTPUT:  Returns the handle to the sound -- just like Play_Sample().                        *
 *                                                                                             *
 * WARNINGS:   The temporary buffer is allocated when this routine is                          *
 *             called and then freed when the sound is finished.  Keep                         *
 *             this in mind.                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *=============================================================================================*/

int32_t File_Stream_Sample_Vol(char const *filename, int32_t volume, int32_t real_time_start)
{
	SampleTrackerType       *st;
	int32_t     fh;
	int32_t     handle = -1;
	int32_t     index;
	int32_t    found = FALSE;

	if (filename) {
		found = Find_File(filename);
	}

	Sound_Trace("File_Stream_Sample_Vol start file=%s volume=%d real_time=%d audio_done=%d digi=%d found=%d",
		filename ? filename : "<null>",
		volume,
		real_time_start ? 1 : 0,
		AudioDone ? 1 : 0,
		LockedData.DigiHandle,
		found ? 1 : 0);

	if (!AudioDone && LockedData.DigiHandle != -1 && filename && found) {

	/*
	**  Make sure all sample tracker structures point to the same
	**  upper memory buffer.  This allocation only occurs if at
	**  least one sample gets "streamed".
	*/
	if (!FileStreamBuffer) {
		FileStreamBuffer = Alloc(LockedData.StreamBufferSize * LockedData.StreamBufferCount, (MemoryFlagType)(StreamBufferFlag | MEM_TEMP | MEM_LOCK));
       for (index = 0; index < MAX_SFX; index++) {
       	LockedData.SampleTracker[index].FileBuffer = FileStreamBuffer;
		}
	}
	/*
	** If after trying to allocate the buffer we still fail then
	** we can stream this sample.
	*/
	if (!FileStreamBuffer) return(-1);


    	/*
    	** Lets see if we can sucessfully open up the file.  If we can't,
    	** then there is no point in going any farther.
    	*/
    	if ((fh = Open_File(filename, READ)) == -1) {
 	 			return (-1);
    	}

		/*
		** Reserve a handle so that we can fill in the sample tracker
		** with the needed information.  If we dont get valid handle then
		** we might as well give up.
		*/
		if ((uint32_t)(handle = Get_Free_Sample_Handle(0xFF)) >= MAX_SFX) {
 				return(-1);
		}

		/*
		** Now lets get a pointer to the proper sample handler and start
		** our manipulations.
		*/
		st							= &LockedData.SampleTracker[handle];
		st->IsScore				= TRUE;
		st->FilePending		= 0;
		st->FilePendingSize	= 0;
		st->Loading				= real_time_start;
		st->Volume				= volume;
		st->FileHandle			= fh;

		/*
		** Now that we have setup our initial data properly, let load up
		** the beginning of the sample we intend to stream.
		*/
		File_Stream_Preload(handle);
	}

	Sound_Trace("File_Stream_Sample_Vol result file=%s handle=%d loading=%d pending=%d file_handle=%d",
		filename ? filename : "<null>",
		handle,
		(handle >= 0) ? (st->Loading ? 1 : 0) : -1,
		(handle >= 0) ? st->FilePending : -1,
		(handle >= 0) ? st->FileHandle : -1);
	return (handle);
}

/***********************************************************************************************
 * Sound_Callback -- Audio driver callback function.                                           *
 *                                                                                             *
 *    Maintains the audio buffers.  This routine must be called at least                       *
 *    11 times per second or else audio glitches will occur.                                   *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   If this routine is not called often enough then audio                           *
 *             glitches will occur.                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
void  Sound_Callback(void)
{
	int32_t					index;
    SampleTrackerType	*st;

    if (!AudioDone && LockedData.DigiHandle != -1) {

		/*
		** Call the timer callback now as we may block it in this function
		*/
		Pump_Sound_Service();

		st = &LockedData.SampleTracker[0];
		for (index = 0; index < MAX_SFX; index++) {
			if (st->Loading) {
				File_Stream_Preload(index);
			} else {
          	/*
          	**  General service routine to handle moving small blocks from the
          	**  source into the low RAM staging buffers.
          	*/

				if (st->Active) {

					/*
          		**  Special check to see if the sample is a fading one AND
          		**  it has faded to silence, then stop it here.
          		*/
          		if (st->Reducer && !st->Volume) {
						Stop_Sample(index);
					} else {

          			/*
             		**  Fill the queuebuffer if it is currently empty
             		**  and there is a callback function defined to fill it.
             		**
             		** PWG/CDY & CO: We should be down by at least two blocks
						**                                        before we bother with this
             		*/

             		if ((!st->QueueBuffer ||
							(st->FileHandle != WW_ERROR && st->FilePending < LockedData.StreamBufferCount-3)) &&
				 			st->Callback) {

                		if (!st->Callback((int16_t)index, (int16_t *)&st->Odd, &st->QueueBuffer, &st->QueueSize)) {
								st->Callback = NULL;
                		}
             		}
          		}

				} else {

          		/*
          		** This catches the case where a streaming sample gets
          		** aborted prematurely because of failure to call the
          		** callback function frequently enough.  In this case, the
          		** sample will be flagged as inactive, but the file handle
          		** will not have been closed.
          		*/
          		if (st->FileHandle != WW_ERROR) {
          			Close_File(st->FileHandle);
             		st->FileHandle = WW_ERROR;
          		}
				}
			}

          /*
          **      Advance to the next sample control structure.
			*/
    		st++;
		}

    }
}


/***********************************************************************************************
 * Load_Sample -- Loads a digitized sample into RAM.                                           *
 *                                                                                             *
 *    This routine loads a digitized sample into RAM.                                          *
 *                                                                                             *
 * INPUT:   filename -- Name of the sound file to load.                                        *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the loaded sound file.  This pointer                     *
 *          is passed to Play_Sample when playback is desired.                                 *
 *                                                                                             *
 * WARNINGS:   If there is insufficient memory to load the sample, then                        *
 *             NULL will be returned.                                                          *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *   01/06/1994 JLB : HMI version.                                                             *
 *=============================================================================================*/
void *Load_Sample(char const *filename)
{
	void *buffer = NULL;
	int32_t size;
	int32_t     fh;

	if (LockedData.DigiHandle == -1 || !filename || !Find_File(filename)) {
		return nullptr;
	}

	fh = Open_File(filename, READ);
	if (fh != WW_ERROR) {
		size = File_Size(fh)+sizeof(AUDHeaderType);
		buffer = Alloc(size, MEM_NORMAL);

		if (buffer) {
			Sample_Read(fh, buffer, size);
		}
		Close_File(fh);
		Misc = size;
	}
	return(buffer);
}


/***********************************************************************************************
 * Load_Sample_Into_Buffer -- Loads a digitized sample into a buffer.                          *
 *                                                                                             *
 *    This routine is used to load a digitized sample into a buffer                            *
 *    provided by the programmer.  This buffer can be in XMS or EMS.                           *
 *                                                                                             *
 * INPUT:   filename -- The filename to load.                                                  *
 *                                                                                             *
 *          buffer   -- Pointer to the buffer to load into.                                    *
 *                                                                                             *
 *          size     -- The size of the buffer to load into.                                   *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually used in the buffer.                           *
 *                                                                                             *
 * WARNINGS:   This routine will not overflow the buffer provided.  This                       *
 *             means that the buffer must be big enough to hold the data                       *
 *             or else the sound will be cut short.                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int32_t Load_Sample_Into_Buffer(char const *filename, void *buffer, int32_t size)
{
	int32_t     fh;

	/*
	**      Verify legality of parameters.
	*/
	if (!buffer || !size || LockedData.DigiHandle == -1 || !filename || !Find_File(filename)) {
		return 0;
	}

	fh = Open_File(filename, READ);
	if (fh != WW_ERROR) {
		size = Sample_Read(fh, buffer, size);
       Close_File(fh);
	} else {
		return(0);
	}
	return(size);
}


/***********************************************************************************************
 * Sample_Read -- Reads sample data from an openned file.                                      *
 *                                                                                             *
 *    This routine reads a sample file.  It is presumed that the file is                       *
 *    already positioned at the start of the sample.  From this, it can                        *
 *    determine if it is a VOC or raw data and proceed accordingly.                            *
 *                                                                                             *
 * INPUT:   fh       -- File handle of already openned sample file.                            *
 *                                                                                             *
 *          buffer   -- Pointer to the buffer to load data into.                               *
 *                                                                                             *
 *          size     -- The size of the buffer.                                                *
 *                                                                                             *
 * OUTPUT:  Returns the number of bytes actually used in the buffer.                           *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   01/06/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
int32_t Sample_Read(int32_t fh, void *buffer, int32_t size)
{

	AUDHeaderType	RawHeader;
	void    			*outbuffer;				// Pointer to start of raw data.
	int32_t			actual_bytes_read;	// Actual bytes read in, including header

/*
** Conversion formula for TCrate and Hz rate.
**
** TC = 256 - 1m/rate
** rate = 1m / (256-TC)
*/

	if (!buffer || fh == WW_ERROR || size <= static_cast<int32_t>(sizeof(RawHeader))) return(0);

	size -= sizeof(RawHeader);
	outbuffer = Add_Long_To_Pointer(buffer, sizeof(RawHeader));
  	actual_bytes_read = Read_File(fh, &RawHeader, sizeof(RawHeader));
	actual_bytes_read +=Read_File(fh, outbuffer, MIN(size, (int32_t)RawHeader.Size));
	Mem_Copy(&RawHeader, buffer, sizeof(RawHeader));
	return(actual_bytes_read);
}


/***********************************************************************************************
 * Free_Sample -- Frees a previously loaded digitized sample.                                  *
 *                                                                                             *
 *    Use this routine to free the memory allocated by a previous call to                      *
 *    Load_Sample.                                                                             *
 *                                                                                             *
 * INPUT:   sample   -- Pointer to the sample to be freed.                                     *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
void Free_Sample(void const *sample)
{
	if (sample) Free((void *)sample);
}











/***********************************************************************************************
 * Pump_Sound_Service -- runs the audio maintenance pass on the current thread                 *
 *=============================================================================================*/

static void Pump_Sound_Service(void)
{
	if (!AudioDone) {
		maintenance_callback();
	}
}


/***********************************************************************************************
 * Set_Primary_Buffer_Format -- set the format of the primary sound buffer                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   TRUE if successfully set                                                          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    12/22/95 4:06PM ST : Created                                                             *
 *=============================================================================================*/

int32_t Set_Primary_Buffer_Format(void)
{
	if (SoundObject && PrimaryBufferPtr){
		return (PrimaryBufferPtr->SetFormat ( &PrimaryBuffFormat ) == RAAUDIO_OK);
	}
	return (FALSE);
}



/***********************************************************************************************
 * Print_Sound_Error -- show error messages from failed sound initialisation                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    error text                                                                        *
 *           handle to window                                                                  *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/7/96 10:17AM ST : Created                                                              *
 *=============================================================================================*/

void Print_Sound_Error(char const * sound_error, RAWindow* window)
{
	RA_ShowMessageBox(sound_error, "Command & Conquer", SDL_MESSAGEBOX_ERROR);
}



/***********************************************************************************************
 * Audio_Init -- Initialise the sound system                                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    window - window to send callback messages to                                      *
 *           maximum bits_per_sample - 8 or 16                                                 *
 *           stereo - will stereo samples be played                                            *
 *           rate - maximum sample rate required                                               *
 *           reverse_channels                                                                  *
 *                                                                                             *
 * OUTPUT:   TRUE if correctly initialised                                                     *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   Unknown....                                                                               *
 *   08-24-95 10:01am ST : Modified for Windows 95 audio output                                *
 *=============================================================================================*/

int32_t Audio_Init( RAWindow* window , int32_t bits_per_sample, int32_t stereo , int32_t rate , int32_t reverse_channels)
{
	int32_t	index;
	int16_t old_bits_per_sample;
	int16_t old_block_align;
	int32_t	old_bytes_per_sec;


	Init_Locked_Data();
	FileStreamBuffer = NULL;
	memset(&LockedData.SampleTracker[0], 0, sizeof(LockedData.SampleTracker));

	if ( !SoundObject ){

		/*
		**	Create the SDL audio backend object.
		*/
		if (CreateAudioBackendDevice(&SoundObject) != RAAUDIO_OK) {
			Print_Sound_Error("Unable to create SDL audio backend", window);
			return (FALSE);
		}

		/*
		**	Give the backend the owning window for focus-related behavior.
		*/
		if (SoundObject->SetWindow(window) != RAAUDIO_OK) {
			Print_Sound_Error("Unable to configure SDL audio backend", window);
			SoundObject->Release();
			SoundObject = NULL;
			return (FALSE);
		}

		/*
		**	Set up the primary buffer format
		*/
		memset (&DsBuffFormat , 0 , sizeof(AudioBufferFormat));
		DsBuffFormat.wFormatTag		= RA_AUDIO_FORMAT_PCM;
		DsBuffFormat.nChannels		= (uint16_t)(1 + stereo);
		DsBuffFormat.nSamplesPerSec	= rate;
		DsBuffFormat.wBitsPerSample	= (int16_t)bits_per_sample;
		DsBuffFormat.nBlockAlign	= (uint16_t)((DsBuffFormat.wBitsPerSample / 8) * DsBuffFormat.nChannels);
		DsBuffFormat.nAvgBytesPerSec= DsBuffFormat.nSamplesPerSec * DsBuffFormat.nBlockAlign;
		DsBuffFormat.cbSize = 0;


		memcpy(&PrimaryBuffFormat, &DsBuffFormat, sizeof(AudioBufferFormat));

		/*
		**	Create the primary buffer object
		*/
		if (SoundObject->CreateBuffer(4096, NULL, TRUE, &PrimaryBufferPtr) != RAAUDIO_OK) {
			Print_Sound_Error("Unable to create SDL primary audio buffer", window);
			SoundObject->Release();
			SoundObject = NULL;
			return (FALSE);
		}

		/*
		** Set the format of the primary sound buffer
		**
		*/
		if (!Set_Primary_Buffer_Format()){

			/*
			** If we failed to create a 16 bit primary buffer - try for an 8bit one
			*/
			if (DsBuffFormat.wBitsPerSample == 16){
				/*
				** Save the old values
				*/
				old_bits_per_sample	= DsBuffFormat.wBitsPerSample;
				old_block_align		= DsBuffFormat.nBlockAlign;
				old_bytes_per_sec		= DsBuffFormat.nAvgBytesPerSec;

				/*
				** Set up the 8-bit ones
				*/
				DsBuffFormat.wBitsPerSample	= 8;
				DsBuffFormat.nBlockAlign	= (uint16_t)((DsBuffFormat.wBitsPerSample / 8) * DsBuffFormat.nChannels);
				DsBuffFormat.nAvgBytesPerSec= DsBuffFormat.nSamplesPerSec * DsBuffFormat.nBlockAlign;

				memcpy(&PrimaryBuffFormat, &DsBuffFormat, sizeof(AudioBufferFormat));
			}

			if (!Set_Primary_Buffer_Format()){

				/*
				** We failed to set any useful format so print up an error message and give up
				*/
				PrimaryBufferPtr->Release();
				PrimaryBufferPtr = NULL;
				SoundObject->Release();
				SoundObject = NULL;

				Print_Sound_Error("Unable to configure a compatible SDL audio format", window);

				return (FALSE);
			}else{

				/*
				** OK, got an 8bit sound buffer. Not perfect but it will do
				** We still want 16 bit secondary buffers so restore those values
				*/
				DsBuffFormat.wBitsPerSample	= old_bits_per_sample;
				DsBuffFormat.nBlockAlign		= old_block_align;
				DsBuffFormat.nAvgBytesPerSec	= old_bytes_per_sec;
			}
		}


		/*
		** Start the primary sound buffer playing
		**
		*/if (PrimaryBufferPtr->Play(TRUE) != RAAUDIO_OK) {
			Print_Sound_Error("Unable to start SDL primary audio buffer", window);
			PrimaryBufferPtr->Release();
			PrimaryBufferPtr = NULL;
			SoundObject->Release();
			SoundObject = NULL;
			return (FALSE);
		}

		LockedData.DigiHandle=1;


		/*
		** Run an initial audio maintenance pass on the current thread so the
		** legacy refill logic stays single-threaded.
		*/
		AudioDone = FALSE;
		Pump_Sound_Service();

		/*
		**	Define the format for the secondary sound buffers
		*/
		/*
    	** Allocate a decompression buffer equal to the size of a SONARC frame
		** block.
    	*/
    	LockedData.UncompBuffer = Alloc(LARGEST_SONARC_BLOCK + 50, (MemoryFlagType)(MEM_NORMAL|MEM_CLEAR|MEM_LOCK));

    	/*
    	** Allocate one backend secondary buffer for each simultaneous sound effect
		**
		*/
		for (index = 0; index < MAX_SFX; index++) {

			if (SoundObject->CreateBuffer(SECONDARY_BUFFER_SIZE, &DsBuffFormat, FALSE, &LockedData.SampleTracker[index].PlayBuffer) != RAAUDIO_OK) {
				Sound_End();
				Print_Sound_Error("Unable to create SDL sample buffer", window);
				return (FALSE);
			}

			LockedData.SampleTracker[index].PlaybackRate	= rate;
			LockedData.SampleTracker[index].Stereo			= (stereo) ? AUD_FLAG_STEREO : 0;
			LockedData.SampleTracker[index].BitSize 		= (bits_per_sample == 16) ? AUD_FLAG_16BIT : 0;
			LockedData.SampleTracker[index].FileHandle 	= WW_ERROR;
			LockedData.SampleTracker[index].QueueBuffer 	= NULL;

		}

		ReverseChannels = reverse_channels;

	}

	return(TRUE);
}


/***********************************************************************************************
 * Sound_End -- Uninitializes the sound driver.                                                *
 *                                                                                             *
 *    This routine will uninitialize the sound driver (if any was                              *
 *    installed).  This routine must be called at program termination                          *
 *    time.                                                                                    *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   07/23/1991 JLB : Created.                                                                 *
 *   11/02/1995 ST  : Modified for backend audio                                                *
 *=============================================================================================*/
void Sound_End(void)
{

	int32_t	index;

	if (SoundObject && PrimaryBufferPtr){
		/*
		** Stop all sounds and release the backend secondary sound buffers
		*/
		for (index=0 ; index < MAX_SFX; index++){
			if ( LockedData.SampleTracker[index].PlayBuffer ){
				Stop_Sample (index);
				LockedData.SampleTracker[index].PlayBuffer->Stop();
				LockedData.SampleTracker[index].PlayBuffer->Release();
				LockedData.SampleTracker[index].PlayBuffer = NULL;
			}
		}
	}

	if (FileStreamBuffer){
		Free (FileStreamBuffer);
		FileStreamBuffer = NULL;
	}

	/*
	** Stop and release the backend primary buffer
	*/
	if (PrimaryBufferPtr){
		PrimaryBufferPtr->Stop();
		PrimaryBufferPtr->Release();
		PrimaryBufferPtr = NULL;
	}

	/*
	** Release the backend audio device
	*/
	if (SoundObject){
		SoundObject->Release();
		SoundObject = NULL;
	}


	if (LockedData.UncompBuffer) {
    	Free(LockedData.UncompBuffer);
		LockedData.UncompBuffer = 0;
	}

	AudioDone = TRUE;
}




/***********************************************************************************************
 * Stop_Sample -- Stops any currently playing sampled sound.                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1992 JLB : Created.                                                                 *
 *   11/2/95 4:09PM ST : Modified for streamed audio                                           *
 *=============================================================================================*/
void Stop_Sample(int32_t handle)
{
	if (LockedData.DigiHandle != -1 && (uint32_t)handle < MAX_SFX) {

		if (AudioDone)	return;


		if (LockedData.SampleTracker[handle].Active || LockedData.SampleTracker[handle].Loading) {

			LockedData.SampleTracker[handle].Active = FALSE;

			if (!LockedData.SampleTracker[handle].IsScore) {
          	LockedData.SampleTracker[handle].Original = NULL;
			}

			LockedData.SampleTracker[handle].Priority = 0;

			/*
          **  Stop the sample if it is playing.
          */
			if (!LockedData.SampleTracker[handle].Loading) {
				LockedData.SampleTracker[handle].PlayBuffer->Stop();
          }

			LockedData.SampleTracker[handle].Loading = FALSE;

          /*
			**  If this is a streaming sample, then close the source file.
          */
			if (LockedData.SampleTracker[handle].FileHandle != WW_ERROR) {
				Close_File(LockedData.SampleTracker[handle].FileHandle);
				LockedData.SampleTracker[handle].FileHandle = WW_ERROR;
			}

			LockedData.SampleTracker[handle].QueueBuffer = NULL;
		}
	}
}


/***********************************************************************************************
 * Sample_Status -- Queries the current playing sample status (if any).                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:                                                                                      *
 *                                                                                             *
 * OUTPUT:                                                                                     *
 *                                                                                             *
 * WARNINGS:                                                                                   *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   06/02/1992 JLB : Created.                                                                 *
 *=============================================================================================*/
int32_t Sample_Status(int32_t handle)
{
	uint32_t	status;

	if (AudioDone) return (FALSE);

	/*
	** If its an invalid handle or we do not have a sound driver then
	** the sample in question is not playing.
	*/
	if (LockedData.DigiHandle == -1 || (uint32_t)handle >= MAX_SFX) return(FALSE);

	/*
	** If the sample is loading, then for all intents and purposes the
	** sample is playing.
	*/
	if (LockedData.SampleTracker[handle].Loading) return(TRUE);

	/*
	** If the sample is not active, then it is not playing
	*/
	if (!LockedData.SampleTracker[handle].Active) return(FALSE);

	/*
	** If we made it this far, then the Sample is still playing if sos says
	** that it is.
	*/
	DumpBuffer = LockedData.SampleTracker[handle].PlayBuffer;
	if (LockedData.SampleTracker[handle].PlayBuffer->GetStatus( &status ) == RAAUDIO_OK){
		return ( (RAAUDIOSTATUS_PLAYING & status) || (RAAUDIOSTATUS_LOOPING & status) );
	}else{
		return (TRUE);
	}
}



/***********************************************************************************************
 * Is_Sample_Playing -- returns the play state of a sample                                     *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to sample data                                                                *
 *                                                                                             *
 * OUTPUT:   TRUE if sample is currently playing                                               *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:11PM ST : Commented                                                            *
 *=============================================================================================*/

int32_t Is_Sample_Playing(void const * sample)
{
	int32_t index;

	if (AudioDone) return (FALSE);


	if (!sample) {
		return FALSE;
	}
	for (index = 0; index < MAX_SFX; index++) {
		if (LockedData.SampleTracker[index].Original == sample && Sample_Status(index)) {
			return (TRUE);
		}
	}
	return (FALSE);
}


/***********************************************************************************************
 * Stop_Sample_Playing -- stops a playing sample                                               *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to sample data                                                                *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:13PM ST : Commented                                                            *
 *=============================================================================================*/

void Stop_Sample_Playing(void const * sample)
{
	int32_t index;

	if (sample) {
		for (index = 0; index < MAX_SFX; index++) {
			if (LockedData.SampleTracker[index].Original == sample) {
				Stop_Sample(index);
				break;
			}
		}
	}
}





/***********************************************************************************************
 * Get_Free_Sample_Handle -- finds a free slot in which to play a new sample                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    priority of sample we want to play                                                *
 *                                                                                             *
 * OUTPUT:   Handle or -1 if none free                                                         *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:14PM ST : Added function header                                                *
 *=============================================================================================*/

int32_t Get_Free_Sample_Handle(int32_t priority)
{
	int32_t     id;

	/*
	**      Find a free SFX holding buffer slot.
	*/
	for (id = MAX_SFX - 1; id >= 0; id--) {
		if (!LockedData.SampleTracker[id].Active && !LockedData.SampleTracker[id].Loading) {
			if (!StartingFileStream && LockedData.SampleTracker[id].IsScore) {
				StartingFileStream = TRUE;      // Ensures only one channel is kept free for scores.
				continue;
			}
			break;
		}
	}

	if (id < 0) {
		for (id = 0; id < MAX_SFX; id++) {
			if (LockedData.SampleTracker[id].Priority <= priority) break;
		}

		if (id == MAX_SFX) {
			return(-1);             // Cannot play!
		}
		Stop_Sample(id);                                                        // This sample gets clobbered.
	}

	if (id == -1) {
		return -1;
	}

	if (LockedData.SampleTracker[id].FileHandle != WW_ERROR) {
		Close_File(LockedData.SampleTracker[id].FileHandle);
		LockedData.SampleTracker[id].FileHandle = WW_ERROR;
	}

	if (LockedData.SampleTracker[id].Original && !LockedData.SampleTracker[id].IsScore) {
		LockedData.SampleTracker[id].Original = NULL;
	}
	LockedData.SampleTracker[id].IsScore = FALSE;
	return(id);
}

int32_t Play_Sample(void const *sample, int32_t priority, int32_t volume, int16_t panloc)
{
		  return(Play_Sample_Handle(sample, priority, volume, panloc, Get_Free_Sample_Handle(priority)));
}





/***********************************************************************************************
 * Attempt_Audio_Restore -- tries to restore backend audio buffers                             *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    ptr to backend audio buffer                                                       *
 *                                                                                             *
 * OUTPUT:   TRUE if buffer was successfully restored                                          *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    3/20/96 9:47AM ST : Created                                                              *
 *=============================================================================================*/

int32_t Attempt_Audio_Restore (AudioBackendBuffer * sound_buffer)
{

	int32_t 	return_code;
	uint32_t	play_status;
	int32_t	restore_attempts=0;

	if (AudioDone){
		return (FALSE);
	}

	/*
	** Call the audio focus loss function if it has been set up
	*/
	if (Audio_Focus_Loss_Function){
		Audio_Focus_Loss_Function();
	}

	/*
	** Try to restore the sound buffer
	*/
	do{
		Restore_Sound_Buffers();
		return_code = sound_buffer->GetStatus ( &play_status );

	} while (restore_attempts++<2 && return_code == RAAUDIOERR_BUFFERLOST);

	return (return_code != RAAUDIOERR_BUFFERLOST);
}



/***********************************************************************************************
 * Convert_HMI_To_Backend_Volume -- Converts a linear volume value into an expotential    *
 *                                        value                                                *
 *                                                                                             *
 * This function converts a linear C&C volume in the range 0-255 (255 loudest) to the backend  *
 * volume scale in the range 0 to -10000 (with 0 being the loudest)                            *
 *                                                                                             *
 * INPUT:    volume in range 0-255                                                             *
 *                                                                                             *
 * OUTPUT:   volume in range -10000 to 0                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * Note: The 27.685 value comes from 255 divided by the log of 10001                           *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    9/18/96 11:36AM ST : Created                                                             *
 *=============================================================================================*/
int32_t Convert_HMI_To_Backend_Volume(int32_t volume)
{
	if (volume == 0) return -10000;
	if (volume == 255) return 0;

	float vol = (float)volume;
	float retval = exp ( (255.0-vol)/27.68597374) -1;
	return ((int32_t)-retval);
}





/***********************************************************************************************
 * Play_Sample_Vol -- Plays a digitized sample.                                                *
 *                                                                                             *
 *    Use this routine to play a previously loaded digitized sample.                           *
 *                                                                                             *
 * INPUT:   sample   -- Sample pointer as returned from Load_Sample.                           *
 *                                                                                             *
 *          volume   -- The volume to play (0..255 with 255=loudest).                          *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   04/17/1992 JLB : Created.                                                                 *
 *   05/24/1992 JLB : Volume support -- Soundblaster Pro                                       *
 *   04/22/1994 JLB : Multiple sample playback rates.                                          *
 *   11/02/1995 ST  : Windows audio backend support                                            *
 *=============================================================================================*/
extern int32_t Any_Locked(void);
extern int32_t GameInFocus;
int32_t Play_Sample_Handle(void const *sample, int32_t priority, int32_t volume, int16_t , int32_t id)
{
	AUDHeaderType                   RawHeader;
	SampleTrackerType               *st=NULL;       // Working pointer to sample tracker structure.

	void *				play_buffer_ptr;		//pointer to locked backend buffer
	void *				dummy_buffer_ptr;		//dummy pointer to second area of locked backend buffer
	uint32_t					lock_length1;
	uint32_t					lock_length2;
	uint32_t					play_status;
	HRESULT				return_code;


	if (Any_Locked()) return(0);

	if (AudioDone) return (0);

	if (!sample || LockedData.DigiHandle == -1) {
		return(-1);
	}

	if ((uint32_t)id >= MAX_SFX) {
		return -1;
	}

	st = &LockedData.SampleTracker[id];

	/*
	**      Fetch the control bytes from the start of the sample data.
	*/
	Mem_Copy((void *)sample, (void *)&RawHeader, sizeof(RawHeader));

	/*
	** Fudge the sample rate to 22k
	*/
	if (RawHeader.Rate <24000 && RawHeader.Rate >20000) RawHeader.Rate = 22050;

	/*
	**      Prepare the sample tracker structure for processing of this
	**      sample.  Fill the structure with data that can be determined
	**      before the sample is started.
	*/
	st->Compression 			= (SCompressType) ((uint8_t)RawHeader.Compression);
	st->Original            = sample;
	st->OriginalSize        = RawHeader.Size + sizeof(RawHeader);
	st->Priority            = (int16_t)priority;
	st->DontTouch           = TRUE;
	st->Odd						= 0;
	st->Reducer             = 0;
	st->Restart             = FALSE;
	st->QueueBuffer 			= NULL;
	st->QueueSize           = 0;
	st->TrailerLen  			= 0;
	st->Remainder           = RawHeader.Size;
	st->Source              = Add_Long_To_Pointer((void *)sample, sizeof(RawHeader));
	st->Service             = FALSE;

    /*
	 ** If the code in question using HMI based compression then we need
    ** to set up for uncompressing it.
	 */
    if (st->Compression == SCOMP_SOS) {
		st->sosinfo.wChannels    = (RawHeader.Flags & AUD_FLAG_STEREO) ? 2  : 1;
		st->sosinfo.wBitSize             = (RawHeader.Flags & AUD_FLAG_16BIT)  ? 16 : 8;
		st->sosinfo.dwCompSize   = RawHeader.Size;
		st->sosinfo.dwUnCompSize = RawHeader.Size * ( st->sosinfo.wBitSize / 4 );
		sosCODECInitStream(&st->sosinfo);
	}

	/*
	** If the sample rate , bits per sample or stereo capabilities of the buffer do not
	** match the sample then reallocate the backend buffer with the required capabilities
	*/
	if (  ( RawHeader.Rate != st->PlaybackRate ) ||
			( ( RawHeader.Flags & AUD_FLAG_16BIT ) != ( st->BitSize & AUD_FLAG_16BIT ) ) ||
			( ( RawHeader.Flags & AUD_FLAG_STEREO) != ( st->Stereo & AUD_FLAG_STEREO ) ) ) {

		st->Active=0;
		st->Service=0;
		st->MoreSource=0;

		/*
		** Stop the sound buffer playing
		*/
		DumpBuffer = st->PlayBuffer;
		do {
			return_code = st->PlayBuffer->GetStatus ( &play_status );
			if (return_code==RAAUDIOERR_BUFFERLOST){
				if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
			}
		}while (return_code == RAAUDIOERR_BUFFERLOST);

		if (play_status & (RAAUDIOSTATUS_PLAYING | RAAUDIOSTATUS_LOOPING) ){
			st->PlayBuffer->Stop();
			if (return_code==RAAUDIOERR_BUFFERLOST){
				if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
			}
		}

		st->PlayBuffer->Release();
		st->PlayBuffer=NULL;

		DsBuffFormat.nSamplesPerSec	= (uint16_t)RawHeader.Rate;
		DsBuffFormat.nChannels			= (RawHeader.Flags & AUD_FLAG_STEREO) ? 2 : 1 ;
		DsBuffFormat.wBitsPerSample	= (RawHeader.Flags & AUD_FLAG_16BIT) ? 16 : 8 ;
		DsBuffFormat.nBlockAlign	= (int16_t)((DsBuffFormat.wBitsPerSample / 8) * DsBuffFormat.nChannels);
		DsBuffFormat.nAvgBytesPerSec= DsBuffFormat.nSamplesPerSec * DsBuffFormat.nBlockAlign;

		/*
		** Create the new sound buffer
		*/
		return_code = SoundObject->CreateBuffer(SECONDARY_BUFFER_SIZE, &DsBuffFormat, FALSE, &st->PlayBuffer);
		if (return_code==RAAUDIOERR_BUFFERLOST){
			if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
		}
		/*
		** Just return if the create failed unexpectedly
		**
		** If we failed then flag the buffer as having an impossible format so it wont match
		** any sample. This will ensure that we try and create the buffer again next time its used.
		*/
		if (return_code!=RAAUDIO_OK && return_code!=RAAUDIOERR_BUFFERLOST){
			st->PlaybackRate = 0;
			st->Stereo = 0;
			st->BitSize = 0;
			return(-1);
		}

		/*
		** Remember the format of the new buffer
		*/
		st->PlaybackRate 	= RawHeader.Rate;
		st->Stereo			= RawHeader.Flags & AUD_FLAG_STEREO;
		st->BitSize 		= RawHeader.Flags & AUD_FLAG_16BIT;
	}

	/*
	**      Fill in 3/4 of the play buffer.
	*/

	//
	// Stop the sound buffer playing before we lock it
	//
	do {
		DumpBuffer = st->PlayBuffer;
		return_code = st->PlayBuffer->GetStatus ( &play_status );
		if (return_code==RAAUDIOERR_BUFFERLOST){
			if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
		}
	} while (return_code==RAAUDIOERR_BUFFERLOST);

	if (play_status & (RAAUDIOSTATUS_PLAYING | RAAUDIOSTATUS_LOOPING) ){
		st->Active=0;
		st->Service=0;
		st->MoreSource=0;
		st->PlayBuffer->Stop();
	}
	//
	// Lock the backend buffer so we can write to it
	//
	do {
		return_code = st->PlayBuffer->Lock(0,
										SECONDARY_BUFFER_SIZE,
										&play_buffer_ptr,
										&lock_length1,
										&dummy_buffer_ptr,
										&lock_length2);
		if (return_code==RAAUDIOERR_BUFFERLOST){
			if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
		}
	} while (return_code==RAAUDIOERR_BUFFERLOST);

	if (return_code != RAAUDIO_OK) {
		return (-1);
	}

	//
	// Decompress the sample into the backend buffer
	//
	st->DestPtr=(uint32_t)Sample_Copy ( 	st,
								&st->Source,
								&st->Remainder,
								&st->QueueBuffer,
								&st->QueueSize,
								play_buffer_ptr,
								SECONDARY_BUFFER_SIZE*1/4,
								st->Compression,
								&st->Trailer[0],
								&st->TrailerLen);

	if (st->DestPtr == (uint32_t)(SECONDARY_BUFFER_SIZE*1/4)){

		// Must be more data to copy so we dont need to zero the buffer
		st->MoreSource=TRUE;
		st->Service=TRUE;
		st->OneShot=FALSE;
	} else {

		// Whole sample is in the buffer so flag that we dont need to
		// copy more. Clear out the end of the buffer so that it
		// goes quiet if we play past the end
		st->MoreSource=FALSE;
		st->OneShot=TRUE;
		st->Service=TRUE;				//We still need to service it so that we can stop it when
											// it plays past the end of the sample data
		memset (((char*)play_buffer_ptr) + st->DestPtr,
			(RawHeader.Flags & AUD_FLAG_16BIT) ? 0 : 0x80,
			SECONDARY_BUFFER_SIZE/4);
	}

	st->PlayBuffer->Unlock(	play_buffer_ptr,
							lock_length1,
							dummy_buffer_ptr,
							lock_length2);


	/*
    **
    **  Set the volume of the sample.
    **
    */
	st->Volume = (volume << 7);
	do {

		//return_code = st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume(volume));
		st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume( ( LockedData.SoundVolume*volume)/256) );
		//return_code = st->PlayBuffer->SetVolume (- ( ( (32768- ( (st->Volume >> 8) *LockedData.SoundVolume) )
		//															*1000) >>15 ) );
		if (return_code==RAAUDIOERR_BUFFERLOST){
			if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
		}
	} while (return_code==RAAUDIOERR_BUFFERLOST);


	/*
	** Make sure the primary sound buffer is playing
	*/
	if (!Start_Primary_Sound_Buffer(FALSE)){
		Sound_Trace("Play_Sample_Handle primary start failed id=%d focus=%d", id, GameInFocus ? 1 : 0);
		return(-1);
	}


	/*
	** Set the buffers play pointer to the beginning of the buffer
	*/
	do {
		return_code = st->PlayBuffer->SetCurrentPosition (0);
		if (return_code==RAAUDIOERR_BUFFERLOST){
			if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
		}
	} while (return_code==RAAUDIOERR_BUFFERLOST);


	/*
    ** Start the sample playing now.
	*/
	do
	{
		return_code = st->PlayBuffer->Play(TRUE);

		switch (return_code){

			case RAAUDIO_OK :
				st->Active=TRUE;
				st->Handle=(int16_t)id;
				st->DontTouch = FALSE;
				return (st->Handle);

			case RAAUDIOERR_BUFFERLOST :
				if (!Attempt_Audio_Restore(st->PlayBuffer)) return(-1);
				break;

			default:
				st->Active=FALSE;
				return (st->Handle);
		}

	} while (return_code==RAAUDIOERR_BUFFERLOST);

	return (st->Handle);
}





/***********************************************************************************************
 * Restore_Sound_Buffers -- restore the sound buffers                                          *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/3/95 3:53PM ST : Created                                                              *
 *=============================================================================================*/

void Restore_Sound_Buffers ( void )
{

	if (PrimaryBufferPtr){
		PrimaryBufferPtr->Restore();
	}


	for ( int32_t index = 0; index < MAX_SFX; index++) {
		if (LockedData.SampleTracker[index].PlayBuffer){
			LockedData.SampleTracker[index].PlayBuffer->Restore();
		}
	}
}








/***********************************************************************************************
 * Set_Sound_Vol -- sets the overall volume for sampled sounds                                 *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    volume                                                                            *
 *                                                                                             *
 * OUTPUT:   the previous volume setting                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:19PM ST : Added function header                                                *
 *=============================================================================================*/

int32_t Set_Sound_Vol(int32_t volume)
{
	int32_t old;
 	old = LockedData.SoundVolume;
 	LockedData.SoundVolume = volume & 0xFF;
	return(old);
}


/***********************************************************************************************
 * Set_Score_Vol -- sets the overall volume for music scores                                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    volume                                                                            *
 *                                                                                             *
 * OUTPUT:   the previous volume setting                                                       *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:19PM ST : Added function header                                                *
 *=============================================================================================*/
int32_t Set_Score_Vol(int32_t volume)
{
	int32_t old;
	SampleTrackerType	*st;					//ptr to SampleTracker structure

	old = LockedData.ScoreVolume;
	LockedData.ScoreVolume = volume & 0xFF;

	for (int32_t index=0 ; index<MAX_SFX ; index++){
		st = &LockedData.SampleTracker[index];

		if (st->IsScore && st->Active){
			//st->PlayBuffer->SetVolume (- ( ( (32768- ( (st->Volume >> 8) *LockedData.ScoreVolume) )
			//														*1000) >>15 ) );

			//st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume ( st->Volume >>7 ) );
			st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume( ( LockedData.ScoreVolume*(st->Volume >>7))/256) );
		}
	}
	return(old);
}



/***********************************************************************************************
 * Fade_Sample -- Start a sample fading                                                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Sample handle                                                                     *
 *           fade rate                                                                         *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    11/2/95 4:21PM ST : Added function header                                                *
 *=============================================================================================*/

void Fade_Sample(int32_t handle, int32_t ticks)
{
	if (Sample_Status(handle)) {
    	if (!ticks || LockedData.SampleTracker[handle].Loading) {
       	Stop_Sample(handle);
			} else {
				SampleTrackerType * st;
				st = &LockedData.SampleTracker[handle];
				st->Reducer = (int16_t)((st->Volume / ticks) + 1);
			}
	}
}

/***************************************************************************
 * SAMPLE_LENGTH -- returns length of a sample in ticks                    *
 *                                                                         *
 * INPUT:  void const *sample - pointer to the sample to get length of.    *
 *                                                                         *
 * OUTPUT: long - length of the sample in ticks (60/sec)                   *
 *                                                                         *
 * HISTORY:                                                                *
 *   07/05/1995 PWG : Created.                                             *
 *=========================================================================*/
int32_t Sample_Length(void const *sample)
{
	AUDHeaderType	RawHeader;

	if (!sample) return(0);

	Mem_Copy((void *)sample, (void *)&RawHeader, sizeof(RawHeader));

	int32_t time = RawHeader.UncompSize;

	/*
	** If the sample is a 16 bit sample, then it will take only half
	** as long to play.
	*/
	if (RawHeader.Flags & AUD_FLAG_16BIT) {
		time >>= 1;
	}

	/*
	** If the sample is a stereo sample, then it will take only half
	** as long to play.
	*/
	if (RawHeader.Flags & AUD_FLAG_STEREO) {
		time >>= 1;
	}

	if (RawHeader.Rate/60) {
		time /= (RawHeader.Rate/60);
	}
	return(time);
}



/***********************************************************************************************
 * Start_Primary_Sound_Buffer -- start the primary sound buffer playing                        *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 12:28PM ST : Created                                                              *
 *=============================================================================================*/
int32_t Start_Primary_Sound_Buffer (int32_t forced)
{
	uint32_t status;

	if (PrimaryBufferPtr && GameInFocus){
		if (forced){
			PrimaryBufferPtr->Play(TRUE);
			return (TRUE);
		} else {

			if (PrimaryBufferPtr->GetStatus (&status) == RAAUDIO_OK){
				if (! ((status & RAAUDIOSTATUS_PLAYING) || (status & RAAUDIOSTATUS_LOOPING))){
					PrimaryBufferPtr->Play(TRUE);
					return (TRUE);
				}else{
					return (TRUE);
				}
			}
		}
	}
	return (FALSE);
}


/***********************************************************************************************
 * Stop_Primary_Sound_Buffer -- stops the primary sound buffer from playing.                   *
 *                                                                                             *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: This stops all sound playback                                                     *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *    2/1/96 12:28PM ST : Created                                                              *
 *=============================================================================================*/

void Stop_Primary_Sound_Buffer (void)
{
	if (PrimaryBufferPtr){
		PrimaryBufferPtr->Stop();
	}

	for ( int32_t index = 0; index < MAX_SFX; index++) {
		Stop_Sample(index);
	}

}


#pragma pack(pop)
