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
 **     C O N F I D E N T I A L --- W E S T W O O D   S T U D I O S       **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : wwstd.h                                  *
 *                                                                         *
 *                    File Name : WWLIB.H                                  *
 *                                                                         *
 *                   Programmer : Jeff Wilson                              *
 *                                                                         *
 *                   Start Date : March 1, 1994                            *
 *                                                                         *
 *                  Last Update : March 1, 1994   []                       *
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef WWSTD_H
#define	WWSTD_H


//
// Win 95 includes
//



// Note: SKB 4/11/94
// Before this library is done, this needs to be able to be set to TRUE.
// Once it is, the FALSE parts should be removed from the source code.
#define LIB_EXTERNS_RESOLVED FALSE


#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "win32_compat.h"
//================================

// TRUE and FALSE are defined in pltypes.h

#ifndef IBM
#define IBM				TRUE
#endif

#ifndef AMIGA
#define AMIGA			FALSE
#endif

#ifndef	SEGA
#define	SEGA			FALSE
#endif

/*
**	Common constants used in normal code.
*/
#define WW_ERROR		-1

#ifdef NULL
#undef	NULL
#endif

#ifndef NULL
#define NULL		0L
#endif

#define PRIVATE		static
#define PUBLIC			/* Routines & data don't have a specifier */

#ifdef __cplusplus
#define	__CPPARGS	...
#else
#define	__CPPARGS
#endif

// This macro will get the size (in elements) of the specified array.
#ifdef GET_SIZE
#undef GET_SIZE
#endif
#define GET_SIZE(a)					((sizeof(a) / sizeof(*a)))

// Returns the absolute value of the number.
#ifdef ABS
#undef ABS
#endif
template<class T> T ABS(T a)
{
	return (a < 0) ? -a : a;
}
inline int32_t ABS(int32_t a)
{
	return ABS<int32_t>(a);
}

// Returns the minimum of the two numbers.
#ifdef MIN
#undef MIN
#endif
template<class T> T MIN(T a, T b)
{
	return (b < a) ? b : a;
};
inline int16_t MIN(int16_t a, int16_t b)
{
	return MIN<int16_t>(a, b);
}
inline int32_t MIN(int32_t a, int32_t b)
{
	return MIN<int32_t>(a, b);
}

// Returns the maximum of the two numbers.
#ifdef MAX
#undef MAX
#endif
template<class T> T MAX(T a, T b)
{
	return (b > a) ? b : a;
};
inline int16_t MAX(int16_t a, int16_t b)
{
	return MAX<int16_t>(a, b);
}
inline int32_t MAX(int32_t a, int32_t b)
{
	return MAX<int32_t>(a, b);
}

// Returns the low word of a long
#define	LOW_WORD(a)		((uint16_t)((uint32_t)(a) & UINT32_C(0x0000FFFF)))

// Returns the high word of a long
#define	HIGH_WORD(a)	((uint16_t)(((uint32_t)(a) >> 16) & UINT32_C(0x0000FFFF)))

// Merges to shorts to become a long
#define	MAKE_LONG(a,b) ((int32_t)(((uint32_t)(uint16_t)(a) << 16) | (uint32_t)(uint16_t)(b)))

/*
** Macro allows our routines to act like
** sprintf etc..
*/
#ifdef AssembleTo
#undef AssembleTo
#endif

#define AssembleTo(dest,fmt)\
{\
	va_list argptr;\
	if (fmt != (dest))\
		{\
		va_start (argptr, fmt);\
		vsprintf ((dest), fmt, argptr);\
		va_end (argptr);\
		}\
}

#ifndef PRIVATE
#define  PRIVATE	static
#endif

// The purpose of the INT and UINT is for efficiency.  It says that while a short int (16 bit)
// has enough precision, it is more efficient to pass in an int (32 bits).  For efficiency, most
// WORD and UWORD should be an INT or UINT, especially on the stack and structures that will
// not be in large arrays.  When size efficiency is more important then speed, use WORD UWORD.

#define	TRUE		1
#define	FALSE		0

/*
**	The type of processor running on this system as
**	returned by Processor().
*/
#define	PROC_80386		0
#define	PROC_80486		1
#define	PROC_PENTIUM	2


// Inline Routines
//����������������
//
// These Template functions are generally used
// by classes when they havce over loaded > and <.
//
#ifdef __cplusplus
template<class T> T Min(T a, T b)
{
	return (a<b ? a : b);
}

template<class T> inline  T Max(T a, T b)
{
	return (a>b ? a : b);
}

template<class T> T Abs(T a)
{
	return ((a<0) ? -(a) : a);
}

template<class T> void minimize(T &a, T b)
{
	if( b<a )
		a=b;
}

template<class T> void maximize(T &a, T b)
{
	if( b>a )
		a=b;
}
#endif

/*
**	Macros that control bit settings in a variable.
*/
#define Bit_Flags_On(a,b)			a |= (b)
#define Bit_Flags_Off(a,b)			a &= (~(b))
#define Bit_Flags_Value(a,b)		(a & (b))
#define Bit_Flags_Flip(a,b)		a ^= (b)

// Template replacements for the user defines above
#ifdef __cplusplus
template<class T> void BitFlagsOn(T &a, T b)
{
	a |= (b);
}

template<class T> void BitFlagsOff(T &a, T b)
{
	a &= (~(b));
}

template<class T> T BitFlagsValue(T a, T b)
{
	return (a & (b));
}

template<class T> void BitFlagsFlip(T &a, T b)
{
	a ^= (b);
}
#endif

typedef enum {
	TBLACK,
	PURPLE,
	CYAN,
	GREEN,
	LTGREEN,
	YELLOW,
	PINK,
	BROWN,
	RED,
	LTCYAN,
	LTBLUE,
	BLUE,
	BLACK,
	GREY,
	LTGREY,
	WHITE,
	COLOR_PADDING=0x1000
} ColorType;


#endif
