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
#include "SOCKETS.H"

#include <cstdint>
#include <cstring>

void CCDebugString(char const *string);

namespace {

SocketHandle IpxSocket = INVALID_SOCKET_HANDLE;
uint16_t IpxPort = 0;
uint32_t LocalAddress = SOCKET_ADDRESS_ANY;
unsigned char LocalNode[6] = {0, 0, 0, 0, 0, 0};
bool BackendInitialised = false;

bool Has_NonZero_Bytes(unsigned char const* data, size_t length)
{
	for (size_t index = 0; index < length; ++index) {
		if (data[index] != 0) {
			return true;
		}
	}

	return false;
}

bool Is_All_Bytes(unsigned char const* data, size_t length, unsigned char value)
{
	for (size_t index = 0; index < length; ++index) {
		if (data[index] != value) {
			return false;
		}
	}

	return true;
}

void Log_IPX95_Modernised(void)
{
	static bool logged = false;

	if (!logged) {
		char message[] = "IPX95 LAN path is using the SDL socket UDP backend.";
		CCDebugString(message);
		logged = true;
	}
}

void Reset_Local_Node(void)
{
	std::memset(LocalNode, 0, sizeof(LocalNode));
	std::memcpy(LocalNode, &LocalAddress, sizeof(LocalAddress));
	LocalNode[4] = static_cast<unsigned char>(IpxPort & 0x00ffu);
	LocalNode[5] = static_cast<unsigned char>((IpxPort >> 8) & 0x00ffu);
}

bool Resolve_Local_Address(void)
{
	char host_name[SOCKET_MAX_HOST_NAME] = {0};
	SocketHostEntry host_entry;

	LocalAddress = SOCKET_ADDRESS_ANY;

	if (Socket_Get_Host_Name(host_name, sizeof(host_name))
		&& Socket_Lookup_Host_By_Name(host_name, &host_entry)
		&& host_entry.AddressCount > 0) {
		for (int index = 0; index < host_entry.AddressCount; ++index) {
			uint32_t candidate = host_entry.Addresses[index];
			uint32_t host_order = Socket_Network_To_Host32(candidate);
			if ((host_order >> 24) != 127) {
				LocalAddress = candidate;
				break;
			}
		}
		if (LocalAddress == SOCKET_ADDRESS_ANY) {
			LocalAddress = host_entry.Addresses[0];
		}
	}

	if (LocalAddress == SOCKET_ADDRESS_ANY) {
		Socket_Address_From_String("127.0.0.1", &LocalAddress);
	}

	Reset_Local_Node();
	return true;
}

void Close_Ipx_Socket(void)
{
	if (IpxSocket != INVALID_SOCKET_HANDLE) {
		Socket_Close(IpxSocket);
		IpxSocket = INVALID_SOCKET_HANDLE;
	}
	IpxPort = 0;
	Reset_Local_Node();
}

bool Ensure_Backend_Initialised(void)
{
	if (BackendInitialised) {
		return true;
	}

	if (!Socket_Startup()) {
		return false;
	}

	if (!Resolve_Local_Address()) {
		return false;
	}

	BackendInitialised = true;
	Log_IPX95_Modernised();
	return true;
}

uint32_t Select_Target_Address(unsigned char const* network, unsigned char const* node)
{
	uint32_t address = SOCKET_ADDRESS_ANY;

	if (node != NULL && !Is_All_Bytes(node, 6, 0xff)) {
		std::memcpy(&address, node, sizeof(address));
	}

	if (address == SOCKET_ADDRESS_ANY && network != NULL && Has_NonZero_Bytes(network, 4) && !Is_All_Bytes(network, 4, 0xff)) {
		std::memcpy(&address, network, sizeof(address));
	}

	return address;
}

void Fill_Ipx_Header(IPXHeaderType& header, unsigned char const* network, unsigned char const* node, uint16_t payload_length)
{
	std::memset(&header, 0, sizeof(header));
	header.CheckSum = 0xffff;
	header.Length = Socket_Host_To_Network16(static_cast<uint16_t>(sizeof(IPXHeaderType) + payload_length));
	header.PacketType = 4;
	header.DestNetworkSocket = Socket_Host_To_Network16(IpxPort);
	header.SourceNetworkSocket = Socket_Host_To_Network16(IpxPort);
	std::memcpy(header.SourceNetworkNode, LocalNode, sizeof(LocalNode));

	if (network != NULL) {
		std::memcpy(header.DestNetworkNumber, network, sizeof(header.DestNetworkNumber));
	}

	if (node != NULL) {
		std::memcpy(header.DestNetworkNode, node, sizeof(header.DestNetworkNode));
	}
}

int Send_Ipx_Frame(unsigned char const* network, unsigned char const* node, unsigned char const* payload, int payload_length)
{
	if (IpxSocket == INVALID_SOCKET_HANDLE || payload == NULL || payload_length < 0) {
		return IPXERR_SOCKET_ERROR;
	}

	bool const broadcast = (node != NULL && Is_All_Bytes(node, 6, 0xff))
		|| (network != NULL && Is_All_Bytes(network, 4, 0xff));
	SocketAddress destination;
	destination.Address = broadcast ? SOCKET_ADDRESS_BROADCAST : Select_Target_Address(network, node);
	destination.Port = Socket_Host_To_Network16(IpxPort);

	if (!broadcast && destination.Address == SOCKET_ADDRESS_ANY) {
		return IPXERR_NO_PATH;
	}

	unsigned char* packet = new unsigned char[sizeof(IPXHeaderType) + payload_length];
	IPXHeaderType* header = reinterpret_cast<IPXHeaderType*>(packet);
	Fill_Ipx_Header(*header, network, node, static_cast<uint16_t>(payload_length));
	std::memcpy(packet + sizeof(IPXHeaderType), payload, static_cast<size_t>(payload_length));

	int rc = Socket_Write_To(IpxSocket, packet, static_cast<int>(sizeof(IPXHeaderType) + payload_length), &destination);
	delete[] packet;

	return rc >= 0 ? 0 : IPXERR_CONNECTION_FAILED;
}

bool Is_Local_Source(IPXHeaderType const& header)
{
	return std::memcmp(header.SourceNetworkNode, LocalNode, sizeof(LocalNode)) == 0;
}

}

extern "C" BOOL IPX_Initialise(void)
{
	return Ensure_Backend_Initialised() ? 1 : 0;
}

extern "C" BOOL IPX_Get_Outstanding_Buffer95(unsigned char *buffer)
{
	if (IpxSocket == INVALID_SOCKET_HANDLE || buffer == NULL) {
		return 0;
	}

	for (;;) {
		SocketAddress source = {0, 0};
		unsigned char packet[sizeof(IPXHeaderType) + 1024];
		int rc = Socket_Read_From(IpxSocket, packet, sizeof(packet), &source);
		if (rc <= 0) {
			int error_code = Socket_Get_Last_Error();
			if (rc == 0 || Socket_Error_Is_Would_Block(error_code) || Socket_Error_Is_Connection_Reset(error_code)) {
				return 0;
			}
			Socket_Clear_Error(IpxSocket);
			return 0;
		}

		if (rc < static_cast<int>(sizeof(IPXHeaderType))) {
			continue;
		}

		IPXHeaderType const& header = *reinterpret_cast<IPXHeaderType const*>(packet);
		if (Is_Local_Source(header)) {
			continue;
		}

		std::memcpy(buffer, packet, static_cast<size_t>(rc));
		return 1;
	}
}

extern "C" void IPX_Shut_Down95(void)
{
	Close_Ipx_Socket();
}

extern "C" int IPX_Send_Packet95(unsigned char *, unsigned char *buffer, int buffer_length, unsigned char* net, unsigned char* node)
{
	return Send_Ipx_Frame(net, node, buffer, buffer_length);
}

extern "C" int IPX_Broadcast_Packet95(unsigned char *buffer, int buffer_length)
{
	unsigned char broadcast_net[4] = {0xff, 0xff, 0xff, 0xff};
	unsigned char broadcast_node[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
	return Send_Ipx_Frame(broadcast_net, broadcast_node, buffer, buffer_length);
}

extern "C" BOOL IPX_Start_Listening95(void)
{
	return IpxSocket != INVALID_SOCKET_HANDLE;
}

extern "C" int IPX_Open_Socket95(int socket_number)
{
	SocketAddress bind_address;

	if (!Ensure_Backend_Initialised() || socket_number <= 0) {
		return IPXERR_SOCKET_ERROR;
	}

	if (IpxSocket != INVALID_SOCKET_HANDLE && IpxPort == static_cast<uint16_t>(socket_number)) {
		return 0;
	}

	Close_Ipx_Socket();

	IpxPort = static_cast<uint16_t>(socket_number);
	Reset_Local_Node();

	IpxSocket = Socket_Open(SOCKET_TYPE_DATAGRAM, SOCKET_PROTOCOL_UDP);
	if (IpxSocket == INVALID_SOCKET_HANDLE) {
		Close_Ipx_Socket();
		return IPXERR_SOCKET_ERROR;
	}

	Socket_Set_Integer_Option(IpxSocket, SOCKET_OPTION_REUSE_ADDRESS, 1);
	Socket_Set_Integer_Option(IpxSocket, SOCKET_OPTION_BROADCAST, 1);
	Socket_Set_NonBlocking(IpxSocket, true);

	bind_address.Address = SOCKET_ADDRESS_ANY;
	bind_address.Port = Socket_Host_To_Network16(IpxPort);
	if (Socket_Bind(IpxSocket, &bind_address) != 0) {
		Close_Ipx_Socket();
		return IPXERR_SOCKET_ERROR;
	}

	return 0;
}

extern "C" void IPX_Close_Socket95(int)
{
	Close_Ipx_Socket();
}

extern "C" int IPX_Get_Connection_Number95(void)
{
	return BackendInitialised ? 1 : 0;
}

extern "C" int IPX_Get_Local_Target95(unsigned char *, unsigned char *node, unsigned short, unsigned char *immediate)
{
	if (node != NULL && immediate != NULL) {
		memcpy(immediate, node, 6);
	}

	return 0;
}
