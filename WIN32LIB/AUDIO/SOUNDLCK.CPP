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
 *                    File Name : SOUNDLCK.CPP                             *
 *                                                                         *
 *                   Programmer : Phil W. Gorrow                           *
 *                                                                         *
 *                   Start Date : June 23, 1995                            *
 *                                                                         *
 *                  Last Update : June 23, 1995   [PWG]                    *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include "win32_compat.h"
#include "SDLAUDIOBACKEND.H"
#include <string.h>
#include "WWMEM.H"
#include "WWSTD.H"
#include "SOUNDINT.H"



LockedDataType LockedData;

/*=========================================================================*/
/* The following PRIVATE functions are in this file:                       */
/*=========================================================================*/


/*= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

/***************************************************************************
 * INIT_LOCKED_DATA -- Initializes sound driver locked data                *
 *                                                                         *
 * INPUT:		none                                                          *
 *                                                                         *
 * OUTPUT:     none                                                        *
 *                                                                         *
 * HISTORY:                                                                *
 *   06/23/1995 PWG : Created.                                             *
 *=========================================================================*/
void Init_Locked_Data(void)
{

	/*
	** Initialize all of the data elements that need to be locked.
	*/
	LockedData.DigiHandle			= -1;
	LockedData.ServiceSomething	= FALSE;
	LockedData.MagicNumber			= 0xDEAF;
	LockedData.UncompBuffer			= NULL;
//	LockedData.StreamBufferSize	= (2*SECONDARY_BUFFER_SIZE)+128;
	LockedData.StreamBufferSize	= (SECONDARY_BUFFER_SIZE/4)+128;
	LockedData.StreamBufferCount	= 16;
	LockedData.SoundVolume			= 255;
	LockedData.ScoreVolume			= 255;
	LockedData._int					= FALSE;
}
