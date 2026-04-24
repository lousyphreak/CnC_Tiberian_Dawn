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

/* $Header:   F:\projects\c&c\vcs\code\wwfile.h_v   2.14   06 Sep 1995 16:30:00   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : WWFILE.H                                                     *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : August 8, 1994   [JLB]                                       *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WWFILE_H
#define WWFILE_H

#include	<stdio.h>

#ifndef READ
#define	READ	_READ
#endif
#ifndef WRITE
#define	WRITE	_WRITE
#endif

class FileClass
{
	public:
		virtual ~FileClass(void) {};
		virtual char const * File_Name(void) const = 0;
		virtual char const * Set_Name(char const *filename) = 0;
		virtual int32_t Create(void) = 0;
		virtual int32_t Delete(void) = 0;
		virtual int32_t Is_Available(int32_t forced=false) = 0;
		virtual int32_t Is_Open(void) const = 0;
		virtual int32_t Open(char const *filename, int32_t rights=READ) = 0;
		virtual int32_t Open(int32_t rights=READ) = 0;
		virtual int32_t Read(void *buffer, int32_t size) = 0;
		virtual int32_t Seek(int32_t pos, int32_t dir=SEEK_CUR) = 0;
		virtual int32_t Size(void) = 0;
		virtual int32_t Write(void const *buffer, int32_t size) = 0;
		virtual void Close(void) = 0;

		operator char const * () {return File_Name();};
};

#endif
