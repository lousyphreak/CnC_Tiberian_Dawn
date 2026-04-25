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

/* $Header:   F:\projects\c&c\vcs\code\rawfile.cpv   2.18   16 Oct 1995 16:50:54   JOE_BOSTIC  $ */
/***********************************************************************************************
 ***             C O N F I D E N T I A L  ---  W E S T W O O D   S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Westwood Library                                             *
 *                                                                                             *
 *                    File Name : RAWFILE.CPP                                                  *
 *                                                                                             *
 *                   Programmer : Joe L. Bostic                                                *
 *                                                                                             *
 *                   Start Date : August 8, 1994                                               *
 *                                                                                             *
 *                  Last Update : October 18, 1994   [JLB]                                     *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 * Functions:                                                                                  *
 *   RawFileClass::Bias -- Bias a file with a specific starting position and length.           *
 *   RawFileClass::Close -- Perform a closure of the file.                                     *
 *   RawFileClass::Create -- Creates an empty file.                                            *
 *   RawFileClass::Delete -- Deletes the file object from the disk.                            *
 *   RawFileClass::Error -- Handles displaying a file error message.                           *
 *   RawFileClass::Get_Date_Time -- Gets the date and time the file was last modified.         *
 *   RawFileClass::Is_Available -- Checks to see if the specified file is available to open.   *
 *   RawFileClass::Open -- Assigns name and opens file in one operation.                       *
 *   RawFileClass::Open -- Opens the file object with the rights specified.                    *
 *   RawFileClass::RawFileClass -- Simple constructor for a file object.                       *
 *   RawFileClass::Raw_Seek -- Performs a seek on the unbiased file.                           *
 *   RawFileClass::Read -- Reads the specified number of bytes into a memory buffer.           *
 *   RawFileClass::Seek -- Reposition the file pointer as indicated.                           *
 *   RawFileClass::Set_Date_Time -- Sets the date and time the file was last modified.         *
 *   RawFileClass::Set_Name -- Manually sets the name for a file object.                       *
 *   RawFileClass::Size -- Determines size of file (in bytes).                                 *
 *   RawFileClass::Write -- Writes the specified data to the buffer specified.                 *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "RAWFILE.H"
#include "FUNCTION.H"
#include "sdl_fs.h"

namespace {

uint16_t Pack_DOS_Date(const SDL_DateTime& date_time)
{
	return static_cast<uint16_t>(((date_time.year - 1980) << 9) | (date_time.month << 5) | date_time.day);
}

uint16_t Pack_DOS_Time(const SDL_DateTime& date_time)
{
	return static_cast<uint16_t>((date_time.hour << 11) | (date_time.minute << 5) | (date_time.second / 2));
}

void Build_File_Error_Message(char* message, size_t message_size, int error, char const* filename, int canretry)
{
	if (!message || message_size == 0) {
		return;
	}

#ifdef GERMAN
	std::snprintf(message, message_size, "DATEIFEHLER");
#else
#ifdef FRENCH
	std::snprintf(message, message_size, "ERREUR DE FICHIER");
#else
	std::snprintf(message, message_size, "FILE ERROR");
#endif
#endif

	if (filename && filename[0] != '\0') {
		std::strncat(message, " (", message_size - std::strlen(message) - 1);
		std::strncat(message, filename, message_size - std::strlen(message) - 1);
		std::strncat(message, ")", message_size - std::strlen(message) - 1);
	}

	std::strncat(message, ": ", message_size - std::strlen(message) - 1);
	std::strncat(message, std::strerror(error), message_size - std::strlen(message) - 1);

	if (canretry) {
		std::strncat(message, ". Retry requested.", message_size - std::strlen(message) - 1);
	}
}

} // namespace

RawFileClass::RawFileClass(char const* filename)
	: Rights(READ)
	, BiasStart(0)
	, BiasLength(-1)
	, Handle(nullptr)
	, Filename(filename)
	, Date(0)
	, Time(0)
	, Allocated(false)
{
}

RawFileClass::RawFileClass(RawFileClass const& file)
	: Rights(file.Rights)
	, BiasStart(file.BiasStart)
	, BiasLength(file.BiasLength)
	, Handle(nullptr)
	, Filename(NULL)
	, Date(file.Date)
	, Time(file.Time)
	, Allocated(false)
{
	if (file.Filename) {
		Set_Name(file.Filename);
	}
}

RawFileClass& RawFileClass::operator=(RawFileClass const& file)
{
	if (this == &file) {
		return *this;
	}

	Close();
	if (Allocated && Filename) {
		free((void*)Filename);
	}

	Filename = NULL;
	Allocated = false;
	Rights = file.Rights;
	BiasStart = file.BiasStart;
	BiasLength = file.BiasLength;
	Date = file.Date;
	Time = file.Time;
	Handle = nullptr;

	if (file.Filename) {
		Set_Name(file.Filename);
	}

	return *this;
}

void RawFileClass::Error(int error, int canretry, char const* filename)
{
	char message[512] = {0};
	Build_File_Error_Message(message, sizeof(message), error, filename ? filename : Filename, canretry);
	if (message[0] != '\0') {
		std::fprintf(stderr, "%s\n", message);
		SDL_Log("%s", message);
	}

	if (!canretry) {
		Prog_End();
		std::exit(EXIT_FAILURE);
	}
}

char const* RawFileClass::Set_Name(char const* filename)
{
	if (Allocated && Filename) {
		free((void*)Filename);
		Filename = NULL;
		Allocated = false;
	}

	if (filename == NULL) {
		return NULL;
	}

	Bias(0);

	Filename = strdup(filename);
	if (Filename == NULL) {
		Error(ENOMEM, false, filename);
	}

	Allocated = true;
	return Filename;
}

int RawFileClass::Open(char const* filename, int rights)
{
	Set_Name(filename);
	return Open(rights);
}

int RawFileClass::Open(int rights)
{
	Close();

	if (Filename == NULL) {
		Error(ENOENT, false);
	}

	Rights = rights;

	int open_flags = WWFS_OPEN_BINARY;
	switch (rights) {
		default:
			errno = EINVAL;
			Error(EINVAL, false, Filename);
			return false;

		case READ:
			open_flags |= WWFS_OPEN_READ_ONLY;
			break;

		case WRITE:
			open_flags |= WWFS_OPEN_WRITE_ONLY | WWFS_OPEN_CREATE | WWFS_OPEN_TRUNCATE;
			break;

		case READ | WRITE:
			open_flags |= WWFS_OPEN_READ_WRITE | WWFS_OPEN_CREATE;
			break;
	}

	Handle = WWFS_OpenWithFlags(Filename, open_flags);
	if (Handle == nullptr) {
		Error(errno ? errno : ENOENT, false, Filename);
	}

	if (Handle != nullptr && (BiasStart != 0 || BiasLength != -1)) {
		Seek(0, SEEK_SET);
	}

	return true;
}

int RawFileClass::Is_Available(int forced)
{
	if (Filename == NULL) {
		return false;
	}

	if (Is_Open()) {
		return true;
	}

	if (forced) {
		RawFileClass::Open(READ);
		RawFileClass::Close();
		return true;
	}

	std::string normalized = WWFS_NormalizePath(Filename);
	SDL_PathInfo info{};
	return WWFS_GetPathInfo(normalized.c_str(), &info) ? true : false;
}

void RawFileClass::Close(void)
{
	if (!Is_Open()) {
		return;
	}

	if (!SDL_CloseIO(Handle)) {
		Error(EIO, false, Filename);
	}

	Handle = nullptr;
}

int32_t RawFileClass::Read(void* buffer, int32_t size)
{
	int32_t bytesread = 0;
	int opened = false;

	if (buffer == NULL || size <= 0) {
		return 0;
	}

	if (!Is_Open()) {
		if (!Open(READ)) {
			return 0;
		}
		opened = true;
	}

	if (BiasLength != -1) {
		const int32_t remainder = BiasLength - Seek(0);
		size = size < remainder ? size : remainder;
	}

	int32_t total = 0;
	char* read_buffer = static_cast<char*>(buffer);
	while (size > 0) {
		const size_t actual = SDL_ReadIO(Handle, read_buffer, static_cast<size_t>(size));
		if (actual == 0) {
			if (SDL_GetIOStatus(Handle) == SDL_IO_STATUS_ERROR) {
				Error(EIO, true, Filename);
				continue;
			}
			break;
		}

		read_buffer += actual;
		size -= static_cast<int32_t>(actual);
		total += static_cast<int32_t>(actual);
	}
	bytesread = total;

	if (opened) {
		Close();
	}
	return bytesread;
}

int32_t RawFileClass::Write(void const* buffer, int32_t size)
{
	int32_t byteswritten = 0;
	int opened = false;

	if (buffer == NULL || size <= 0) {
		return 0;
	}

	if (!Is_Open()) {
		if (!Open(WRITE)) {
			return 0;
		}
		opened = true;
	}

	const size_t actual = SDL_WriteIO(Handle, buffer, static_cast<size_t>(size));
	if (actual < static_cast<size_t>(size)) {
		Error(ENOSPC, false, Filename);
	}
	byteswritten = static_cast<int32_t>(actual);

	if (BiasLength != -1 && Raw_Seek(0) > BiasStart + BiasLength) {
		BiasLength = Raw_Seek(0) - BiasStart;
	}

	if (opened) {
		Close();
	}

	return byteswritten;
}

int32_t RawFileClass::Seek(int32_t pos, int dir)
{
	if (BiasLength != -1) {
		switch (dir) {
			case SEEK_SET:
				if (pos > BiasLength) {
					pos = BiasLength;
				}
				pos += BiasStart;
				break;

			case SEEK_CUR:
				break;

			case SEEK_END:
				dir = SEEK_SET;
				pos += BiasStart + BiasLength;
				break;
		}

		int32_t newpos = Raw_Seek(pos, dir) - BiasStart;
		if (newpos < 0) {
			newpos = Raw_Seek(BiasStart, SEEK_SET) - BiasStart;
		}
		if (newpos > BiasLength) {
			newpos = Raw_Seek(BiasStart + BiasLength, SEEK_SET) - BiasStart;
		}
		return newpos;
	}

	return Raw_Seek(pos, dir);
}

int32_t RawFileClass::Size(void)
{
	if (BiasLength != -1) {
		return BiasLength;
	}

	int32_t size = 0;
	if (Is_Open()) {
		const Sint64 io_size = SDL_GetIOSize(Handle);
		if (io_size < 0) {
			Error(EIO, false, Filename);
		}
		if (io_size > INT32_MAX) {
			Error(EOVERFLOW, false, Filename);
		}
		size = static_cast<int32_t>(io_size);
	} else if (Open()) {
		size = Size();
		Close();
	}

	BiasLength = size - BiasStart;
	if (BiasLength < 0) {
		BiasLength = 0;
	}
	return BiasLength;
}

int RawFileClass::Create(void)
{
	Close();
	if (Open(WRITE)) {
		if (BiasLength != -1) {
			Seek(0, SEEK_SET);
		}
		Close();
		return true;
	}

	return false;
}

int RawFileClass::Delete(void)
{
	Close();

	if (!Filename) {
		Error(ENOENT, false);
	}

	if (!Is_Available()) {
		return false;
	}

	std::string normalized = WWFS_NormalizePath(Filename);
	if (!WWFS_RemovePath(normalized.c_str())) {
		Error(EACCES, false, Filename);
		return false;
	}

	return true;
}

uint32_t RawFileClass::Get_Date_Time(void)
{
	if (Filename == NULL) {
		return 0;
	}

	std::string normalized = WWFS_NormalizePath(Filename);
	SDL_PathInfo info{};
	if (!WWFS_GetPathInfo(normalized.c_str(), &info) || info.modify_time <= 0) {
		return 0;
	}

	SDL_DateTime date_time{};
	if (!SDL_TimeToDateTime(info.modify_time, &date_time, true)) {
		return 0;
	}

	Date = Pack_DOS_Date(date_time);
	Time = Pack_DOS_Time(date_time);
	return (static_cast<uint32_t>(Date) << 16) | Time;
}

bool RawFileClass::Set_Date_Time(uint32_t)
{
	return false;
}

void RawFileClass::Set_Buffer_Size(int32_t)
{
}

void RawFileClass::Bias(int32_t start, int32_t length)
{
	if (start == 0) {
		BiasStart = 0;
		BiasLength = -1;
		return;
	}

	BiasLength = RawFileClass::Size();
	BiasStart += start;
	if (length != -1 && BiasLength > length) {
		BiasLength = length;
	}
	BiasLength = BiasLength > 0 ? BiasLength : 0;

	if (Is_Open()) {
		RawFileClass::Seek(0, SEEK_SET);
	}
}

int32_t RawFileClass::Raw_Seek(int32_t pos, int dir)
{
	if (!Is_Open()) {
		Error(EBADF, false, Filename);
	}

	SDL_IOWhence whence = SDL_IO_SEEK_SET;
	switch (dir) {
		case SEEK_SET:
			whence = SDL_IO_SEEK_SET;
			break;

		case SEEK_CUR:
			whence = SDL_IO_SEEK_CUR;
			break;

		case SEEK_END:
			whence = SDL_IO_SEEK_END;
			break;
	}

	const Sint64 newpos = SDL_SeekIO(Handle, static_cast<Sint64>(pos), whence);
	if (newpos < 0) {
		Error(EIO, false, Filename);
	}
	if (newpos > INT32_MAX) {
		Error(EOVERFLOW, false, Filename);
	}

	return static_cast<int32_t>(newpos);
}
