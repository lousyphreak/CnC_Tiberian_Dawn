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
 *                    File Name : SOUNDINT.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : June 23, 1995                            *
 *                                                                         *
 *                  Last Update : June 28, 1995   [PWG]                    *
 *                                                                         *
 * This module contains all of the functions that are used within our      *
 * sound interrupt.  They are stored in a seperate module because memory	*
 * around these functions must be locked or they will cause a read to		*
 * be generated while in an interrupt.													*
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 *   Simple_Copy -- Copyies 1 or 2 source chuncks to a dest                *
 *   Sample_Copy -- Copies sound data from source format to raw format.    *
 *   DigiCallback -- Low level double buffering handler.                   *
 *   save_my_regs -- Inline function which will save assembly regs         *
 *   restore_my_regs -- Inline function which will restore saved registes  *
 *   Audio_Add_Long_To_Pointer -- Adds an offset to a ptr casted void      *
 *   Init_Locked_Data -- Initializes sound driver locked data              *
 *   Audio_Mem_Set -- Quick see routine to set memory to a value           *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

#pragma pack(push, 4)
#include "win32_compat.h"
#include	"SDLAUDIOBACKEND.H"
#include	<WWSTD.H>
#include "SOUNDINT.H"
#include "MEMFLAG.H"
#include "AUDIO.H"

extern	char DebugBuffer[];

/***************************************************************************
** All routines past this point must be locked for the sound driver to	  **
** function under a VCPI memory manager.  These locks are unnecessary if  **
** the driver does not have to run under windows or does not use virtual  **
** memory.                                                                **
***************************************************************************/



/***************************************************************************
 * SIMPLE_COPY -- Copyies 1 or 2 source chuncks to a dest                  *
 *                                                                         *
 * INPUT:                                                                  *
 *                                                                         *
 * OUTPUT:                                                                 *
 *                                                                         *
 * WARNINGS:                                                               *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/23/1995 PWG : Created.                                             *
 *=========================================================================*/
int32_t Simple_Copy(void ** source, int32_t * ssize, void ** alternate, int32_t * altsize, void **dest, int32_t size)
{


	int32_t	out = 0;		// Number of bytes copied to the destination.

	/*
	**	It could happen that entering this routine, the source buffer
	**	has been exhausted, but the alternate buffer is still valid.
	**	Move the alternate into the primary position before proceeding.
	*/
	if (!(*ssize)) {
		*source = *alternate;
		*ssize = *altsize;
		*alternate = NULL;
		*altsize = 0;
	}

	if (*source && *ssize) {
		int32_t	s;				// Scratch length var.

		/*
		**	Copy as much as possible from the primary source, but no
		**	more than the primary source has to offer.
		*/
		s = size;
		if (*ssize < s) s = *ssize;
		Mem_Copy(*source, *dest, s);
		*source = Audio_Add_Long_To_Pointer(*source, s);
		*ssize -= s;
		*dest = Audio_Add_Long_To_Pointer(*dest, s);
		size -= s;
		out += s;

		/*
		**	If the primary source was insufficient to fill the request, then
		**	move the alternate into the primary position and try again.
		*/
		if (size) {
			*source = *alternate;
			*ssize = *altsize;
			*alternate = 0;
			*altsize = 0;
			out += Simple_Copy(source, ssize, alternate, altsize, dest, size);
		}
	}

	return(out);
}

/***********************************************************************************************
 * Sample_Copy -- Copies sound data from source format to raw format.                          *
 *                                                                                             *
 *    This routine is used to copy the sound data (possibly compressed) to the destination     *
 *    buffer in raw format.                                                                    *
 *                                                                                             *
 * INPUT:   source   -- Pointer to the source data (possibly compressed).                      *
 *                                                                                             *
 *          dest     -- Pointer to the destination buffer.                                     *
 *                                                                                             *
 *          size     -- The size of the destination buffer.                                    *
 *                                                                                             *
 * OUTPUT:  Returns with the number of bytes placed into the output buffer.  This is usually   *
 *          the number of bytes requested except in the case when the source is exhausted.     *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *   09/03/1994 JLB : Created.                                                                 *
 *   09/04/1994 JLB : Revamped entirely.                                                       *
 *=============================================================================================*/

int32_t Sample_Copy(SampleTrackerType *st, void ** source, int32_t * ssize, void ** alternate, int32_t * altsize, void * dest, int32_t size, SCompressType scomp, void * , int16_t *)
{

	int32_t	s;
	int32_t	datasize = 0;		// Output bytes.

	switch (scomp) {
		default:

		case SCOMP_NONE:
			datasize = Simple_Copy(source, ssize, alternate, altsize, &dest, size);
			break;

		case SCOMP_WESTWOOD:
		case SCOMP_SOS:
			while (size > 0) {

				/*
				**	The block spans two buffers.  It must be copied down to
				**	a staging area before it can be decompressed.
				*/
				{
					uint32_t magic;
					uint16_t fsize;
					uint16_t dsize;
					void *fptr;
					void *dptr;
					void *mptr;

					fptr = &fsize;
					dptr = &dsize;
					mptr = &magic;

					s = Simple_Copy(source, ssize, alternate, altsize, &fptr, sizeof(fsize));
					if (s < static_cast<int32_t>(sizeof(fsize))) {
						return datasize;
					}
					s = Simple_Copy(source, ssize, alternate, altsize, &dptr, sizeof(dsize));
					if (s < static_cast<int32_t>(sizeof(dsize)) || size < static_cast<int32_t>(dsize)) {
						return datasize;
					}

					s = Simple_Copy(source, ssize, alternate, altsize, &mptr, sizeof(magic));
					if (s < static_cast<int32_t>(sizeof(magic)) || magic != LockedData.MagicNumber) {
						return datasize;
					}

					/*
					**	If the frame and uncompressed data size are identical, then this
					**	indicates that the frame is not compressed.  Just copy it directly
					**	to the destination buffer in this case.
					*/
					if (fsize == dsize) {
						s = Simple_Copy(source, ssize, alternate, altsize, &dest, fsize);
						if (s < dsize) {
							return (datasize);
						}
					} else {

						/*
						**	The frame was compressed, so copy it to the staging buffer, and then
						**	uncompress it into the final destination buffer.
						*/
						fptr = LockedData.UncompBuffer;
						s = Simple_Copy(source, ssize, alternate, altsize, &fptr, fsize);
						if (s < fsize) {
							return (datasize);
						}
						if (scomp == SCOMP_WESTWOOD) {
							Decompress_Frame(LockedData.UncompBuffer, dest, dsize);
						} else {
							st->sosinfo.lpSource = (char *)LockedData.UncompBuffer;
							st->sosinfo.lpDest	 = (char *)dest;
							if (st->sosinfo.wBitSize==16 && st->sosinfo.wChannels==1){
								sosCODECDecompressData(&st->sosinfo, dsize);
							} else {
								General_sosCODECDecompressData(&st->sosinfo, dsize);
							}
						}
						dest = Audio_Add_Long_To_Pointer(dest, dsize);
					}
					datasize += dsize;
					size -= dsize;
				}
			}

			break;
	}
	return(datasize);
}






extern int32_t Convert_HMI_To_Backend_Volume(int32_t volume);

/***********************************************************************************************
 * maintenance_callback -- routine to service the direct play secondary buffers                *
 *                         and other stuff..?                                                  *
 *                                                                                             *
 *                                                                                             *
 * INPUT:    Nothing                                                                           *
 *                                                                                             *
 * OUTPUT:   Nothing                                                                           *
 *                                                                                             *
 * WARNINGS: None                                                                              *
 *                                                                                             *
 * HISTORY:                                                                                    *
 *     ....Unknown                                                                             *
 *    10/17/95 10:15PM ST : tidied up a tad for streamed audio                                 *
 *=============================================================================================*/
void   maintenance_callback(void)
{

	int32_t					index;				//index used in for loop
	SampleTrackerType	*st;					//ptr to SampleTracker structure
	uint32_t					play_cursor;		//Position that the backend is reading from
	uint32_t					write_cursor;		//Position in buffer that we can write to
	int32_t			 		bytes_copied;		//Number of bytes copied into the buffer
	int32_t					write_more;			//Flag to set if we need to write more into the buffer
	void *				play_buffer_ptr;	//Beginning of locked area of buffer
	void *				dummy_buffer_ptr;	//Length of locked area in buffer
	uint32_t					lock_length1;		//Beginning of second locked area in buffer
	uint32_t					lock_length2;		//Length of second locked area in buffer
	HRESULT				return_code;

	st = &LockedData.SampleTracker[0];
	for (index = 0; index < MAX_SFX; index++) {

		if (st->Active) {

			/*
			**	General service routine to handle moving small blocks from the
			**	source into the audio backend buffers.  If the source is
			**	compressed, then this will also uncompress it as the copy
			**	is performed.
			*/
			if (st->Service && !st->DontTouch ) {

				st->DontTouch = TRUE;

				/*
				** Get the current position of the backend play cursor within the buffer
				*/
				return_code = st->PlayBuffer->GetCurrentPosition ( &play_cursor , &write_cursor );

				/*
				** Check for unusual situations like a focus loss
				*/
				if (return_code != RAAUDIO_OK){
					if (return_code == RAAUDIOERR_BUFFERLOST){
						if (Audio_Focus_Loss_Function){
							Audio_Focus_Loss_Function();
						}
					}
					return;		//Our app has lost focus or something else nasty has happened
				}					//so dont update the sound buffers


				if (st->MoreSource){

					/*
					** If the backend read pointer is less than a quarter
					** of a buffer away from the end of the data then copy some
					** more.
				 	*/
					write_more = FALSE;

					if (play_cursor < st->DestPtr){
						if (st->DestPtr - play_cursor <= (uint32_t)(SECONDARY_BUFFER_SIZE/4)){
							write_more=TRUE;
						}
					} else {
						/* The only time that play_cursor can be greater than DestPtr is
						** if we wrote right to the end of the buffer last time and DestPtr
						** looped back to the beginning of the buffer.
						** That being the case, all we have to do is see if play_cursor is
						** within the last 25% of the buffer
						*/
						if (play_cursor > (uint32_t)(SECONDARY_BUFFER_SIZE*3/4) && st->DestPtr == 0U){
							write_more=TRUE;
						}
					}

					if (write_more){

						/*
						** Lock 1/2 of the backend buffer so we can write to it
						*/
						if ( RAAUDIO_OK== st->PlayBuffer->Lock (	(uint32_t)st->DestPtr ,
															(uint32_t)SECONDARY_BUFFER_SIZE/2,
															&play_buffer_ptr,
															&lock_length1,
															&dummy_buffer_ptr,
															&lock_length2 )){

							bytes_copied = Sample_Copy(	st,
														&st->Source,
														&st->Remainder,
														&st->QueueBuffer,
														&st->QueueSize,
														play_buffer_ptr,
														SECONDARY_BUFFER_SIZE/4,
														st->Compression,
														&st->Trailer[0],
														&st->TrailerLen);


							if ( bytes_copied != (SECONDARY_BUFFER_SIZE/4) ){
								/*
								** We must have reached the end of the sample
								*/
								st->MoreSource=FALSE;
								memset (((char*)play_buffer_ptr)+bytes_copied ,
										(st->BitSize & AUD_FLAG_16BIT) ? 0 : 0x80 ,
										(SECONDARY_BUFFER_SIZE/4)-bytes_copied);

								/*
								** Clear out an extra area in the buffer ahead of the play cursor
								** to give us a quiet period of grace in which to stop the buffer playing
								*/
								if (st->DestPtr == (uint32_t)(SECONDARY_BUFFER_SIZE*3/4)){
									if ( dummy_buffer_ptr && lock_length2 ){
										memset (dummy_buffer_ptr , (st->BitSize & AUD_FLAG_16BIT) ? 0 : 0x80 , lock_length2);
									}
								} else {
									memset ((char*)play_buffer_ptr+SECONDARY_BUFFER_SIZE/4 , (st->BitSize & AUD_FLAG_16BIT) ? 0 : 0x80 , SECONDARY_BUFFER_SIZE/4);
								}
							}

							/*
							**  Update our pointer into the backend buffer
							**
							*/
							st->DestPtr += (uint32_t)bytes_copied;

							if (st->DestPtr >= (uint32_t)SECONDARY_BUFFER_SIZE){
								st->DestPtr -= (uint32_t)SECONDARY_BUFFER_SIZE;
							}


							/*
							** Unlock the backend buffer
							*/
							st->PlayBuffer->Unlock(	play_buffer_ptr,
													lock_length1,
													dummy_buffer_ptr,
													lock_length2);
						}

					}				//write_more

				} else {			//!more_source

					/*
					** no more source to write - check if the buffer play
					** has overrun the end of the sample and stop it if it has
					*/
					if (((play_cursor >= st->DestPtr) && ((play_cursor - st->DestPtr) < (uint32_t)(SECONDARY_BUFFER_SIZE/4))) ||
						(!st->OneShot && ((play_cursor < st->DestPtr) && ((st->DestPtr - play_cursor) > (uint32_t)(SECONDARY_BUFFER_SIZE*3/4))))){
							st->PlayBuffer->Stop();
							st->Service = FALSE;
							Stop_Sample( index );
					}
				}					//more_source

				st->DontTouch = FALSE;

			}
			/*
			**	For file streamed samples, fill the queue pointer if needed.
			**	This allows for delays in calling the Sound_Callback function.
			*/
			if (!st->DontTouch && !st->QueueBuffer && st->FilePending) {
				st->QueueBuffer = Audio_Add_Long_To_Pointer(st->FileBuffer, (intptr_t)(st->Odd % LockedData.StreamBufferCount) * (intptr_t)LockedData.StreamBufferSize);
				st->FilePending--;
				st->Odd++;
				if (!st->FilePending) {
					st->QueueSize = st->FilePendingSize;
				} else {
					st->QueueSize = LockedData.StreamBufferSize;
				}
			}
		}

		/*
		**	Advance to the next sample control structure.
		*/
		st++;
	}


	if (!LockedData._int) {

		LockedData._int++;
		st = &LockedData.SampleTracker[0];
		for (index = 0; index < MAX_SFX; index++) {

			/*
			**	If there are any samples that require fading, then do so at this
			**	time.
			*/
			if (st->Active && st->Reducer && st->Volume) {
				if (st->Reducer >= st->Volume) {
					st->Volume = 0;
				} else {
					st->Volume -= st->Reducer;
				}

				//st->PlayBuffer->SetVolume (-( ( (32768-st->Volume)*1000) >>15 ) );

				if (st->IsScore){
					st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume( ( LockedData.ScoreVolume*(st->Volume >>7))/256) );
				}else{
					st->PlayBuffer->SetVolume ( Convert_HMI_To_Backend_Volume( ( LockedData.SoundVolume*(st->Volume >>7))/256) );
				}

			}
			st++;
		}
		LockedData._int--;
	}
}







/***************************************************************************
 * ADD_LONG_TO_POINTER -- Adds an offset to a ptr casted void              *
 *                                                                         *
 * INPUT:		void * ptr - the pointer to add to                            *
 *					long size  - the size to add to it								  *
 *                                                                         *
 * OUTPUT:     void * ptr - the new location it will point to              *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/23/1995 PWG : Created.                                             *
 *=========================================================================*/

void *Audio_Add_Long_To_Pointer(void const *ptr, intptr_t size)
{
	return ((void *) ( (char const *) ptr + size));
}

/***************************************************************************
 * AUDIO_MEM_SET -- Quick see routine to set memory to a value             *
 *                                                                         *
 * INPUT:		void const *  - the memory that needs to be set               *
 *					unsigned char - the value to set the memory to				  *
 *					long size     - how big an area to set							  *
 *                                                                         *
 * OUTPUT:		none                                                       *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/28/1995 PWG : Created.                                             *
 *=========================================================================*/
void Audio_Mem_Set(void const *ptr, uint8_t value, intptr_t size)
{
	uint8_t *temp = (uint8_t *)ptr;
	for (int32_t lp = 0; lp < size; lp ++) {
		*temp++ = value;
	}
}
#pragma pack(pop)
