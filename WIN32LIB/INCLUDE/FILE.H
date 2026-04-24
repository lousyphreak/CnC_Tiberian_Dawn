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
 **   C O N F I D E N T I A L --- W E S T W O O D   A S S O C I A T E S   **
 ***************************************************************************
 *                                                                         *
 *                 Project Name : Library - Filio header stuff.            *
 *                                                                         *
 *                    File Name : FILE.H                                 	*
 *                                                                         *
 *                   Programmer : Scott K. Bowen                           *
 *                                                                         *
 *                   Start Date : September 13, 1993                       *
 *                                                                         *
 *                  Last Update : April 11, 1994									*
 *                                                                         *
 *-------------------------------------------------------------------------*
 * Functions:                                                              *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#ifndef FILE_H
#define FILE_H

#ifndef FILETEMP_H
// This should be removed once the library is all intacked.
#include "FILETEMP.H"
#endif

/*=========================================================================*/
/* File IO system defines and enumerations											*/
/*=========================================================================*/

#define	XMAXPATH	80

/*
**	These are the Open_File, Read_File, and Seek_File constants.
*/
#ifndef READ
#define READ						1	// Read access.
#endif
#ifndef WRITE
#define WRITE						2	// Write access.
#endif
#ifndef SEEK_SET
#define SEEK_SET					0	// Seek from start of file.
#define SEEK_CUR					1	// Seek relative from current location.
#define SEEK_END					2	// Seek from end of file.
#endif


typedef enum {
	FILEB_PROCESSED=8,// Was the packed file header of this file processed?
	FILEB_PRELOAD,		// Scan for and make file resident at WWDOS_Init time?
	FILEB_RESIDENT,	// Make resident at Open_File time?
	FILEB_FLUSH,		// Un-resident at Close_File time?
	FILEB_PACKED,		// Is this file packed?
	FILEB_KEEP,			// Don't ever flush this resident file?
	FILEB_PRIORITY,	// Flush this file last?

	FILEB_LAST
} FileFlags_Type;

#define	FILEF_NONE			0
#define	FILEF_PROCESSED	(1<<FILEB_PROCESSED)
#define	FILEF_PRELOAD		(1<<FILEB_PRELOAD)
#define	FILEF_RESIDENT		(1<<FILEB_RESIDENT)
#define	FILEF_FLUSH			(1<<FILEB_FLUSH)
#define	FILEF_PACKED		(1<<FILEB_PACKED)
#define	FILEF_KEEP			(1<<FILEB_KEEP)
#define	FILEF_PRIORITY		(1<<FILEB_PRIORITY)

/*
** These errors are returned by WWDOS_Init().  All errors encountered are
** or'd together so there may be more then one error returned.  Not all
** errors are fatal, such as the cache errors.
*/
typedef enum {
	FI_SUCCESS						= 0x00,
	FI_CACHE_TOO_BIG				= 0x01,
	FI_CACHE_ALREADY_INIT		= 0x02,
	FI_FILEDATA_FILE_NOT_FOUND	= 0x04,
	FI_FILEDATA_TOO_BIG			= 0x08,
 	FI_SEARCH_PATH_NOT_FOUND	= 0x10,
 	FI_STARTUP_PATH_NOT_FOUND	= 0x20,
	FI_NO_CACHE_FOR_PRELOAD		= 0x40,
	FI_FILETABLE_NOT_INIT		= 0x80,
} FileInitErrorType;


/*
**	These are the errors that are detected by the File I/O system and
**	passed to the io error routine.
*/
//lint -strong(AJX,FileErrorType)
typedef enum {
	CANT_CREATE_FILE,
	BAD_OPEN_MODE,
	COULD_NOT_OPEN,
	TOO_MANY_FILES,
	CLOSING_NON_HANDLE,
	READING_NON_HANDLE,
	WRITING_NON_HANDLE,
	SEEKING_NON_HANDLE,
	SEEKING_BAD_OFFSET,
	WRITING_RESIDENT,
	UNKNOWN_INDEX,
	DID_NOT_CLOSE,
	FATAL_ERROR,
	FILE_NOT_LISTED,
	FILE_LENGTH_MISMATCH,
	INTERNAL_ERROR,
	MAKE_RESIDENT_ZERO_SIZE,
	RESIDENT_SORT_FAILURE,

	NUMBER_OF_ERRORS				/* MAKE SURE THIS IS THE LAST ENTRY */
} FileErrorType;

// This is here temporarily until library is put together.
extern int16_t (*Open_Error)(FileErrorType, char const *);

/*=========================================================================*/
/* File IO system structures																*/
/*=========================================================================*/

//lint -strong(AJX,FileDataType)
typedef struct {
	char	*Name;		// File name (include sub-directory but not volume).
	int32_t	Size;			// File size (0=indeterminate).
	void	*Ptr;			// Resident file pointer.
	int32_t	Start;		// Starting offset in DOS handle file.
	uint8_t	Disk;			// Disk number location.
	uint8_t	OpenCount;	// Count of open locks on resident file.
	uint16_t	Flag;			// File control flags.
} FileDataType;


/*=========================================================================*/
/* FIle IO system globals.																	*/
/*=========================================================================*/

// These are C++ declarations, so data declarations do not need legacy type modifiers.
extern FileDataType FileData[];
extern char ExecPath[XMAXPATH + 1];
extern char DataPath[XMAXPATH + 1];
extern char StartPath[XMAXPATH + 1];
extern int32_t UseCD;



/*=========================================================================*/
/* The following prototypes are for the file: FILEINIT.CPP						*/
/*=========================================================================*/

void  WWDOS_Shutdown(void);
FileInitErrorType  WWDOS_Init(uint32_t cachesize, char *filedata, char *cdpath);


/*=========================================================================*/
/* The following prototypes are for the file: FILE.CPP							*/
/*=========================================================================*/

int32_t  Open_File(char const *file_name, int32_t mode);
void  Close_File(int32_t handle);
int32_t  Read_File(int32_t handle, void *buf, uint32_t bytes);
int32_t  Load_File ( const char *file_name , void *load_addr);
int32_t  Write_File(int32_t handle, void const *buf, uint32_t bytes);
uint32_t  Seek_File(int32_t handle, int32_t offset, int32_t starting);
int32_t  File_Exists(char const *file_name);
uint32_t  File_Size(int32_t handle);
int32_t  Is_Handle_Valid(int32_t handle, FileErrorType error, char const *name);
int32_t  Open_File_With_Recovery( char const *file_name, uint32_t mode );


/*=========================================================================*/
/* The following prototypes are for the file: FILECACH.CPP						*/
/*=========================================================================*/

void Unfragment_File_Cache(void);
int32_t  Make_File_Resident(char const *filename);
int32_t  Flush_Unused_File_Cache(int32_t flush_keeps);
int32_t  Free_Resident_File(char const *file);


/*=========================================================================*/
/* The following prototypes are for the file: FILECHNG.CPP						*/
/*=========================================================================*/

int32_t  Create_File(char const *file_name);
int32_t  Delete_File(char const *file_name);
int32_t  Change_File_Size(int32_t handle, uint32_t new_size);


/*=========================================================================*/
/* The following prototypes are for the file: FILEINFO.CPP						*/
/*=========================================================================*/

int32_t  Get_DOS_Handle(int32_t fh);
int32_t  Free_Handles(void);
int32_t  Find_Disk_Number(char const *file_name);
int32_t  Set_File_Flags(char const *filename, int32_t flags);
int32_t  Clear_File_Flags(char const *filename, int32_t flags);
int32_t  Get_File_Flags(char const *filename);
int32_t  Multi_Drive_Search(int32_t on);


/*=========================================================================*/
/* The following prototypes are for the file: FINDFILE.CPP						*/
/*=========================================================================*/

int32_t  Find_File(char const *file_name);
int32_t  Find_File_Index(char const *filename);






#endif
