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

#ifndef SOCKETS_H
#define SOCKETS_H

#include <cstddef>
#include <cstdint>

using SocketHandle = intptr_t;

static constexpr SocketHandle INVALID_SOCKET_HANDLE = static_cast<SocketHandle>(-1);

static constexpr int SOCKET_TYPE_STREAM = 1;
static constexpr int SOCKET_TYPE_DATAGRAM = 2;

static constexpr int SOCKET_PROTOCOL_DEFAULT = 0;
static constexpr int SOCKET_PROTOCOL_TCP = 1;
static constexpr int SOCKET_PROTOCOL_UDP = 2;

static constexpr int SOCKET_OPTION_REUSE_ADDRESS = 1;
static constexpr int SOCKET_OPTION_BROADCAST = 2;
static constexpr int SOCKET_OPTION_SEND_BUFFER = 3;
static constexpr int SOCKET_OPTION_RECEIVE_BUFFER = 4;
static constexpr int SOCKET_OPTION_TCP_NO_DELAY = 5;

static constexpr uint32_t SOCKET_ADDRESS_ANY = 0x00000000u;
static constexpr uint32_t SOCKET_ADDRESS_BROADCAST = 0xffffffffu;

static constexpr int SOCKET_MAX_HOST_NAME = 256;
static constexpr int SOCKET_MAX_DOT_ADDRESS = 16;
static constexpr int SOCKET_MAX_HOST_ADDRESSES = 16;

struct SocketAddress {
	uint32_t Address;
	uint16_t Port;
};

struct SocketHostEntry {
	char Name[SOCKET_MAX_HOST_NAME];
	int AddressCount;
	uint32_t Addresses[SOCKET_MAX_HOST_ADDRESSES];
};

bool Socket_Startup(void);
void Socket_Cleanup(void);

int Socket_Get_Last_Error(void);
bool Socket_Error_Is_Would_Block(int error_code);
bool Socket_Error_Is_Connection_Reset(int error_code);

SocketHandle Socket_Open(int type, int protocol);
int Socket_Close(SocketHandle socket_handle);
int Socket_Bind(SocketHandle socket_handle, SocketAddress const* address);
int Socket_Listen(SocketHandle socket_handle, int backlog);
SocketHandle Socket_Accept(SocketHandle socket_handle, SocketAddress* address);
int Socket_Connect(SocketHandle socket_handle, SocketAddress const* address);
int Socket_Read(SocketHandle socket_handle, void* buffer, int buffer_length);
int Socket_Write(SocketHandle socket_handle, void const* buffer, int buffer_length);
int Socket_Read_From(SocketHandle socket_handle, void* buffer, int buffer_length, SocketAddress* address);
int Socket_Write_To(SocketHandle socket_handle, void const* buffer, int buffer_length, SocketAddress const* address);

int Socket_Set_NonBlocking(SocketHandle socket_handle, bool nonblocking);
int Socket_Set_Integer_Option(SocketHandle socket_handle, int option_name, int value);
int Socket_Set_Linger(SocketHandle socket_handle, bool enabled, int seconds);
int Socket_Get_Error(SocketHandle socket_handle);
void Socket_Clear_Error(SocketHandle socket_handle);
bool Socket_Is_Write_Ready(SocketHandle socket_handle);
bool Socket_Get_Peer_Address(SocketHandle socket_handle, SocketAddress* address);

bool Socket_Get_Host_Name(char* name, int name_length);
bool Socket_Lookup_Host_By_Name(char const* name, SocketHostEntry* entry);
bool Socket_Lookup_Host_By_Address(uint32_t address, char* name, int name_length);
bool Socket_Address_From_String(char const* text, uint32_t* address);
bool Socket_Address_To_String(uint32_t address, char* buffer, int buffer_length);
bool Socket_Address_Is_Private(uint32_t address);

uint16_t Socket_Host_To_Network16(uint16_t value);
uint16_t Socket_Network_To_Host16(uint16_t value);
uint32_t Socket_Host_To_Network32(uint32_t value);
uint32_t Socket_Network_To_Host32(uint32_t value);

#endif
