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

#include "IPX.H"
#include "win32_compat.h"
#include "IPX95.H"

#include <string.h>

void CCDebugString(char *string);

namespace {

void Log_IPX95_Unavailable(void)
{
	static bool logged = false;

	if (!logged) {
		char message[] = "IPX95 backend unavailable; legacy IPX multiplayer disabled.";
		CCDebugString(message);
		logged = true;
	}
}

}

extern "C" BOOL __stdcall IPX_Initialise(void)
{
	Log_IPX95_Unavailable();
	return 0;
}

extern "C" BOOL __stdcall IPX_Get_Outstanding_Buffer95(unsigned char *)
{
	return 0;
}

extern "C" void __stdcall IPX_Shut_Down95(void)
{
}

extern "C" int __stdcall IPX_Send_Packet95(unsigned char *, unsigned char *, int, unsigned char*, unsigned char*)
{
	return 0;
}

extern "C" int __stdcall IPX_Broadcast_Packet95(unsigned char *, int)
{
	return 0;
}

extern "C" BOOL __stdcall IPX_Start_Listening95(void)
{
	return 0;
}

extern "C" int __stdcall IPX_Open_Socket95(int)
{
	return IPXERR_SOCKET_ERROR;
}

extern "C" void __stdcall IPX_Close_Socket95(int)
{
}

extern "C" int __stdcall IPX_Get_Connection_Number95(void)
{
	return 0;
}

extern "C" int __stdcall IPX_Get_Local_Target95(unsigned char *, unsigned char *node, unsigned short, unsigned char *immediate)
{
	if (node != NULL && immediate != NULL) {
		memcpy(immediate, node, 6);
	}

	return 0;
}
