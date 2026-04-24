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

/* $Header:   F:\projects\c&c\vcs\code\rawfile.h_v   2.15   06 Sep 1995 16:29:30   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : RAWFILE.H                                                    *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RawFileClass::File_Name -- Returns with the filename associate with the file object.      *
 *   RawFileClass::RawFileClass -- Default constructor for a file object.                      *
 *   RawFileClass::Is_Open -- Checks to see if the file is open or not.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RAWFILE_H
#define RAWFILE_H

#include "win32_compat.h"

//#include	<WWLIB32.H>
#include	<limits.h>
#include	<errno.h>
#include "win32_compat.h"
//#include	<algo.h>
#include	"WWFILE.H"

/*
**	This is the definition of the raw file class. It is derived from the abstract base FileClass
**	and handles the interface to the low level DOS routines. This is the first class in the
**	chain of derived file classes that actually performs a useful function. With this class,
**	I/O is possible. More sophisticated features, such as packed files, higher-level media access,
**	file caching, and XMS/EMS memory support, are handled by derived classes.
**
**	Of particular importance is the need to override the error routine if more sophisticated
**	error handling is required. This is more than likely if greater functionality is derived
**	from this base class.
*/
class RawFileClass : public FileClass
{
	public:

		/*
		**	This is a record of the access rights used to open the file. These rights are
		**	used if the file object is duplicated.
		*/
		int32_t Rights;

		RawFileClass(char const *filename);
		RawFileClass(void);
		RawFileClass (RawFileClass const & f);
		RawFileClass & operator = (RawFileClass const & f);
		virtual ~RawFileClass(void) {if (Allocated && Filename) free((char *)Filename);};

		virtual char const * File_Name(void) const;
		virtual char const * Set_Name(char const *filename);
		virtual int32_t Create(void);
		virtual int32_t Delete(void);
		virtual int32_t Is_Available(int32_t forced=false);
		virtual int32_t Is_Open(void) const;
		virtual int32_t Open(char const *filename, int32_t rights=READ);
		virtual int32_t Open(int32_t rights=READ);
		virtual int32_t Read(void *buffer, int32_t size);
		virtual int32_t Seek(int32_t pos, int32_t dir=SEEK_CUR);
		virtual int32_t Size(void);
		virtual int32_t Write(void const *buffer, int32_t size);
		virtual void Close(void);
		virtual void Error(int32_t error, int32_t canretry = false, char const * filename=NULL);
		virtual void Set_Buffer_Size(int32_t size);


	protected:

		/*
		**	This function returns the largest size a low level DOS read or write may
		**	perform. Larger file transfers are performed in chunks of this size or less.
		*/
		int32_t Transfer_Block_Size(void) {return (int32_t)((uint32_t)UINT_MAX)-16L;};

	private:

		/*
		**	This is the low level DOS handle. A -1 indicates an empty condition.
		*/
		int32_t Handle;

		/*
		**	This points to the filename as a NULL terminated string. It may point to either a
		**	constant or an allocated string as indicated by the "Allocated" flag.
		*/
		char const * const Filename;

		/*
		**	Filenames that were assigned as part of the construction process
		**	are not allocated. It is assumed that the filename string is a
		**	constant in that case and thus making duplication unnecessary.
		**	This value will be non-zero if the filename has be allocated
		**	(using strdup()).
		*/
		uint32_t Allocated:1;
};


/***********************************************************************************************
 * RawFileClass::File_Name -- Returns with the filename associate with the file object.        *
 *                                                                                             *
 *    Use this routine to determine what filename is associated with this file object. If no   *
 *    filename has yet been assigned, then this routing will return NULL.                      *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  Returns with a pointer to the file name associated with this file object or NULL   *
 *          if one doesn't exist.                                                              *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
;*   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline char const * RawFileClass::File_Name(void) const
{
	return Filename;
}

/***********************************************************************************************
 * RawFileClass::RawFileClass -- Default constructor for a file object.                        *
 *                                                                                             *
 *    This constructs a null file object. A null file object has no file handle or filename    *
 *    associated with it. In order to use a file object created in this fashion it must be     *
 *    assigned a name and then opened.                                                         *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  none                                                                               *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
;*   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline RawFileClass::RawFileClass(void) : Filename(0)
{
	Handle = -1;
	Allocated = false;
}

/***********************************************************************************************
 * RawFileClass::Is_Open -- Checks to see if the file is open or not.                          *
 *                                                                                             *
 *    Use this routine to determine if the file is open. It returns true if it is.             *
 *                                                                                             *
 * INPUT:   none                                                                               *
 *                                                                                             *
 * OUTPUT:  bool; Is the file open?                                                            *
 *                                                                                             *
 *                                                                                             *
 * WARNINGS:   none                                                                            *
 *                                                                                             *
 * HISTORY:                                                                                    *
;*   10/18/1994 JLB : Created.                                                                 *
 *=============================================================================================*/
inline int32_t RawFileClass::Is_Open(void) const
{
	return (Handle != -1);
}

#endif
