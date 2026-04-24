/*
**	Command & Conquer(tm)
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

/* $Header:   F:\projects\c&c\vcs\code\rawfile.h_v   2.16   16 Oct 1995 16:45:46   JOE_BOSTIC  $ */
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
 *   RawFileClass::~RawFileClass -- Default deconstructor for a file object.                   *
 *   RawFileClass::Is_Open -- Checks to see if the file is open or not.                        *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef RAWFILE_H
#define RAWFILE_H

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include <cstdint>

#include <SDL3/SDL_iostream.h>

#include "WWFILE.H"

#ifndef WWERROR
#define WWERROR -1
#endif

class RawFileClass : public FileClass
{
	public:
		int Rights;

		RawFileClass(char const* filename);
		RawFileClass(void);
		RawFileClass(RawFileClass const& file);
		RawFileClass& operator=(RawFileClass const& file);
		virtual ~RawFileClass(void);

		virtual char const* File_Name(void) const;
		virtual char const* Set_Name(char const* filename);
		virtual int Create(void);
		virtual int Delete(void);
		virtual int Is_Available(int forced = false);
		virtual int Is_Open(void) const;
		virtual int Open(char const* filename, int rights = READ);
		virtual int Open(int rights = READ);
		virtual int32_t Read(void* buffer, int32_t size);
		virtual int32_t Seek(int32_t pos, int dir = SEEK_CUR);
		virtual int32_t Size(void);
		virtual int32_t Write(void const* buffer, int32_t size);
		virtual void Close(void);
		virtual uint32_t Get_Date_Time(void);
		virtual bool Set_Date_Time(uint32_t datetime);
		virtual void Error(int error, int canretry = false, char const* filename = NULL);

		void Bias(int32_t start, int32_t length = -1);
		SDL_IOStream* Get_File_Handle(void) { return Handle; }

		int32_t BiasStart;
		int32_t BiasLength;

	protected:
		int32_t Transfer_Block_Size(void) { return INT32_MAX - 16; }
		int32_t Raw_Seek(int32_t pos, int dir = SEEK_CUR);

	private:
		SDL_IOStream* Handle;
		char const* Filename;
		uint16_t Date;
		uint16_t Time;
		unsigned Allocated : 1;
};

inline char const* RawFileClass::File_Name(void) const
{
	return Filename;
}

inline RawFileClass::RawFileClass(void)
	: Rights(READ)
	, BiasStart(0)
	, BiasLength(-1)
	, Handle(nullptr)
	, Filename(NULL)
	, Date(0)
	, Time(0)
	, Allocated(false)
{
}

inline RawFileClass::~RawFileClass(void)
{
	Close();
	if (Allocated && Filename) {
		free((void*)Filename);
		Filename = NULL;
		Allocated = false;
	}
}

inline int RawFileClass::Is_Open(void) const
{
	return Handle != nullptr;
}

#endif
