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

#include <ctype.h>
#include <stdint.h>
#include <string.h>

void strtrim(char* buffer)
{
	if (buffer == nullptr) {
		return;
	}

	char* source = buffer;
	while (isspace(static_cast<uint8_t>(*source))) {
		++source;
	}
	if (source != buffer) {
		memmove(buffer, source, strlen(source) + 1);
	}

	int length = static_cast<int>(strlen(buffer));
	while (length > 0 && isspace(static_cast<uint8_t>(buffer[length - 1]))) {
		buffer[--length] = '\0';
	}
}
