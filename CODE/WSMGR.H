/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	Modern Westwood Online connection manager for Tiberian Dawn.
**	It keeps Red Alert's modern WOL split between:
**	- WebSocket transport
**	- WOL control frames
**	- global/private gameplay queues keyed by connection id
*/
#ifndef WSMGR_H
#define WSMGR_H

#include "CONNMGR.H"
#include "UDPADDR.H"
#include "WSCLIENT.H"

#include <cstdint>

class WSManagerClass : public ConnManClass
{
	public:
		enum ManagerEnum {
			MAX_CONNECTIONS = 6,
			CONN_NAME_MAX = 40,
			MAX_QUEUED_MESSAGES = 128,
			MAX_WOL_MESSAGE_LENGTH = 64 * 1024
		};

		enum RelayChannelType {
			RELAY_PRIVATE_CHANNEL = 0,
			RELAY_GLOBAL_CHANNEL = 1
		};

		WSManagerClass();
		virtual ~WSManagerClass();

		void Configure(char const *server_url, char const *nickname);
		bool Connect(char const *server_url);
		int Init(void);
		void Shutdown(void);
		void Return_To_Lobby(void);

		bool Is_Connected(void) const
		{
			return Connected;
		}

		bool Is_Logged_In(void) const
		{
			return LoggedIn;
		}

		std::uint32_t My_Client_Id(void) const
		{
			return LocalClientId;
		}

		bool Send_Hello(char const *nickname);
		bool Send_Channel_Join(char const *channel_name);
		bool Send_Game_Create(char const *game_name, std::uint8_t max_players);
		bool Send_Game_Join(std::uint32_t game_id);
		bool Send_Game_Leave(void);
		bool Send_Game_Start(void);

		std::uint16_t Next_Control_Frame(void *buffer, int *length);

		static void Client_Id_To_Address(std::uint32_t client_id, UDPAddressClass *address);
		static std::uint32_t Address_To_Client_Id(UDPAddressClass *address);

		bool Create_Connection(int id, char *name, UDPAddressClass *address);
		bool Create_Connection_By_Cid(int id, char const *name, std::uint32_t client_id);
		bool Delete_Connection(int id);
		char *Connection_Name(int id);
		UDPAddressClass *Connection_Address(int id);
		std::uint32_t Connection_Remote_Cid(int id) const;

		int Send_Global_Message(void *buffer, int length, int ack_req = 0, UDPAddressClass *address = 0);
		int Get_Global_Message(void *buffer, int *length, UDPAddressClass *address, unsigned short *product_id);

		virtual int Service(void);
		virtual int Send_Private_Message(void *buffer, int length, int ack_req = 1, int conn_id = CONNECTION_NONE);
		virtual int Get_Private_Message(void *buffer, int *length, int *conn_id);
		virtual int Num_Connections(void);
		virtual int Connection_ID(int index);
		virtual int Connection_Index(int id);
		virtual int Global_Num_Send(void);
		virtual int Global_Num_Receive(void);
		virtual int Private_Num_Send(int id = CONNECTION_NONE);
		virtual int Private_Num_Receive(int id = CONNECTION_NONE);
		virtual void Reset_Response_Time(void);
		virtual unsigned long Response_Time(void);
		virtual void Set_Timing(unsigned long retrydelta, unsigned long maxretries, unsigned long timeout);
		virtual void Configure_Debug(int index, int offset, int size, char **names, int maxnames);
		virtual void Mono_Debug_Print(int index, int refresh = 0);

	private:
		struct ConnectionType {
			int Used;
			int Id;
			std::uint32_t RemoteClientId;
			char Name[CONN_NAME_MAX];
			UDPAddressClass Address;
		};

		struct QueuedMessageType {
			std::uint8_t *Data;
			int Length;
			int ConnectionId;
			std::uint32_t SourceClientId;
			std::uint16_t Opcode;
			QueuedMessageType *Next;
		};

		bool Send_Frame(std::uint16_t opcode, void const *payload, std::size_t payload_length);
		int Find_Connection_Index_By_Id(int id) const;
		int Find_Connection_Index_By_Cid(std::uint32_t client_id) const;
		void Reset_Peers(void);
		void Reset_Runtime_State(void);
		void Clear_Queue(QueuedMessageType **head, QueuedMessageType **tail, int *count);
		bool Queue_Message(QueuedMessageType **head, QueuedMessageType **tail, int *count, int connection_id, std::uint32_t source_client_id, std::uint16_t opcode, void const *data, int length);
		QueuedMessageType *Pop_Message(QueuedMessageType **head, QueuedMessageType **tail, int *count);
		void Purge_Private_For_Connection(int connection_id);
		void Purge_Pending_Private_For_Cid(std::uint32_t client_id);
		void Promote_Pending_Private(std::uint32_t client_id, int connection_id);

		WSClientClass Client;
		bool Connected;
		bool LoggedIn;
		std::uint32_t LocalClientId;
		char PendingUrl[256];
		char PendingNick[64];
		char ActiveUrl[256];
		char ActiveNick[64];
		ConnectionType Connections[MAX_CONNECTIONS];
		QueuedMessageType *PrivateHead;
		QueuedMessageType *PrivateTail;
		QueuedMessageType *PendingPrivateHead;
		QueuedMessageType *PendingPrivateTail;
		QueuedMessageType *GlobalHead;
		QueuedMessageType *GlobalTail;
		QueuedMessageType *ControlHead;
		QueuedMessageType *ControlTail;
		int PrivateCount;
		int PendingPrivateCount;
		int GlobalCount;
		int ControlCount;
};

extern WSManagerClass Wol;

#endif
