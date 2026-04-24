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

/* $Header: /CounterStrike/TCPIP.H 1     3/03/97 10:25a Joe_bostic $ */

#ifndef TCPIP_H
#define TCPIP_H

#include "SOCKETS.H"

#include <cstdint>
#include <vector>

extern bool Server;

#define PORTNUM						0x1000
#define UDP_PORT					0x1001
#define WS_INTERNET_BUFFER_LEN  1024
#define WS_NUM_TX_BUFFERS		16
#define WS_NUM_RX_BUFFERS		16
#define WS_RECEIVE_BUFFER_LEN	1024

#define PLANET_WESTWOOD_HANDLE_MAX 20
#define PLANET_WESTWOOD_PASSWORD_MAX 20
#define IP_ADDRESS_MAX 40
#define PORT_NUMBER_MAX 6

class TcpipManagerClass {

	public:

		TcpipManagerClass(void);
		~TcpipManagerClass(void);

		int32_t Init(void);
		void Service(void);
		void Start_Server(void);
		void Start_Client(void);
		void Close_Socket(SocketHandle s);
	 	void Message_Handler(HWND window, UINT message, UINT wParam, LONG lParam);
		void Copy_To_In_Buffer(void const* data, int bytes);
		int Read(void* buffer, int buffer_len);
		void Write(void* buffer, int buffer_len);
		int32_t Add_Client(void);
		void Close(void);
		void Set_Host_Address(char const* address);
		void Set_Protocol_UDP(int32_t state);
		void Clear_Socket_Error(SocketHandle socket);

		inline int32_t Get_Connected(void) { return Connected; }

		typedef enum ConnectStatusEnum {
			CONNECTED_OK = 0,
			NOT_CONNECTING,
			CONNECTING,
			UNABLE_TO_CONNECT_TO_SERVER,
			CONTACTING_SERVER,
			SERVER_ADDRESS_LOOKUP_FAILED,
			RESOLVING_HOST_ADDRESS,
			UNABLE_TO_ACCEPT_CLIENT,
			UNABLE_TO_CONNECT,
			CONNECTION_LOST
		} ConnectStatusEnum;

		inline ConnectStatusEnum Get_Connection_Status(void) { return ConnectStatus; }

	private:

		typedef struct {
			uint32_t Address;
			char DotAddr[SOCKET_MAX_DOT_ADDRESS];
			char Name[255];
		} HostType;

		typedef struct {
			char Buffer[WS_INTERNET_BUFFER_LEN];
			int DataLength;
			bool InUse : 1;
		} InternetBufferType;

		int32_t WinsockInitialised;
		SocketHandle ListenSocket;
		SocketHandle ConnectSocket;
		SocketHandle UDPSocket;
		uint32_t ClientIPAddress;
		char ClientName[128];
		char ReceiveBuffer[WS_RECEIVE_BUFFER_LEN];
		int32_t IsServer;
		int32_t Connected;
		HostType Server;
		char HostAddress[IP_ADDRESS_MAX];
		ConnectStatusEnum ConnectStatus;
		int32_t UseUDP;
		uint32_t UDPIPAddress;
		int SocketReceiveBuffer;
		int SocketSendBuffer;
		InternetBufferType ReceiveBuffers[WS_NUM_RX_BUFFERS];
		InternetBufferType TransmitBuffers[WS_NUM_TX_BUFFERS];
		int TXBufferHead;
		int TXBufferTail;
		int RXBufferHead;
		int RXBufferTail;
		int32_t ConnectInProgress;
		std::vector<uint8_t> StreamReceiveBuffer;
		uint8_t StreamSendBuffer[WS_INTERNET_BUFFER_LEN + sizeof(uint32_t)];
		int StreamSendLength;
		int StreamSendOffset;
		int32_t StreamSendActive;
};

extern TcpipManagerClass Winsock;

extern char PlanetWestwoodIPAddress[IP_ADDRESS_MAX];
extern uint16_t PlanetWestwoodPortNumber;
extern bool PlanetWestwoodIsHost;
extern int Read_Game_Options(char*);
extern bool UseVirtualSubnetServer;
extern int InternetMaxPlayers;

#define TXT_WINSOCK_CONNECTING							4567+13
#define TXT_WINSOCK_NOT_CONNECTING					4567+14
#define TXT_WINSOCK_UNABLE_TO_CONNECT_TO_SERVER	4567+15
#define TXT_WINSOCK_CONTACTING_SERVER				4567+16
#define TXT_WINSOCK_SERVER_ADDRESS_LOOKUP_FAILED	4567+17
#define TXT_WINSOCK_UNABLE_TO_ACCEPT_CLIENT		4567+18
#define TXT_WINSOCK_UNABLE_TO_CONNECT				4567+19
#define TXT_WINSOCK_CONNECTION_LOST					4567+20
#define TXT_WINSOCK_RESOLVING_HOST_ADDRESS			4567+21

#endif
