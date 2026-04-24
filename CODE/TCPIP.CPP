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

#include "FUNCTION.H"
#include "TCPIP.H"

namespace {

uint32_t Decode_Frame_Length(const std::vector<uint8_t>& stream_buffer)
{
	uint32_t frame_length = 0;
	memcpy(&frame_length, &stream_buffer[0], sizeof(frame_length));
	return Socket_Network_To_Host32(frame_length);
}

void Copy_Socket_String(char* destination, size_t destination_size, const char* source)
{
	if (destination_size == 0) {
		return;
	}

	if (source == nullptr) {
		destination[0] = '\0';
		return;
	}

	strncpy(destination, source, destination_size - 1);
	destination[destination_size - 1] = '\0';
}

}

bool Server;
TcpipManagerClass Winsock;

TcpipManagerClass::TcpipManagerClass(void)
{
	WinsockInitialised = FALSE;
	Connected = FALSE;
	UseUDP = TRUE;
	ListenSocket = INVALID_SOCKET_HANDLE;
	ConnectSocket = INVALID_SOCKET_HANDLE;
	UDPSocket = INVALID_SOCKET_HANDLE;
	SocketReceiveBuffer = 4096;
	SocketSendBuffer = 4096;
	ConnectInProgress = FALSE;
	StreamSendLength = 0;
	StreamSendOffset = 0;
	StreamSendActive = FALSE;
	ConnectStatus = NOT_CONNECTING;
	memset(&ClientIPAddress, 0, sizeof(ClientIPAddress));
	memset(&UDPIPAddress, 0, sizeof(UDPIPAddress));
	Server.Address = 0;
	HostAddress[0] = '\0';
	Server.Name[0] = '\0';
	Server.DotAddr[0] = '\0';
}

TcpipManagerClass::~TcpipManagerClass(void)
{
	Close();
}

void TcpipManagerClass::Close(void)
{
	if (ConnectSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
	}

	if (ListenSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ListenSocket);
		ListenSocket = INVALID_SOCKET_HANDLE;
	}

	if (UDPSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(UDPSocket);
		UDPSocket = INVALID_SOCKET_HANDLE;
	}

	if (WinsockInitialised) {
		Socket_Cleanup();
	}

	WinsockInitialised = FALSE;
	Connected = FALSE;
	ConnectInProgress = FALSE;
	StreamReceiveBuffer.clear();
	StreamSendLength = 0;
	StreamSendOffset = 0;
	StreamSendActive = FALSE;
	ConnectStatus = NOT_CONNECTING;
	memset(&ClientIPAddress, 0, sizeof(ClientIPAddress));
	memset(&UDPIPAddress, 0, sizeof(UDPIPAddress));
	Server.Address = 0;
	Server.Name[0] = '\0';
	Server.DotAddr[0] = '\0';
}

int32_t TcpipManagerClass::Init(void)
{
	if (WinsockInitialised) {
		return TRUE;
	}

	ListenSocket = INVALID_SOCKET_HANDLE;
	ConnectSocket = INVALID_SOCKET_HANDLE;
	UDPSocket = INVALID_SOCKET_HANDLE;

	if (!Socket_Startup()) {
		return FALSE;
	}

	WinsockInitialised = TRUE;
	return TRUE;
}

void TcpipManagerClass::Service(void)
{
	if (!WinsockInitialised) {
		return;
	}

	if (UseUDP) {
		if (!Connected || UDPSocket == INVALID_SOCKET_HANDLE) {
			return;
		}

		SocketAddress source_address;
		for (;;) {
			const int rc = Socket_Read_From(UDPSocket, ReceiveBuffer, sizeof(ReceiveBuffer), &source_address);
			if (rc > 0) {
				UDPIPAddress = source_address.Address;
				Copy_To_In_Buffer(ReceiveBuffer, rc);
				continue;
			}

			if (rc == 0) {
				break;
			}

			const int error_code = Socket_Get_Last_Error();
			if (Socket_Error_Is_Would_Block(error_code) || Socket_Error_Is_Connection_Reset(error_code)) {
				break;
			}

			Socket_Clear_Error(UDPSocket);
			break;
		}

		SocketAddress destination_address;
		destination_address.Address = UDPIPAddress;
		destination_address.Port = Socket_Host_To_Network16(PlanetWestwoodPortNumber);

		while (UDPSocket != INVALID_SOCKET_HANDLE && TransmitBuffers[TXBufferTail].InUse) {
			const int rc = Socket_Write_To(UDPSocket,
				TransmitBuffers[TXBufferTail].Buffer,
				TransmitBuffers[TXBufferTail].DataLength,
				&destination_address);
			if (rc < 0) {
				const int error_code = Socket_Get_Last_Error();
				if (!Socket_Error_Is_Would_Block(error_code)) {
					Socket_Clear_Error(UDPSocket);
				}
				break;
			}

			TransmitBuffers[TXBufferTail].InUse = false;
			TXBufferTail = (TXBufferTail + 1) & (WS_NUM_TX_BUFFERS - 1);
		}

		return;
	}

	if (!Connected) {
		if (IsServer) {
			if (ListenSocket != INVALID_SOCKET_HANDLE && Add_Client()) {
				ConnectStatus = CONNECTED_OK;
				Connected = TRUE;
				ConnectInProgress = FALSE;
			}
		} else if (ConnectInProgress && ConnectSocket != INVALID_SOCKET_HANDLE && Socket_Is_Write_Ready(ConnectSocket)) {
			Socket_Clear_Error(ConnectSocket);
			const int socket_error = Socket_Get_Last_Error();
			if (socket_error == 0) {
				ConnectStatus = CONNECTED_OK;
				Connected = TRUE;
				ConnectInProgress = FALSE;
			} else if (!Socket_Error_Is_Would_Block(socket_error)) {
				ConnectStatus = UNABLE_TO_CONNECT;
				Close_Socket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET_HANDLE;
				ConnectInProgress = FALSE;
			}
		}
	}

	if (!Connected || ConnectSocket == INVALID_SOCKET_HANDLE) {
		return;
	}

	uint8_t receive_buffer[WS_RECEIVE_BUFFER_LEN];
	for (;;) {
		const int rc = Socket_Read(ConnectSocket, receive_buffer, sizeof(receive_buffer));
		if (rc > 0) {
			StreamReceiveBuffer.insert(StreamReceiveBuffer.end(), receive_buffer, receive_buffer + rc);
			while (StreamReceiveBuffer.size() >= sizeof(uint32_t)) {
				const uint32_t frame_length = Decode_Frame_Length(StreamReceiveBuffer);
				if (frame_length == 0 || frame_length > WS_INTERNET_BUFFER_LEN) {
					ConnectStatus = CONNECTION_LOST;
					Connected = FALSE;
					Close_Socket(ConnectSocket);
					ConnectSocket = INVALID_SOCKET_HANDLE;
					StreamReceiveBuffer.clear();
					return;
				}
				if (StreamReceiveBuffer.size() < sizeof(uint32_t) + frame_length) {
					break;
				}
				Copy_To_In_Buffer(&StreamReceiveBuffer[sizeof(uint32_t)], (int)frame_length);
				StreamReceiveBuffer.erase(StreamReceiveBuffer.begin(),
					StreamReceiveBuffer.begin() + sizeof(uint32_t) + frame_length);
			}
			continue;
		}

		if (rc == 0) {
			ConnectStatus = CONNECTION_LOST;
			Connected = FALSE;
			Close_Socket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET_HANDLE;
			return;
		}

		const int error_code = Socket_Get_Last_Error();
		if (Socket_Error_Is_Would_Block(error_code)) {
			break;
		}

		ConnectStatus = CONNECTION_LOST;
		Connected = FALSE;
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
		return;
	}

	while (ConnectSocket != INVALID_SOCKET_HANDLE) {
		if (!StreamSendActive) {
			if (!TransmitBuffers[TXBufferTail].InUse) {
				break;
			}

			const uint32_t frame_length = Socket_Host_To_Network32((uint32_t)TransmitBuffers[TXBufferTail].DataLength);
			memcpy(StreamSendBuffer, &frame_length, sizeof(frame_length));
			memcpy(StreamSendBuffer + sizeof(frame_length),
				TransmitBuffers[TXBufferTail].Buffer,
				TransmitBuffers[TXBufferTail].DataLength);
			StreamSendLength = TransmitBuffers[TXBufferTail].DataLength + sizeof(frame_length);
			StreamSendOffset = 0;
			StreamSendActive = TRUE;
		}

		const int rc = Socket_Write(ConnectSocket,
			reinterpret_cast<const char*>(StreamSendBuffer) + StreamSendOffset,
			StreamSendLength - StreamSendOffset);
		if (rc > 0) {
			StreamSendOffset += rc;
			if (StreamSendOffset >= StreamSendLength) {
				TransmitBuffers[TXBufferTail].InUse = false;
				TXBufferTail = (TXBufferTail + 1) & (WS_NUM_TX_BUFFERS - 1);
				StreamSendLength = 0;
				StreamSendOffset = 0;
				StreamSendActive = FALSE;
			}
			continue;
		}

		if (rc == 0) {
			ConnectStatus = CONNECTION_LOST;
			Connected = FALSE;
			Close_Socket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET_HANDLE;
			return;
		}

		const int error_code = Socket_Get_Last_Error();
		if (Socket_Error_Is_Would_Block(error_code)) {
			break;
		}

		ConnectStatus = CONNECTION_LOST;
		Connected = FALSE;
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
		return;
	}
}

void TcpipManagerClass::Start_Server(void)
{
	int i;
	SocketAddress addr;

	if (!Init()) {
		ConnectStatus = NOT_CONNECTING;
		return;
	}

	TXBufferHead = 0;
	TXBufferTail = 0;
	RXBufferHead = 0;
	RXBufferTail = 0;

	for (i = 0; i < WS_NUM_TX_BUFFERS; ++i) {
		TransmitBuffers[i].InUse = false;
	}

	for (i = 0; i < WS_NUM_RX_BUFFERS; ++i) {
		ReceiveBuffers[i].InUse = false;
	}

	IsServer = TRUE;
	Connected = FALSE;
	ConnectInProgress = FALSE;
	ConnectStatus = CONNECTING;
	StreamReceiveBuffer.clear();
	StreamSendLength = 0;
	StreamSendOffset = 0;
	StreamSendActive = FALSE;

	if (ConnectSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
	}
	if (ListenSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ListenSocket);
		ListenSocket = INVALID_SOCKET_HANDLE;
	}
	if (UDPSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(UDPSocket);
		UDPSocket = INVALID_SOCKET_HANDLE;
	}

	if (UseUDP) {
		Start_Client();
		return;
	}

	addr.Address = SOCKET_ADDRESS_ANY;
	addr.Port = Socket_Host_To_Network16(PlanetWestwoodPortNumber);

	ListenSocket = Socket_Open(SOCKET_TYPE_STREAM, SOCKET_PROTOCOL_TCP);
	if (ListenSocket == INVALID_SOCKET_HANDLE) {
		ConnectStatus = NOT_CONNECTING;
		return;
	}

	if (Socket_Set_NonBlocking(ListenSocket, true) != 0 ||
		Socket_Set_Integer_Option(ListenSocket, SOCKET_OPTION_REUSE_ADDRESS, TRUE) != 0 ||
		Socket_Bind(ListenSocket, &addr) != 0 ||
		Socket_Listen(ListenSocket, 1) != 0) {
		Close_Socket(ListenSocket);
		ListenSocket = INVALID_SOCKET_HANDLE;
		ConnectStatus = NOT_CONNECTING;
	}
}

int TcpipManagerClass::Read(void* buffer, int buffer_len)
{
	int bytes_copied = 0;

	Keyboard::Check();
	Service();

	if (ReceiveBuffers[RXBufferTail].InUse) {
		memcpy(buffer, ReceiveBuffers[RXBufferTail].Buffer,
			MIN(ReceiveBuffers[RXBufferTail].DataLength, buffer_len));
		ReceiveBuffers[RXBufferTail].InUse = false;

		bytes_copied = MIN(ReceiveBuffers[RXBufferTail++].DataLength, buffer_len);
		RXBufferTail &= WS_NUM_RX_BUFFERS - 1;
	}

	return bytes_copied;
}

void TcpipManagerClass::Write(void* buffer, int buffer_len)
{
	if (!TransmitBuffers[TXBufferHead].InUse) {
		memcpy(TransmitBuffers[TXBufferHead].Buffer, buffer, MIN(buffer_len, WS_INTERNET_BUFFER_LEN));
		TransmitBuffers[TXBufferHead].InUse = true;
		TransmitBuffers[TXBufferHead++].DataLength = MIN(buffer_len, WS_INTERNET_BUFFER_LEN);
		TXBufferHead &= WS_NUM_TX_BUFFERS - 1;
	}

	Service();
	Keyboard::Check();
}

int32_t TcpipManagerClass::Add_Client(void)
{
	SocketAddress addr;
	int delay = TRUE;

	ConnectSocket = Socket_Accept(ListenSocket, &addr);
	if (ConnectSocket == INVALID_SOCKET_HANDLE) {
		if (Socket_Error_Is_Would_Block(Socket_Get_Last_Error())) {
			return FALSE;
		}
		return FALSE;
	}

	Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_TCP_NO_DELAY, delay);
	Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_RECEIVE_BUFFER, SocketReceiveBuffer);
	Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_SEND_BUFFER, SocketSendBuffer);
	if (Socket_Set_NonBlocking(ConnectSocket, true) != 0) {
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
		return FALSE;
	}

	ClientIPAddress = addr.Address;
	UDPIPAddress = addr.Address;

	if (!Socket_Lookup_Host_By_Address(addr.Address, &ClientName[0], sizeof(ClientName))) {
		ClientName[0] = '\0';
	}

	if (!UseUDP) {
		if (ListenSocket != INVALID_SOCKET_HANDLE) {
			Close_Socket(ListenSocket);
			ListenSocket = INVALID_SOCKET_HANDLE;
		}
		return TRUE;
	}

	UDPSocket = Socket_Open(SOCKET_TYPE_DATAGRAM, SOCKET_PROTOCOL_DEFAULT);
	if (UDPSocket == INVALID_SOCKET_HANDLE) {
		return FALSE;
	}

	addr.Address = SOCKET_ADDRESS_ANY;
	addr.Port = Socket_Host_To_Network16(PlanetWestwoodPortNumber);

	if (Socket_Bind(UDPSocket, &addr) != 0) {
		Close_Socket(UDPSocket);
		ConnectStatus = NOT_CONNECTING;
		return FALSE;
	}

	Socket_Set_Integer_Option(UDPSocket, SOCKET_OPTION_RECEIVE_BUFFER, SocketReceiveBuffer);
	Socket_Set_Integer_Option(UDPSocket, SOCKET_OPTION_SEND_BUFFER, SocketSendBuffer);
	Socket_Set_NonBlocking(UDPSocket, true);

	return TRUE;
}

void TcpipManagerClass::Message_Handler(HWND, UINT, UINT, LONG)
{
	Service();
}

void TcpipManagerClass::Copy_To_In_Buffer(void const* data, int bytes)
{
	if (!ReceiveBuffers[RXBufferHead].InUse) {
		memcpy(ReceiveBuffers[RXBufferHead].Buffer, data, MIN(bytes, WS_INTERNET_BUFFER_LEN));
		ReceiveBuffers[RXBufferHead].InUse = true;
		ReceiveBuffers[RXBufferHead++].DataLength = MIN(bytes, WS_INTERNET_BUFFER_LEN);
		RXBufferHead &= WS_NUM_RX_BUFFERS - 1;
	}
}

void TcpipManagerClass::Set_Host_Address(char const* address)
{
	char const* resolved = address != NULL ? address : "";
	SDL_strlcpy(HostAddress, resolved, sizeof(HostAddress));
	SDL_strlcpy(PlanetWestwoodIPAddress, resolved, sizeof(PlanetWestwoodIPAddress));
}

void TcpipManagerClass::Start_Client(void)
{
	SocketAddress addr;
	int i;
	SocketHostEntry host_entry;

	if (!Init()) {
		ConnectStatus = NOT_CONNECTING;
		return;
	}

	addr.Address = SOCKET_ADDRESS_ANY;
	addr.Port = 0;

	TXBufferHead = 0;
	TXBufferTail = 0;
	RXBufferHead = 0;
	RXBufferTail = 0;

	for (i = 0; i < WS_NUM_TX_BUFFERS; ++i) {
		TransmitBuffers[i].InUse = false;
	}

	for (i = 0; i < WS_NUM_RX_BUFFERS; ++i) {
		ReceiveBuffers[i].InUse = false;
	}

	Connected = FALSE;
	ConnectInProgress = FALSE;
	IsServer = FALSE;
	StreamReceiveBuffer.clear();
	StreamSendLength = 0;
	StreamSendOffset = 0;
	StreamSendActive = FALSE;

	if (ConnectSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ConnectSocket);
		ConnectSocket = INVALID_SOCKET_HANDLE;
	}
	if (ListenSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(ListenSocket);
		ListenSocket = INVALID_SOCKET_HANDLE;
	}
	if (UDPSocket != INVALID_SOCKET_HANDLE) {
		Close_Socket(UDPSocket);
		UDPSocket = INVALID_SOCKET_HANDLE;
	}

	if (!UseUDP) {
		if (PlanetWestwoodIPAddress[0] == '\0') {
			Server.Name[0] = '\0';
			strcpy(Server.DotAddr, "????");
			ConnectStatus = SERVER_ADDRESS_LOOKUP_FAILED;
			return;
		}

		if (Socket_Address_From_String(PlanetWestwoodIPAddress, &Server.Address)) {
			Socket_Address_To_String(Server.Address, Server.DotAddr, sizeof(Server.DotAddr));
		} else if (!Socket_Lookup_Host_By_Name(PlanetWestwoodIPAddress, &host_entry) || host_entry.AddressCount == 0) {
			Server.Name[0] = '\0';
			strcpy(Server.DotAddr, "????");
			ConnectStatus = SERVER_ADDRESS_LOOKUP_FAILED;
			return;
		} else {
			Server.Address = host_entry.Addresses[0];
			Copy_Socket_String(Server.Name, sizeof(Server.Name), host_entry.Name);
			Socket_Address_To_String(Server.Address, Server.DotAddr, sizeof(Server.DotAddr));
		}
		UDPIPAddress = Server.Address;

		addr.Address = Server.Address;
		addr.Port = Socket_Host_To_Network16(PlanetWestwoodPortNumber);

		ConnectSocket = Socket_Open(SOCKET_TYPE_STREAM, SOCKET_PROTOCOL_TCP);
		if (ConnectSocket == INVALID_SOCKET_HANDLE) {
			ConnectStatus = NOT_CONNECTING;
			return;
		}

		Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_TCP_NO_DELAY, TRUE);
		Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_RECEIVE_BUFFER, SocketReceiveBuffer);
		Socket_Set_Integer_Option(ConnectSocket, SOCKET_OPTION_SEND_BUFFER, SocketSendBuffer);
		if (Socket_Set_NonBlocking(ConnectSocket, true) != 0) {
			Close_Socket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET_HANDLE;
			ConnectStatus = NOT_CONNECTING;
			return;
		}

		const int rc = Socket_Connect(ConnectSocket, &addr);
		if (rc == 0) {
			ConnectStatus = CONNECTED_OK;
			Connected = TRUE;
			ConnectInProgress = FALSE;
		} else {
			const int error_code = Socket_Get_Last_Error();
			if (Socket_Error_Is_Would_Block(error_code)) {
				ConnectStatus = CONTACTING_SERVER;
				ConnectInProgress = TRUE;
			} else {
				ConnectStatus = UNABLE_TO_CONNECT;
				Close_Socket(ConnectSocket);
				ConnectSocket = INVALID_SOCKET_HANDLE;
			}
		}
		return;
	}

	UDPSocket = Socket_Open(SOCKET_TYPE_DATAGRAM, SOCKET_PROTOCOL_DEFAULT);
	if (UDPSocket == INVALID_SOCKET_HANDLE) {
		Close_Socket(ConnectSocket);
		ConnectStatus = NOT_CONNECTING;
		return;
	}

	addr.Address = SOCKET_ADDRESS_ANY;
	addr.Port = Socket_Host_To_Network16(PlanetWestwoodPortNumber);

	if (Socket_Bind(UDPSocket, &addr) != 0) {
		Close_Socket(UDPSocket);
		Close_Socket(ConnectSocket);
		ConnectStatus = NOT_CONNECTING;
		return;
	}

	Socket_Set_Integer_Option(UDPSocket, SOCKET_OPTION_RECEIVE_BUFFER, SocketReceiveBuffer);
	Socket_Set_Integer_Option(UDPSocket, SOCKET_OPTION_SEND_BUFFER, SocketSendBuffer);
	Socket_Set_NonBlocking(UDPSocket, true);

	if (Socket_Address_From_String(PlanetWestwoodIPAddress, &Server.Address)) {
		Socket_Address_To_String(Server.Address, Server.DotAddr, sizeof(Server.DotAddr));
		UDPIPAddress = Server.Address;
		ConnectStatus = CONNECTED_OK;
		Connected = TRUE;
	} else if (Socket_Lookup_Host_By_Name(PlanetWestwoodIPAddress, &host_entry) && host_entry.AddressCount > 0) {
		Server.Address = host_entry.Addresses[0];
		UDPIPAddress = Server.Address;
		Copy_Socket_String(Server.Name, sizeof(Server.Name), PlanetWestwoodIPAddress);
		Socket_Address_To_String(Server.Address, Server.DotAddr, sizeof(Server.DotAddr));
		ConnectStatus = CONNECTED_OK;
		Connected = TRUE;
	} else {
		Server.Name[0] = '\0';
		strcpy(Server.DotAddr, "????");
		ConnectStatus = SERVER_ADDRESS_LOOKUP_FAILED;
	}
}

void TcpipManagerClass::Close_Socket(SocketHandle s)
{
	Socket_Set_Linger(s, false, 0);
	Socket_Close(s);
}

void TcpipManagerClass::Set_Protocol_UDP(int32_t state)
{
	UseUDP = state;
}

void TcpipManagerClass::Clear_Socket_Error(SocketHandle socket)
{
	Socket_Clear_Error(socket);
}
