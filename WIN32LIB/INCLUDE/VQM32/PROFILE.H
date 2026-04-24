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

#ifndef VQMPROFILE_H
#include <stdint.h>
#define VQMPROFILE_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*----------------------------------------------------------------------------
*
* FILE
*     Profile.h (32-Bit protected mode)
*
* DESCRIPTION
*     INI file profiling definitions.
*
* PROGRAMMER
*     Denzil E. Long, Jr.
*
* DATE
*     January 26, 1995
*
****************************************************************************/

/* Prototypes */
int32_t GetINIInt(char const *section, char const *entry,
		int32_t deflt, char const * fname);

int32_t GetINIString(char const *section, char const *entry, 
		char const *def, char *retbuffer, int32_t retlen, char const * fname);

int32_t Get_Frame_Pathname(char *inifile,int32_t anim_frame,char *ext,
		char *outbuf);

#endif /* VQMPROFILE_H */
