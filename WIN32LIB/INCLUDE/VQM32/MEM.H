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

#ifndef VQAMEM_H
#include <stdint.h>
#define VQAMEM_H
/****************************************************************************
*
*        C O N F I D E N T I A L -- W E S T W O O D  S T U D I O S
*
*---------------------------------------------------------------------------
*
* FILE
*     mem.h
*
* DESCRIPTION
*     Memory management definitions.
*
* PROGRAMMER
*     Phil Gorrow
*     Denzil E. Long, Jr.
*
* DATE
*     July 5, 1995
*
****************************************************************************/

/* The SDL3 port does not use DPMI page locking, so keep these as no-ops. */
#define DPMI_Lock(a,b) ((void)0)
#define DPMI_Unlock(a,b) ((void)0)

#endif /* VQAMEM_H */
