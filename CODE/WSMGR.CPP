/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	Modern Westwood Online connection manager for Tiberian Dawn.
*/
#include "WSMGR.H"

#include "WOL_PROTO.H"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

namespace {

static constexpr unsigned short TD_WOL_PRODUCT_ID = 0xaa01;
static constexpr char TD_WOL_LOBBY_NAME[] = "td-lobby";

}

using namespace WolProto;

WSManagerClass Wol;

WSManagerClass::WSManagerClass()
{
	Connected = false;
	LoggedIn = false;
	LocalClientId = 0;
	PendingUrl[0] = '\0';
	PendingNick[0] = '\0';
	ActiveUrl[0] = '\0';
	ActiveNick[0] = '\0';
	PrivateHead = nullptr;
	PrivateTail = nullptr;
	PendingPrivateHead = nullptr;
	PendingPrivateTail = nullptr;
	GlobalHead = nullptr;
	GlobalTail = nullptr;
	ControlHead = nullptr;
	ControlTail = nullptr;
	PrivateCount = 0;
	PendingPrivateCount = 0;
	GlobalCount = 0;
	ControlCount = 0;
	Reset_Peers();
}

WSManagerClass::~WSManagerClass()
{
	Shutdown();
}

void WSManagerClass::Configure(char const *server_url, char const *nickname)
{
	if (server_url != nullptr && server_url[0] != '\0') {
		char const *scheme = std::strstr(server_url, "://");
		char const *host = (scheme != nullptr) ? scheme + 3 : server_url;
		char const *path = std::strchr(host, '/');
		if (path == nullptr) {
			std::snprintf(PendingUrl, sizeof(PendingUrl), "%s/ws", server_url);
		} else {
			std::strncpy(PendingUrl, server_url, sizeof(PendingUrl) - 1);
			PendingUrl[sizeof(PendingUrl) - 1] = '\0';
		}
	} else {
		PendingUrl[0] = '\0';
	}

	if (nickname != nullptr) {
		std::strncpy(PendingNick, nickname, sizeof(PendingNick) - 1);
		PendingNick[sizeof(PendingNick) - 1] = '\0';
	} else {
		PendingNick[0] = '\0';
	}
}

bool WSManagerClass::Connect(char const *server_url)
{
	Connected = Client.Connect(server_url);
	if (!Connected) {
		LoggedIn = false;
		LocalClientId = 0;
	}

	return Connected;
}

int WSManagerClass::Init(void)
{
	if (Connected && LoggedIn
	 && std::strcmp(ActiveUrl, PendingUrl) == 0
	 && std::strcmp(ActiveNick, PendingNick) == 0) {
		if (Num_Connections() > 0 || PrivateCount > 0 || GlobalCount > 0 || ControlCount > 0 || PendingPrivateCount > 0) {
			Return_To_Lobby();
		}
		return true;
	}

	if (PendingUrl[0] == '\0') {
		return false;
	}

	if (Connected || LoggedIn || Num_Connections() > 0 || PrivateCount > 0 || GlobalCount > 0 || ControlCount > 0 || PendingPrivateCount > 0) {
		Shutdown();
	}

	if (!Connect(PendingUrl)) {
		return false;
	}

	bool hello_sent = false;
	for (int spin = 0; spin < 600 && !LoggedIn; ++spin) {
		Client.Poll();
		if (!hello_sent && Client.Is_Open()) {
			if (!Send_Hello(PendingNick)) {
				Shutdown();
				return false;
			}
			hello_sent = true;
		}

		Service();

		if (Client.Get_State() == WSClientClass::STATE_FAILED || Client.Get_State() == WSClientClass::STATE_DISCONNECTED) {
			Shutdown();
			return false;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	if (!LoggedIn) {
		Shutdown();
		return false;
	}

	std::strncpy(ActiveUrl, PendingUrl, sizeof(ActiveUrl) - 1);
	ActiveUrl[sizeof(ActiveUrl) - 1] = '\0';
	std::strncpy(ActiveNick, PendingNick, sizeof(ActiveNick) - 1);
	ActiveNick[sizeof(ActiveNick) - 1] = '\0';

	Send_Channel_Join(TD_WOL_LOBBY_NAME);
	for (int spin = 0; spin < 25; ++spin) {
		Client.Poll();
		Service();
		if (Client.Get_State() == WSClientClass::STATE_FAILED || Client.Get_State() == WSClientClass::STATE_DISCONNECTED) {
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

	return LoggedIn ? true : false;
}

void WSManagerClass::Shutdown(void)
{
	Client.Close();
	Reset_Runtime_State();
}

void WSManagerClass::Return_To_Lobby(void)
{
	if (LoggedIn) {
		Send_Game_Leave();
		Send_Channel_Join(TD_WOL_LOBBY_NAME);
	}

	Reset_Peers();
	Clear_Queue(&PrivateHead, &PrivateTail, &PrivateCount);
	Clear_Queue(&PendingPrivateHead, &PendingPrivateTail, &PendingPrivateCount);
	Clear_Queue(&GlobalHead, &GlobalTail, &GlobalCount);
	Clear_Queue(&ControlHead, &ControlTail, &ControlCount);
}

bool WSManagerClass::Send_Hello(char const *nickname)
{
	const std::uint16_t nickname_length = static_cast<std::uint16_t>(std::strlen(nickname != nullptr ? nickname : ""));
	const std::size_t payload_length = 4 + 2 + nickname_length;
	std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (payload == nullptr) {
		return false;
	}

	std::uint8_t *cursor = payload;
	Write_U32(cursor, PROTOCOL_VERSION);
	Write_U16(cursor, nickname_length);
	if (nickname_length > 0) {
		std::memcpy(cursor, nickname, nickname_length);
	}

	const bool sent = Send_Frame(OP_HELLO, payload, payload_length);
	std::free(payload);
	return sent;
}

bool WSManagerClass::Send_Channel_Join(char const *channel_name)
{
	const std::uint16_t channel_length = static_cast<std::uint16_t>(std::strlen(channel_name != nullptr ? channel_name : ""));
	const std::size_t payload_length = 2 + channel_length;
	std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (payload == nullptr) {
		return false;
	}

	std::uint8_t *cursor = payload;
	Write_U16(cursor, channel_length);
	if (channel_length > 0) {
		std::memcpy(cursor, channel_name, channel_length);
	}

	const bool sent = Send_Frame(OP_CHANNEL_JOIN, payload, payload_length);
	std::free(payload);
	return sent;
}

bool WSManagerClass::Send_Game_Create(char const *game_name, std::uint8_t max_players)
{
	const std::uint16_t name_length = static_cast<std::uint16_t>(std::strlen(game_name != nullptr ? game_name : ""));
	const std::size_t payload_length = 2 + name_length + 1 + 4;
	std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (payload == nullptr) {
		return false;
	}

	std::uint8_t *cursor = payload;
	Write_U16(cursor, name_length);
	if (name_length > 0) {
		std::memcpy(cursor, game_name, name_length);
		cursor += name_length;
	}
	Write_U8(cursor, max_players);
	Write_U32(cursor, 0);

	const bool sent = Send_Frame(OP_GAME_CREATE, payload, payload_length);
	std::free(payload);
	return sent;
}

bool WSManagerClass::Send_Game_Join(std::uint32_t game_id)
{
	std::uint8_t payload[4];
	std::uint8_t *cursor = payload;
	Write_U32(cursor, game_id);
	return Send_Frame(OP_GAME_JOIN, payload, sizeof(payload));
}

bool WSManagerClass::Send_Game_Leave(void)
{
	return Send_Frame(OP_GAME_LEAVE, nullptr, 0);
}

bool WSManagerClass::Send_Game_Start(void)
{
	return Send_Frame(OP_GAME_START, nullptr, 0);
}

std::uint16_t WSManagerClass::Next_Control_Frame(void *buffer, int *length)
{
	QueuedMessageType *message = Pop_Message(&ControlHead, &ControlTail, &ControlCount);
	if (message == nullptr) {
		return 0;
	}

	int copy_length = message->Length;
	if (buffer == nullptr || length == nullptr || *length < copy_length) {
		if (length != nullptr) {
			*length = copy_length;
		}
		const std::uint16_t opcode = message->Opcode;
		std::free(message->Data);
		std::free(message);
		return opcode;
	}

	if (copy_length > 0) {
		std::memcpy(buffer, message->Data, copy_length);
	}
	*length = copy_length;
	const std::uint16_t opcode = message->Opcode;
	std::free(message->Data);
	std::free(message);
	return opcode;
}

void WSManagerClass::Client_Id_To_Address(std::uint32_t client_id, UDPAddressClass *address)
{
	NetNumType network = {0, 0, 0, 0};
	NetNodeType node = {0, 0, 0, 0, 0, 0};
	node[0] = static_cast<unsigned char>(client_id & 0xffU);
	node[1] = static_cast<unsigned char>((client_id >> 8) & 0xffU);
	node[2] = static_cast<unsigned char>((client_id >> 16) & 0xffU);
	node[3] = static_cast<unsigned char>((client_id >> 24) & 0xffU);
	address->Set_Address(network, node);
}

std::uint32_t WSManagerClass::Address_To_Client_Id(UDPAddressClass *address)
{
	NetNumType network;
	NetNodeType node;
	address->Get_Address(network, node);
	return static_cast<std::uint32_t>(node[0])
		| (static_cast<std::uint32_t>(node[1]) << 8)
		| (static_cast<std::uint32_t>(node[2]) << 16)
		| (static_cast<std::uint32_t>(node[3]) << 24);
}

bool WSManagerClass::Create_Connection(int id, char *name, UDPAddressClass *address)
{
	const std::uint32_t client_id = (address != nullptr) ? Address_To_Client_Id(address) : 0;
	return Create_Connection_By_Cid(id, name, client_id);
}

bool WSManagerClass::Create_Connection_By_Cid(int id, char const *name, std::uint32_t client_id)
{
	for (int index = 0; index < MAX_CONNECTIONS; ++index) {
		if (Connections[index].Used) {
			if (Connections[index].Id == id || Connections[index].RemoteClientId == client_id) {
				Connections[index].Id = id;
				Connections[index].RemoteClientId = client_id;
				std::strncpy(Connections[index].Name, name != nullptr ? name : "", sizeof(Connections[index].Name) - 1);
				Connections[index].Name[sizeof(Connections[index].Name) - 1] = '\0';
				Client_Id_To_Address(client_id, &Connections[index].Address);
				Promote_Pending_Private(client_id, id);
				return true;
			}
			continue;
		}

		Connections[index].Used = true;
		Connections[index].Id = id;
		Connections[index].RemoteClientId = client_id;
		std::strncpy(Connections[index].Name, name != nullptr ? name : "", sizeof(Connections[index].Name) - 1);
		Connections[index].Name[sizeof(Connections[index].Name) - 1] = '\0';
		Client_Id_To_Address(client_id, &Connections[index].Address);
		Promote_Pending_Private(client_id, id);
		return true;
	}

	return false;
}

bool WSManagerClass::Delete_Connection(int id)
{
	const int connection_index = Find_Connection_Index_By_Id(id);
	if (connection_index < 0) {
		return false;
	}

	Purge_Private_For_Connection(id);
	Purge_Pending_Private_For_Cid(Connections[connection_index].RemoteClientId);
	Connections[connection_index].Used = false;
	Connections[connection_index].Id = CONNECTION_NONE;
	Connections[connection_index].RemoteClientId = 0;
	Connections[connection_index].Name[0] = '\0';
	return true;
}

char *WSManagerClass::Connection_Name(int id)
{
	const int connection_index = Find_Connection_Index_By_Id(id);
	if (connection_index < 0) {
		return const_cast<char *>("");
	}

	return Connections[connection_index].Name;
}

UDPAddressClass *WSManagerClass::Connection_Address(int id)
{
	const int connection_index = Find_Connection_Index_By_Id(id);
	if (connection_index < 0) {
		return nullptr;
	}

	return &Connections[connection_index].Address;
}

std::uint32_t WSManagerClass::Connection_Remote_Cid(int id) const
{
	const int connection_index = Find_Connection_Index_By_Id(id);
	if (connection_index < 0) {
		return 0;
	}

	return Connections[connection_index].RemoteClientId;
}

int WSManagerClass::Send_Global_Message(void *buffer, int length, int, UDPAddressClass *address)
{
	if (!LoggedIn || length < 0 || length > MAX_WOL_MESSAGE_LENGTH) {
		return false;
	}

	if (address == nullptr) {
		const std::size_t payload_length = 1 + static_cast<std::size_t>(length);
		std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
		if (payload == nullptr) {
			return false;
		}

		payload[0] = RELAY_GLOBAL_CHANNEL;
		if (length > 0) {
			std::memcpy(payload + 1, buffer, static_cast<std::size_t>(length));
		}

		const bool sent = Send_Frame(OP_RELAY_BROADCAST, payload, payload_length);
		std::free(payload);
		return sent ? true : false;
	}

	const std::uint32_t client_id = Address_To_Client_Id(address);
	const std::size_t payload_length = 4 + 1 + static_cast<std::size_t>(length);
	std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (payload == nullptr) {
		return false;
	}

	std::uint8_t *cursor = payload;
	Write_U32(cursor, client_id);
	Write_U8(cursor, RELAY_GLOBAL_CHANNEL);
	if (length > 0) {
		std::memcpy(cursor, buffer, static_cast<std::size_t>(length));
	}

	const bool sent = Send_Frame(OP_RELAY_PRIVATE, payload, payload_length);
	std::free(payload);
	return sent ? true : false;
}

int WSManagerClass::Get_Global_Message(void *buffer, int *length, UDPAddressClass *address, unsigned short *product_id)
{
	QueuedMessageType *message = Pop_Message(&GlobalHead, &GlobalTail, &GlobalCount);
	if (message == nullptr) {
		return false;
	}

	if (length == nullptr || buffer == nullptr || *length < message->Length) {
		if (length != nullptr) {
			*length = message->Length;
		}
		std::free(message->Data);
		std::free(message);
		return false;
	}

	if (message->Length > 0) {
		std::memcpy(buffer, message->Data, static_cast<std::size_t>(message->Length));
	}
	*length = message->Length;

	if (address != nullptr) {
		Client_Id_To_Address(message->SourceClientId, address);
	}
	if (product_id != nullptr) {
		*product_id = TD_WOL_PRODUCT_ID;
	}

	std::free(message->Data);
	std::free(message);
	return true;
}

int WSManagerClass::Service(void)
{
	if (Connected) {
		Client.Poll();
	}

	void *raw_message = nullptr;
	std::size_t raw_length = 0;
	while (Client.Receive_Binary(&raw_message, &raw_length)) {
		FrameView frame;
		const bool parsed = Parse_Frame(static_cast<std::uint8_t const *>(raw_message), raw_length, &frame);
		if (!parsed) {
			std::free(raw_message);
			continue;
		}

		switch (frame.Opcode) {
			case OP_WELCOME: {
				if (frame.PayloadLength >= 8) {
					std::uint8_t const *cursor = frame.Payload;
					LocalClientId = Read_U32(cursor);
					(void) Read_U32(cursor);
					LoggedIn = true;
					Queue_Message(&ControlHead, &ControlTail, &ControlCount, CONNECTION_NONE, 0, frame.Opcode, frame.Payload, static_cast<int>(frame.PayloadLength));
				}
				break;
			}

			case OP_RELAY_FROM: {
				if (frame.PayloadLength >= 5) {
					std::uint8_t const *cursor = frame.Payload;
					const std::uint32_t source_client_id = Read_U32(cursor);
					const std::uint8_t channel = Read_U8(cursor);
					const int message_length = static_cast<int>(frame.PayloadLength - 5);

					if (channel == RELAY_GLOBAL_CHANNEL) {
						Queue_Message(&GlobalHead, &GlobalTail, &GlobalCount, CONNECTION_NONE, source_client_id, frame.Opcode, cursor, message_length);
					} else {
						const int connection_index = Find_Connection_Index_By_Cid(source_client_id);
						if (connection_index >= 0) {
							Queue_Message(&PrivateHead, &PrivateTail, &PrivateCount, Connections[connection_index].Id, source_client_id, frame.Opcode, cursor, message_length);
						} else {
							Queue_Message(&PendingPrivateHead, &PendingPrivateTail, &PendingPrivateCount, CONNECTION_NONE, source_client_id, frame.Opcode, cursor, message_length);
						}
					}
				}
				break;
			}

			case OP_PEER_LEFT: {
				if (frame.PayloadLength >= 4) {
					std::uint8_t const *cursor = frame.Payload;
					const std::uint32_t source_client_id = Read_U32(cursor);
					Purge_Pending_Private_For_Cid(source_client_id);
					const int connection_index = Find_Connection_Index_By_Cid(source_client_id);
					if (connection_index >= 0) {
						Delete_Connection(Connections[connection_index].Id);
					}
				}
				Queue_Message(&ControlHead, &ControlTail, &ControlCount, CONNECTION_NONE, 0, frame.Opcode, frame.Payload, static_cast<int>(frame.PayloadLength));
				break;
			}

			default:
				Queue_Message(&ControlHead, &ControlTail, &ControlCount, CONNECTION_NONE, 0, frame.Opcode, frame.Payload, static_cast<int>(frame.PayloadLength));
				break;
		}

		std::free(raw_message);
	}

	if (Connected && (Client.Get_State() == WSClientClass::STATE_FAILED || Client.Get_State() == WSClientClass::STATE_DISCONNECTED)) {
		Connected = false;
		LoggedIn = false;
	}

	return true;
}

int WSManagerClass::Send_Private_Message(void *buffer, int length, int, int conn_id)
{
	if (!LoggedIn || length < 0 || length > MAX_WOL_MESSAGE_LENGTH) {
		return false;
	}

	if (conn_id == CONNECTION_NONE) {
		const std::size_t payload_length = 1 + static_cast<std::size_t>(length);
		std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
		if (payload == nullptr) {
			return false;
		}

		payload[0] = RELAY_PRIVATE_CHANNEL;
		if (length > 0) {
			std::memcpy(payload + 1, buffer, static_cast<std::size_t>(length));
		}

		const bool sent = Send_Frame(OP_RELAY_BROADCAST, payload, payload_length);
		std::free(payload);
		return sent ? true : false;
	}

	const int connection_index = Find_Connection_Index_By_Id(conn_id);
	if (connection_index < 0) {
		return false;
	}

	const std::size_t payload_length = 4 + 1 + static_cast<std::size_t>(length);
	std::uint8_t *payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (payload == nullptr) {
		return false;
	}

	std::uint8_t *cursor = payload;
	Write_U32(cursor, Connections[connection_index].RemoteClientId);
	Write_U8(cursor, RELAY_PRIVATE_CHANNEL);
	if (length > 0) {
		std::memcpy(cursor, buffer, static_cast<std::size_t>(length));
	}

	const bool sent = Send_Frame(OP_RELAY_PRIVATE, payload, payload_length);
	std::free(payload);
	return sent ? true : false;
}

int WSManagerClass::Get_Private_Message(void *buffer, int *length, int *conn_id)
{
	QueuedMessageType *message = Pop_Message(&PrivateHead, &PrivateTail, &PrivateCount);
	if (message == nullptr) {
		return false;
	}

	if (buffer == nullptr || length == nullptr || conn_id == nullptr || *length < message->Length) {
		if (length != nullptr) {
			*length = message->Length;
		}
		std::free(message->Data);
		std::free(message);
		return false;
	}

	if (message->Length > 0) {
		std::memcpy(buffer, message->Data, static_cast<std::size_t>(message->Length));
	}
	*length = message->Length;
	*conn_id = message->ConnectionId;

	std::free(message->Data);
	std::free(message);
	return true;
}

int WSManagerClass::Num_Connections(void)
{
	int count = 0;
	for (int index = 0; index < MAX_CONNECTIONS; ++index) {
		if (Connections[index].Used) {
			++count;
		}
	}
	return count;
}

int WSManagerClass::Connection_ID(int index)
{
	int current = 0;
	for (int connection_index = 0; connection_index < MAX_CONNECTIONS; ++connection_index) {
		if (!Connections[connection_index].Used) {
			continue;
		}
		if (current == index) {
			return Connections[connection_index].Id;
		}
		++current;
	}
	return CONNECTION_NONE;
}

int WSManagerClass::Connection_Index(int id)
{
	int current = 0;
	for (int connection_index = 0; connection_index < MAX_CONNECTIONS; ++connection_index) {
		if (!Connections[connection_index].Used) {
			continue;
		}
		if (Connections[connection_index].Id == id) {
			return current;
		}
		++current;
	}
	return CONNECTION_NONE;
}

int WSManagerClass::Global_Num_Send(void)
{
	return Client.Has_Pending_Output() ? 1 : 0;
}

int WSManagerClass::Global_Num_Receive(void)
{
	return GlobalCount;
}

int WSManagerClass::Private_Num_Send(int)
{
	return Client.Has_Pending_Output() ? 1 : 0;
}

int WSManagerClass::Private_Num_Receive(int id)
{
	if (id == CONNECTION_NONE) {
		return PrivateCount;
	}

	int count = 0;
	for (QueuedMessageType *message = PrivateHead; message != nullptr; message = message->Next) {
		if (message->ConnectionId == id) {
			++count;
		}
	}
	return count;
}

void WSManagerClass::Reset_Response_Time(void)
{
}

uint32_t WSManagerClass::Response_Time(void)
{
	return 0;
}

void WSManagerClass::Set_Timing(uint32_t, uint32_t, uint32_t)
{
}

void WSManagerClass::Configure_Debug(int, int, int, char **, int)
{
}

void WSManagerClass::Mono_Debug_Print(int, int)
{
}

bool WSManagerClass::Send_Frame(std::uint16_t opcode, void const *payload, std::size_t payload_length)
{
	if (payload_length > MAX_FRAME_PAYLOAD) {
		return false;
	}

	const std::size_t total_length = FRAME_HEADER_SIZE + payload_length;
	std::uint8_t *frame = static_cast<std::uint8_t *>(std::malloc(total_length));
	if (frame == nullptr) {
		return false;
	}

	Encode_Frame_Header(frame, opcode, 0, static_cast<std::uint32_t>(payload_length));
	if (payload_length > 0) {
		std::memcpy(frame + FRAME_HEADER_SIZE, payload, payload_length);
	}

	const bool sent = Client.Send_Binary(frame, total_length);
	std::free(frame);
	return sent;
}

int WSManagerClass::Find_Connection_Index_By_Id(int id) const
{
	for (int index = 0; index < MAX_CONNECTIONS; ++index) {
		if (Connections[index].Used && Connections[index].Id == id) {
			return index;
		}
	}
	return -1;
}

int WSManagerClass::Find_Connection_Index_By_Cid(std::uint32_t client_id) const
{
	for (int index = 0; index < MAX_CONNECTIONS; ++index) {
		if (Connections[index].Used && Connections[index].RemoteClientId == client_id) {
			return index;
		}
	}
	return -1;
}

void WSManagerClass::Reset_Peers(void)
{
	for (int index = 0; index < MAX_CONNECTIONS; ++index) {
		Connections[index].Used = false;
		Connections[index].Id = CONNECTION_NONE;
		Connections[index].RemoteClientId = 0;
		Connections[index].Name[0] = '\0';
	}
}

void WSManagerClass::Reset_Runtime_State(void)
{
	Connected = false;
	LoggedIn = false;
	LocalClientId = 0;
	ActiveUrl[0] = '\0';
	ActiveNick[0] = '\0';
	Reset_Peers();
	Clear_Queue(&PrivateHead, &PrivateTail, &PrivateCount);
	Clear_Queue(&PendingPrivateHead, &PendingPrivateTail, &PendingPrivateCount);
	Clear_Queue(&GlobalHead, &GlobalTail, &GlobalCount);
	Clear_Queue(&ControlHead, &ControlTail, &ControlCount);
}

void WSManagerClass::Clear_Queue(QueuedMessageType **head, QueuedMessageType **tail, int *count)
{
	while (*head != nullptr) {
		QueuedMessageType *next = (*head)->Next;
		std::free((*head)->Data);
		std::free(*head);
		*head = next;
	}

	*tail = nullptr;
	*count = 0;
}

bool WSManagerClass::Queue_Message(QueuedMessageType **head, QueuedMessageType **tail, int *count, int connection_id, std::uint32_t source_client_id, std::uint16_t opcode, void const *data, int length)
{
	if (length < 0 || length > MAX_WOL_MESSAGE_LENGTH || *count >= MAX_QUEUED_MESSAGES) {
		return false;
	}

	QueuedMessageType *message = static_cast<QueuedMessageType *>(std::malloc(sizeof(QueuedMessageType)));
	if (message == nullptr) {
		return false;
	}

	message->Data = nullptr;
	if (length > 0) {
		message->Data = static_cast<std::uint8_t *>(std::malloc(static_cast<std::size_t>(length)));
		if (message->Data == nullptr) {
			std::free(message);
			return false;
		}
		std::memcpy(message->Data, data, static_cast<std::size_t>(length));
	}

	message->Length = length;
	message->ConnectionId = connection_id;
	message->SourceClientId = source_client_id;
	message->Opcode = opcode;
	message->Next = nullptr;

	if (*tail != nullptr) {
		(*tail)->Next = message;
	} else {
		*head = message;
	}
	*tail = message;
	++(*count);
	return true;
}

WSManagerClass::QueuedMessageType *WSManagerClass::Pop_Message(QueuedMessageType **head, QueuedMessageType **tail, int *count)
{
	if (*head == nullptr) {
		return nullptr;
	}

	QueuedMessageType *message = *head;
	*head = message->Next;
	if (*head == nullptr) {
		*tail = nullptr;
	}
	if (*count > 0) {
		--(*count);
	}
	return message;
}

void WSManagerClass::Purge_Private_For_Connection(int connection_id)
{
	QueuedMessageType *previous = nullptr;
	QueuedMessageType *message = PrivateHead;
	while (message != nullptr) {
		QueuedMessageType *next = message->Next;
		if (message->ConnectionId == connection_id) {
			if (previous != nullptr) {
				previous->Next = next;
			} else {
				PrivateHead = next;
			}
			if (PrivateTail == message) {
				PrivateTail = previous;
			}
			std::free(message->Data);
			std::free(message);
			--PrivateCount;
		} else {
			previous = message;
		}
		message = next;
	}
}

void WSManagerClass::Purge_Pending_Private_For_Cid(std::uint32_t client_id)
{
	QueuedMessageType *previous = nullptr;
	QueuedMessageType *message = PendingPrivateHead;
	while (message != nullptr) {
		QueuedMessageType *next = message->Next;
		if (message->SourceClientId == client_id) {
			if (previous != nullptr) {
				previous->Next = next;
			} else {
				PendingPrivateHead = next;
			}
			if (PendingPrivateTail == message) {
				PendingPrivateTail = previous;
			}
			std::free(message->Data);
			std::free(message);
			--PendingPrivateCount;
		} else {
			previous = message;
		}
		message = next;
	}
}

void WSManagerClass::Promote_Pending_Private(std::uint32_t client_id, int connection_id)
{
	QueuedMessageType *previous = nullptr;
	QueuedMessageType *message = PendingPrivateHead;
	while (message != nullptr) {
		QueuedMessageType *next = message->Next;
		if (message->SourceClientId == client_id) {
			if (previous != nullptr) {
				previous->Next = next;
			} else {
				PendingPrivateHead = next;
			}
			if (PendingPrivateTail == message) {
				PendingPrivateTail = previous;
			}
			--PendingPrivateCount;

			if (PrivateCount < MAX_QUEUED_MESSAGES) {
				message->ConnectionId = connection_id;
				message->Next = nullptr;
				if (PrivateTail != nullptr) {
					PrivateTail->Next = message;
				} else {
					PrivateHead = message;
				}
				PrivateTail = message;
				++PrivateCount;
			} else {
				std::free(message->Data);
				std::free(message);
			}
		} else {
			previous = message;
		}
		message = next;
	}
}
