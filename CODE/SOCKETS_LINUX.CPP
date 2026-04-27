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

#include "SOCKETS.H"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

int Map_Socket_Type(int type)
{
	switch (type) {
		case SOCKET_TYPE_STREAM:
			return SOCK_STREAM;
		case SOCKET_TYPE_DATAGRAM:
			return SOCK_DGRAM;
		default:
			return 0;
	}
}

int Map_Socket_Protocol(int protocol)
{
	switch (protocol) {
		case SOCKET_PROTOCOL_DEFAULT:
			return 0;
		case SOCKET_PROTOCOL_TCP:
			return IPPROTO_TCP;
		case SOCKET_PROTOCOL_UDP:
			return IPPROTO_UDP;
		default:
			return 0;
	}
}

bool Map_Socket_Option(int option_name, int* level, int* native_option)
{
	switch (option_name) {
		case SOCKET_OPTION_REUSE_ADDRESS:
			*level = SOL_SOCKET;
			*native_option = SO_REUSEADDR;
			return true;
		case SOCKET_OPTION_BROADCAST:
			*level = SOL_SOCKET;
			*native_option = SO_BROADCAST;
			return true;
		case SOCKET_OPTION_SEND_BUFFER:
			*level = SOL_SOCKET;
			*native_option = SO_SNDBUF;
			return true;
		case SOCKET_OPTION_RECEIVE_BUFFER:
			*level = SOL_SOCKET;
			*native_option = SO_RCVBUF;
			return true;
		case SOCKET_OPTION_TCP_NO_DELAY:
			*level = IPPROTO_TCP;
			*native_option = TCP_NODELAY;
			return true;
		default:
			return false;
	}
}

void Fill_Native_Address(SocketAddress const* address, sockaddr_in* native_address)
{
	std::memset(native_address, 0, sizeof(*native_address));
	native_address->sin_family = AF_INET;
	if (address != nullptr) {
		native_address->sin_addr.s_addr = address->Address;
		native_address->sin_port = address->Port;
	}
}

void Fill_Abstract_Address(sockaddr_in const* native_address, SocketAddress* address)
{
	if (address == nullptr) {
		return;
	}

	address->Address = native_address->sin_addr.s_addr;
	address->Port = native_address->sin_port;
}

int To_Native_Socket(SocketHandle socket_handle)
{
	return static_cast<int>(socket_handle);
}

SocketHandle From_Native_Socket(int socket_handle)
{
	return socket_handle < 0 ? INVALID_SOCKET_HANDLE : static_cast<SocketHandle>(socket_handle);
}

}

bool Socket_Startup(void)
{
	return true;
}

void Socket_Cleanup(void)
{
}

int Socket_Get_Last_Error(void)
{
	return errno;
}

bool Socket_Error_Is_Would_Block(int error_code)
{
	return error_code == EWOULDBLOCK || error_code == EAGAIN || error_code == EINPROGRESS || error_code == EALREADY;
}

bool Socket_Error_Is_Connection_Reset(int error_code)
{
	return error_code == ECONNRESET;
}

SocketHandle Socket_Open(int type, int protocol)
{
	const int native_type = Map_Socket_Type(type);
	const int native_protocol = Map_Socket_Protocol(protocol);
	if (native_type == 0) {
		return INVALID_SOCKET_HANDLE;
	}

	return From_Native_Socket(socket(AF_INET, native_type, native_protocol));
}

int Socket_Close(SocketHandle socket_handle)
{
	if (socket_handle == INVALID_SOCKET_HANDLE) {
		return 0;
	}

	return close(To_Native_Socket(socket_handle));
}

int Socket_Bind(SocketHandle socket_handle, SocketAddress const* address)
{
	sockaddr_in native_address;
	Fill_Native_Address(address, &native_address);
	return bind(To_Native_Socket(socket_handle), reinterpret_cast<sockaddr*>(&native_address), sizeof(native_address));
}

int Socket_Listen(SocketHandle socket_handle, int backlog)
{
	return listen(To_Native_Socket(socket_handle), backlog);
}

SocketHandle Socket_Accept(SocketHandle socket_handle, SocketAddress* address)
{
	sockaddr_in native_address;
	socklen_t address_length = sizeof(native_address);
	int accepted_socket = accept(To_Native_Socket(socket_handle), reinterpret_cast<sockaddr*>(&native_address), &address_length);
	if (accepted_socket < 0) {
		return INVALID_SOCKET_HANDLE;
	}

	Fill_Abstract_Address(&native_address, address);
	return From_Native_Socket(accepted_socket);
}

int Socket_Connect(SocketHandle socket_handle, SocketAddress const* address)
{
	sockaddr_in native_address;
	Fill_Native_Address(address, &native_address);
	return connect(To_Native_Socket(socket_handle), reinterpret_cast<sockaddr*>(&native_address), sizeof(native_address));
}

int Socket_Read(SocketHandle socket_handle, void* buffer, int buffer_length)
{
	return recv(To_Native_Socket(socket_handle), buffer, static_cast<size_t>(buffer_length), 0);
}

int Socket_Write(SocketHandle socket_handle, void const* buffer, int buffer_length)
{
	return send(To_Native_Socket(socket_handle), buffer, static_cast<size_t>(buffer_length), 0);
}

int Socket_Read_From(SocketHandle socket_handle, void* buffer, int buffer_length, SocketAddress* address)
{
	sockaddr_in native_address;
	socklen_t address_length = sizeof(native_address);
	int rc = recvfrom(To_Native_Socket(socket_handle),
		buffer,
		static_cast<size_t>(buffer_length),
		0,
		reinterpret_cast<sockaddr*>(&native_address),
		&address_length);
	if (rc >= 0) {
		Fill_Abstract_Address(&native_address, address);
	}
	return rc;
}

int Socket_Write_To(SocketHandle socket_handle, void const* buffer, int buffer_length, SocketAddress const* address)
{
	sockaddr_in native_address;
	Fill_Native_Address(address, &native_address);
	return sendto(To_Native_Socket(socket_handle),
		buffer,
		static_cast<size_t>(buffer_length),
		0,
		reinterpret_cast<sockaddr const*>(&native_address),
		sizeof(native_address));
}

int Socket_Set_NonBlocking(SocketHandle socket_handle, bool nonblocking)
{
	int flags = fcntl(To_Native_Socket(socket_handle), F_GETFL, 0);
	if (flags < 0) {
		return -1;
	}

	if (nonblocking) {
		flags |= O_NONBLOCK;
	} else {
		flags &= ~O_NONBLOCK;
	}

	return fcntl(To_Native_Socket(socket_handle), F_SETFL, flags);
}

int Socket_Set_Integer_Option(SocketHandle socket_handle, int option_name, int value)
{
	int level = 0;
	int native_option = 0;
	if (!Map_Socket_Option(option_name, &level, &native_option)) {
		return -1;
	}

	return setsockopt(To_Native_Socket(socket_handle), level, native_option, &value, sizeof(value));
}

int Socket_Set_Linger(SocketHandle socket_handle, bool enabled, int seconds)
{
	linger linger_value;
	linger_value.l_onoff = enabled ? 1 : 0;
	linger_value.l_linger = static_cast<int>(seconds);
	return setsockopt(To_Native_Socket(socket_handle), SOL_SOCKET, SO_LINGER, &linger_value, sizeof(linger_value));
}

int Socket_Get_Error(SocketHandle socket_handle)
{
	int error_code = 0;
	socklen_t length = sizeof(error_code);
	if (getsockopt(To_Native_Socket(socket_handle), SOL_SOCKET, SO_ERROR, &error_code, &length) != 0) {
		return errno;
	}
	return error_code;
}

void Socket_Clear_Error(SocketHandle socket_handle)
{
	Socket_Get_Error(socket_handle);
}

bool Socket_Is_Write_Ready(SocketHandle socket_handle)
{
	fd_set write_set;
	FD_ZERO(&write_set);
	FD_SET(To_Native_Socket(socket_handle), &write_set);
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	return select(To_Native_Socket(socket_handle) + 1, NULL, &write_set, NULL, &timeout) > 0;
}

bool Socket_Get_Peer_Address(SocketHandle socket_handle, SocketAddress* address)
{
	sockaddr_in native_address;
	socklen_t address_length = sizeof(native_address);
	if (getpeername(To_Native_Socket(socket_handle), reinterpret_cast<sockaddr*>(&native_address), &address_length) != 0) {
		return false;
	}

	Fill_Abstract_Address(&native_address, address);
	return true;
}

bool Socket_Get_Host_Name(char* name, int name_length)
{
	return gethostname(name, static_cast<size_t>(name_length)) == 0;
}

bool Socket_Lookup_Host_By_Name(char const* name, SocketHostEntry* entry)
{
	if (name == nullptr || entry == nullptr) {
		return false;
	}

	addrinfo hints;
	std::memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	addrinfo* result = nullptr;
	if (getaddrinfo(name, nullptr, &hints, &result) != 0) {
		return false;
	}

	std::memset(entry, 0, sizeof(*entry));
	strncpy(entry->Name, name, sizeof(entry->Name) - 1);
	for (addrinfo* cursor = result; cursor != nullptr && entry->AddressCount < SOCKET_MAX_HOST_ADDRESSES; cursor = cursor->ai_next) {
		sockaddr_in* native_address = reinterpret_cast<sockaddr_in*>(cursor->ai_addr);
		entry->Addresses[entry->AddressCount++] = native_address->sin_addr.s_addr;
	}

	freeaddrinfo(result);
	return entry->AddressCount > 0;
}

bool Socket_Lookup_Host_By_Address(uint32_t address, char* name, int name_length)
{
	if (name == nullptr || name_length <= 0) {
		return false;
	}

	sockaddr_in native_address;
	std::memset(&native_address, 0, sizeof(native_address));
	native_address.sin_family = AF_INET;
	native_address.sin_addr.s_addr = address;

	return getnameinfo(reinterpret_cast<sockaddr*>(&native_address),
		sizeof(native_address),
		name,
		static_cast<socklen_t>(name_length),
		nullptr,
		0,
		0) == 0;
}

bool Socket_Address_From_String(char const* text, uint32_t* address)
{
	if (text == nullptr || address == nullptr) {
		return false;
	}

	in_addr native_address;
	if (inet_pton(AF_INET, text, &native_address) != 1) {
		return false;
	}

	*address = native_address.s_addr;
	return true;
}

bool Socket_Address_To_String(uint32_t address, char* buffer, int buffer_length)
{
	if (buffer == nullptr || buffer_length <= 0) {
		return false;
	}

	in_addr native_address;
	native_address.s_addr = address;
	return inet_ntop(AF_INET, &native_address, buffer, static_cast<socklen_t>(buffer_length)) != nullptr;
}

bool Socket_Address_Is_Private(uint32_t address)
{
	const uint32_t host_address = Socket_Network_To_Host32(address);
	return ((host_address & 0xff000000u) == 0x0a000000u) ||
		((host_address & 0xfff00000u) == 0xac100000u) ||
		((host_address & 0xffff0000u) == 0xc0a80000u);
}

uint16_t Socket_Host_To_Network16(uint16_t value)
{
	return htons(value);
}

uint16_t Socket_Network_To_Host16(uint16_t value)
{
	return ntohs(value);
}

uint32_t Socket_Host_To_Network32(uint32_t value)
{
	return htonl(value);
}

uint32_t Socket_Network_To_Host32(uint32_t value)
{
	return ntohl(value);
}
