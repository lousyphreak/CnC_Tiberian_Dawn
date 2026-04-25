/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	Native WebSocket client for the modern WOL backend.
**	This is intentionally minimal: ws:// only, binary frames only, no TLS.
*/
#include "WSCLIENT.H"

#include "SOCKETS.H"

#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

namespace {

struct QueuedBinaryMessage {
	std::uint8_t *Data;
	std::size_t Length;
	QueuedBinaryMessage *Next;
};

struct NativeClientType {
	SocketHandle Socket;
	bool SocketSystemReady;
	bool ConnectPending;
	bool HandshakeSent;
	bool HandshakeDone;
	bool CloseRequested;
	char Host[256];
	char Path[512];
	std::uint16_t Port;
	char ExpectedAccept[64];
	std::uint8_t *HandshakeBuffer;
	std::size_t HandshakeLength;
	std::size_t HandshakeCapacity;
	std::uint8_t *InputBuffer;
	std::size_t InputLength;
	std::size_t InputCapacity;
	std::uint8_t *OutputBuffer;
	std::size_t OutputLength;
	std::size_t OutputCapacity;
	std::uint8_t *FragmentBuffer;
	std::size_t FragmentLength;
	std::size_t FragmentCapacity;
	std::uint8_t FragmentOpcode;
	QueuedBinaryMessage *ReceiveHead;
	QueuedBinaryMessage *ReceiveTail;
};

static int SocketSystemUsers = 0;

void Free_Message_List(QueuedBinaryMessage **head, QueuedBinaryMessage **tail)
{
	while (*head != nullptr) {
		QueuedBinaryMessage *next = (*head)->Next;
		std::free((*head)->Data);
		std::free(*head);
		*head = next;
	}

	*tail = nullptr;
}

void Enqueue_Message(QueuedBinaryMessage **head, QueuedBinaryMessage **tail, std::uint8_t *data, std::size_t length)
{
	QueuedBinaryMessage *entry = static_cast<QueuedBinaryMessage *>(std::malloc(sizeof(QueuedBinaryMessage)));
	if (entry == nullptr) {
		std::free(data);
		return;
	}

	entry->Data = data;
	entry->Length = length;
	entry->Next = nullptr;

	if (*tail != nullptr) {
		(*tail)->Next = entry;
	} else {
		*head = entry;
	}

	*tail = entry;
}

bool Dequeue_Message(QueuedBinaryMessage **head, QueuedBinaryMessage **tail, void **data, std::size_t *length)
{
	if (*head == nullptr || data == nullptr || length == nullptr) {
		return false;
	}

	QueuedBinaryMessage *entry = *head;
	*head = entry->Next;
	if (*head == nullptr) {
		*tail = nullptr;
	}

	*data = entry->Data;
	*length = entry->Length;
	std::free(entry);
	return true;
}

int Count_Messages(QueuedBinaryMessage *head)
{
	int count = 0;
	while (head != nullptr) {
		++count;
		head = head->Next;
	}
	return count;
}

bool Acquire_Socket_System(NativeClientType *client)
{
	if (client->SocketSystemReady) {
		return true;
	}

	if (SocketSystemUsers == 0 && !Socket_Startup()) {
		return false;
	}

	++SocketSystemUsers;
	client->SocketSystemReady = true;
	return true;
}

void Release_Socket_System(NativeClientType *client)
{
	if (!client->SocketSystemReady) {
		return;
	}

	client->SocketSystemReady = false;
	if (SocketSystemUsers > 0) {
		--SocketSystemUsers;
		if (SocketSystemUsers == 0) {
			Socket_Cleanup();
		}
	}
}

void Close_Socket_Only(NativeClientType *client)
{
	if (client->Socket != INVALID_SOCKET_HANDLE) {
		Socket_Close(client->Socket);
		client->Socket = INVALID_SOCKET_HANDLE;
	}
}

void Reset_Stream_Buffers(NativeClientType *client)
{
	std::free(client->HandshakeBuffer);
	client->HandshakeBuffer = nullptr;
	client->HandshakeLength = 0;
	client->HandshakeCapacity = 0;

	std::free(client->InputBuffer);
	client->InputBuffer = nullptr;
	client->InputLength = 0;
	client->InputCapacity = 0;

	std::free(client->OutputBuffer);
	client->OutputBuffer = nullptr;
	client->OutputLength = 0;
	client->OutputCapacity = 0;

	std::free(client->FragmentBuffer);
	client->FragmentBuffer = nullptr;
	client->FragmentLength = 0;
	client->FragmentCapacity = 0;
	client->FragmentOpcode = 0;
}

void Reset_Client_State(NativeClientType *client)
{
	Close_Socket_Only(client);
	Reset_Stream_Buffers(client);
	Free_Message_List(&client->ReceiveHead, &client->ReceiveTail);
	client->ConnectPending = false;
	client->HandshakeSent = false;
	client->HandshakeDone = false;
	client->CloseRequested = false;
	client->Host[0] = '\0';
	client->Path[0] = '\0';
	client->ExpectedAccept[0] = '\0';
	client->Port = 0;
}

bool Ensure_Buffer_Capacity(std::uint8_t **buffer, std::size_t *capacity, std::size_t required)
{
	if (*capacity >= required) {
		return true;
	}

	std::size_t new_capacity = (*capacity == 0) ? 1024 : *capacity;
	while (new_capacity < required) {
		new_capacity *= 2;
	}

	std::uint8_t *new_buffer = static_cast<std::uint8_t *>(std::realloc(*buffer, new_capacity));
	if (new_buffer == nullptr) {
		return false;
	}

	*buffer = new_buffer;
	*capacity = new_capacity;
	return true;
}

bool Append_Buffer(std::uint8_t **buffer, std::size_t *length, std::size_t *capacity, void const *data, std::size_t data_length)
{
	if (data_length == 0) {
		return true;
	}

	if (!Ensure_Buffer_Capacity(buffer, capacity, *length + data_length)) {
		return false;
	}

	std::memcpy(*buffer + *length, data, data_length);
	*length += data_length;
	return true;
}

void Consume_Buffer_Prefix(std::uint8_t *buffer, std::size_t *length, std::size_t consumed)
{
	if (consumed >= *length) {
		*length = 0;
		return;
	}

	std::memmove(buffer, buffer + consumed, *length - consumed);
	*length -= consumed;
}

char To_Lower_ASCII(char value)
{
	if (value >= 'A' && value <= 'Z') {
		return static_cast<char>(value - 'A' + 'a');
	}

	return value;
}

bool Parse_WebSocket_URL(char const *url, bool *secure, char *host, std::size_t host_capacity, std::uint16_t *port, char *path, std::size_t path_capacity)
{
	if (url == nullptr || secure == nullptr || host == nullptr || port == nullptr || path == nullptr) {
		return false;
	}

	char const *cursor = url;
	if (std::strncmp(cursor, "ws://", 5) == 0) {
		*secure = false;
		cursor += 5;
	} else if (std::strncmp(cursor, "wss://", 6) == 0) {
		*secure = true;
		cursor += 6;
	} else {
		return false;
	}

	char const *host_end = cursor;
	while (*host_end != '\0' && *host_end != ':' && *host_end != '/') {
		++host_end;
	}

	const std::size_t host_length = static_cast<std::size_t>(host_end - cursor);
	if (host_length == 0 || host_length >= host_capacity) {
		return false;
	}

	std::memcpy(host, cursor, host_length);
	host[host_length] = '\0';
	cursor = host_end;

	*port = *secure ? 443 : 80;
	if (*cursor == ':') {
		++cursor;
		unsigned int parsed_port = 0;
		int digits = 0;
		while (*cursor >= '0' && *cursor <= '9') {
			parsed_port = parsed_port * 10U + static_cast<unsigned int>(*cursor - '0');
			++cursor;
			++digits;
			if (parsed_port > 65535U) {
				return false;
			}
		}

		if (digits == 0) {
			return false;
		}

		*port = static_cast<std::uint16_t>(parsed_port);
	}

	if (*cursor == '\0') {
		std::snprintf(path, path_capacity, "/");
		return true;
	}

	if (*cursor != '/') {
		return false;
	}

	std::snprintf(path, path_capacity, "%s", cursor);
	return true;
}

struct SHA1StateType {
	std::uint32_t State[5];
	std::uint64_t Count;
	std::uint8_t Buffer[64];
};

std::uint32_t Rotate_Left(std::uint32_t value, int bits)
{
	return (value << bits) | (value >> (32 - bits));
}

void SHA1_Transform(std::uint32_t state[5], std::uint8_t const block[64])
{
	std::uint32_t words[80];
	for (int index = 0; index < 16; ++index) {
		words[index] = (static_cast<std::uint32_t>(block[index * 4 + 0]) << 24)
			| (static_cast<std::uint32_t>(block[index * 4 + 1]) << 16)
			| (static_cast<std::uint32_t>(block[index * 4 + 2]) << 8)
			| static_cast<std::uint32_t>(block[index * 4 + 3]);
	}

	for (int index = 16; index < 80; ++index) {
		words[index] = Rotate_Left(words[index - 3] ^ words[index - 8] ^ words[index - 14] ^ words[index - 16], 1);
	}

	std::uint32_t a = state[0];
	std::uint32_t b = state[1];
	std::uint32_t c = state[2];
	std::uint32_t d = state[3];
	std::uint32_t e = state[4];

	for (int index = 0; index < 80; ++index) {
		std::uint32_t function;
		std::uint32_t constant;

		if (index < 20) {
			function = (b & c) | ((~b) & d);
			constant = 0x5A827999U;
		} else if (index < 40) {
			function = b ^ c ^ d;
			constant = 0x6ED9EBA1U;
		} else if (index < 60) {
			function = (b & c) | (b & d) | (c & d);
			constant = 0x8F1BBCDCU;
		} else {
			function = b ^ c ^ d;
			constant = 0xCA62C1D6U;
		}

		const std::uint32_t temp = Rotate_Left(a, 5) + function + e + constant + words[index];
		e = d;
		d = c;
		c = Rotate_Left(b, 30);
		b = a;
		a = temp;
	}

	state[0] += a;
	state[1] += b;
	state[2] += c;
	state[3] += d;
	state[4] += e;
}

void SHA1_Init(SHA1StateType *sha1)
{
	sha1->State[0] = 0x67452301U;
	sha1->State[1] = 0xEFCDAB89U;
	sha1->State[2] = 0x98BADCFEU;
	sha1->State[3] = 0x10325476U;
	sha1->State[4] = 0xC3D2E1F0U;
	sha1->Count = 0;
}

void SHA1_Update(SHA1StateType *sha1, std::uint8_t const *data, std::size_t length)
{
	std::size_t offset = static_cast<std::size_t>(sha1->Count & 63U);
	sha1->Count += length;

	if (offset + length >= 64) {
		const std::size_t fill = 64 - offset;
		std::memcpy(sha1->Buffer + offset, data, fill);
		SHA1_Transform(sha1->State, sha1->Buffer);
		data += fill;
		length -= fill;
		offset = 0;

		while (length >= 64) {
			SHA1_Transform(sha1->State, data);
			data += 64;
			length -= 64;
		}
	}

	if (length > 0) {
		std::memcpy(sha1->Buffer + offset, data, length);
	}
}

void SHA1_Final(SHA1StateType *sha1, std::uint8_t output[20])
{
	const std::uint64_t bit_count = sha1->Count * 8U;
	const std::uint8_t high_bit = 0x80U;
	const std::uint8_t zero = 0;
	SHA1_Update(sha1, &high_bit, 1);

	while ((sha1->Count & 63U) != 56U) {
		SHA1_Update(sha1, &zero, 1);
	}

	std::uint8_t encoded_length[8];
	for (int index = 0; index < 8; ++index) {
		encoded_length[index] = static_cast<std::uint8_t>(bit_count >> (56 - index * 8));
	}

	SHA1_Update(sha1, encoded_length, sizeof(encoded_length));

	for (int index = 0; index < 5; ++index) {
		output[index * 4 + 0] = static_cast<std::uint8_t>(sha1->State[index] >> 24);
		output[index * 4 + 1] = static_cast<std::uint8_t>(sha1->State[index] >> 16);
		output[index * 4 + 2] = static_cast<std::uint8_t>(sha1->State[index] >> 8);
		output[index * 4 + 3] = static_cast<std::uint8_t>(sha1->State[index]);
	}
}

char const *Base64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

void Base64_Encode(std::uint8_t const *data, std::size_t length, char *output)
{
	std::size_t read_index = 0;
	std::size_t write_index = 0;

	while (read_index + 3 <= length) {
		const std::uint32_t value = (static_cast<std::uint32_t>(data[read_index]) << 16)
			| (static_cast<std::uint32_t>(data[read_index + 1]) << 8)
			| static_cast<std::uint32_t>(data[read_index + 2]);

		output[write_index++] = Base64Alphabet[(value >> 18) & 0x3fU];
		output[write_index++] = Base64Alphabet[(value >> 12) & 0x3fU];
		output[write_index++] = Base64Alphabet[(value >> 6) & 0x3fU];
		output[write_index++] = Base64Alphabet[value & 0x3fU];
		read_index += 3;
	}

	const std::size_t remainder = length - read_index;
	if (remainder == 1) {
		const std::uint32_t value = static_cast<std::uint32_t>(data[read_index]) << 16;
		output[write_index++] = Base64Alphabet[(value >> 18) & 0x3fU];
		output[write_index++] = Base64Alphabet[(value >> 12) & 0x3fU];
		output[write_index++] = '=';
		output[write_index++] = '=';
	} else if (remainder == 2) {
		const std::uint32_t value = (static_cast<std::uint32_t>(data[read_index]) << 16)
			| (static_cast<std::uint32_t>(data[read_index + 1]) << 8);
		output[write_index++] = Base64Alphabet[(value >> 18) & 0x3fU];
		output[write_index++] = Base64Alphabet[(value >> 12) & 0x3fU];
		output[write_index++] = Base64Alphabet[(value >> 6) & 0x3fU];
		output[write_index++] = '=';
	}

	output[write_index] = '\0';
}

std::uint32_t Next_Random_Word(void)
{
	static std::uint32_t state = 0;
	if (state == 0) {
		const std::time_t now = std::time(nullptr);
		state = static_cast<std::uint32_t>(now) ^ 0xA5A5F00DU;
	}

	state ^= (state << 13);
	state ^= (state >> 17);
	state ^= (state << 5);
	return state;
}

void Generate_WebSocket_Key(char *output)
{
	std::uint8_t bytes[16];
	for (int index = 0; index < 16; index += 4) {
		const std::uint32_t random_value = Next_Random_Word();
		bytes[index + 0] = static_cast<std::uint8_t>(random_value & 0xffU);
		bytes[index + 1] = static_cast<std::uint8_t>((random_value >> 8) & 0xffU);
		bytes[index + 2] = static_cast<std::uint8_t>((random_value >> 16) & 0xffU);
		bytes[index + 3] = static_cast<std::uint8_t>((random_value >> 24) & 0xffU);
	}

	Base64_Encode(bytes, sizeof(bytes), output);
}

bool Compute_Accept_Header(char const *client_key, char *output)
{
	static char const *WebSocketGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

	char combined[128];
	const int written = std::snprintf(combined, sizeof(combined), "%s%s", client_key, WebSocketGUID);
	if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(combined)) {
		return false;
	}

	SHA1StateType sha1;
	SHA1_Init(&sha1);
	SHA1_Update(&sha1, reinterpret_cast<std::uint8_t const *>(combined), static_cast<std::size_t>(written));

	std::uint8_t digest[20];
	SHA1_Final(&sha1, digest);
	Base64_Encode(digest, sizeof(digest), output);
	return true;
}

bool Queue_Output_Data(NativeClientType *client, void const *data, std::size_t length)
{
	return Append_Buffer(&client->OutputBuffer, &client->OutputLength, &client->OutputCapacity, data, length);
}

bool Flush_Output_Buffer(NativeClientType *client)
{
	while (client->OutputLength > 0) {
		const int wrote = Socket_Write(client->Socket, client->OutputBuffer, static_cast<int>(client->OutputLength));
		if (wrote > 0) {
			Consume_Buffer_Prefix(client->OutputBuffer, &client->OutputLength, static_cast<std::size_t>(wrote));
			continue;
		}

		if (wrote < 0 && Socket_Error_Is_Would_Block(Socket_Get_Last_Error())) {
			return true;
		}

		return false;
	}

	return true;
}

bool Queue_WebSocket_Frame(NativeClientType *client, std::uint8_t opcode, std::uint8_t const *payload, std::size_t payload_length)
{
	std::uint8_t header[14];
	std::size_t header_length = 0;

	header[header_length++] = static_cast<std::uint8_t>(0x80U | (opcode & 0x0fU));
	if (payload_length < 126U) {
		header[header_length++] = static_cast<std::uint8_t>(0x80U | payload_length);
	} else if (payload_length <= 0xffffU) {
		header[header_length++] = static_cast<std::uint8_t>(0x80U | 126U);
		header[header_length++] = static_cast<std::uint8_t>((payload_length >> 8) & 0xffU);
		header[header_length++] = static_cast<std::uint8_t>(payload_length & 0xffU);
	} else {
		header[header_length++] = static_cast<std::uint8_t>(0x80U | 127U);
		for (int shift = 7; shift >= 0; --shift) {
			header[header_length++] = static_cast<std::uint8_t>((static_cast<std::uint64_t>(payload_length) >> (shift * 8)) & 0xffU);
		}
	}

	const std::uint32_t mask = Next_Random_Word();
	header[header_length++] = static_cast<std::uint8_t>(mask & 0xffU);
	header[header_length++] = static_cast<std::uint8_t>((mask >> 8) & 0xffU);
	header[header_length++] = static_cast<std::uint8_t>((mask >> 16) & 0xffU);
	header[header_length++] = static_cast<std::uint8_t>((mask >> 24) & 0xffU);

	if (!Queue_Output_Data(client, header, header_length)) {
		return false;
	}

	if (payload_length == 0) {
		return true;
	}

	std::uint8_t *masked_payload = static_cast<std::uint8_t *>(std::malloc(payload_length));
	if (masked_payload == nullptr) {
		return false;
	}

	std::uint8_t mask_bytes[4];
	mask_bytes[0] = static_cast<std::uint8_t>(mask & 0xffU);
	mask_bytes[1] = static_cast<std::uint8_t>((mask >> 8) & 0xffU);
	mask_bytes[2] = static_cast<std::uint8_t>((mask >> 16) & 0xffU);
	mask_bytes[3] = static_cast<std::uint8_t>((mask >> 24) & 0xffU);

	for (std::size_t index = 0; index < payload_length; ++index) {
		masked_payload[index] = payload[index] ^ mask_bytes[index & 3U];
	}

	const bool queued = Queue_Output_Data(client, masked_payload, payload_length);
	std::free(masked_payload);
	return queued;
}

bool Begin_Handshake(NativeClientType *client)
{
	char key[32];
	Generate_WebSocket_Key(key);
	if (!Compute_Accept_Header(key, client->ExpectedAccept)) {
		return false;
	}

	char request[1024];
	const int written = std::snprintf(request,
		sizeof(request),
		"GET %s HTTP/1.1\r\n"
		"Host: %s:%u\r\n"
		"Upgrade: websocket\r\n"
		"Connection: Upgrade\r\n"
		"Sec-WebSocket-Key: %s\r\n"
		"Sec-WebSocket-Version: 13\r\n"
		"\r\n",
		client->Path,
		client->Host,
		static_cast<unsigned int>(client->Port),
		key);

	if (written <= 0 || static_cast<std::size_t>(written) >= sizeof(request)) {
		return false;
	}

	client->HandshakeSent = true;
	return Queue_Output_Data(client, request, static_cast<std::size_t>(written));
}

int Consume_Handshake_Response(NativeClientType *client)
{
	if (client->HandshakeLength < 4) {
		return 0;
	}

	for (std::size_t index = 3; index < client->HandshakeLength; ++index) {
		if (client->HandshakeBuffer[index - 3] != '\r'
		 || client->HandshakeBuffer[index - 2] != '\n'
		 || client->HandshakeBuffer[index - 1] != '\r'
		 || client->HandshakeBuffer[index] != '\n') {
			continue;
		}

		const std::size_t header_length = index + 1;
		char *header_copy = static_cast<char *>(std::malloc(header_length + 1));
		char *lower_copy = static_cast<char *>(std::malloc(header_length + 1));
		if (header_copy == nullptr || lower_copy == nullptr) {
			std::free(header_copy);
			std::free(lower_copy);
			return -1;
		}

		std::memcpy(header_copy, client->HandshakeBuffer, header_length);
		header_copy[header_length] = '\0';
		for (std::size_t lower_index = 0; lower_index < header_length; ++lower_index) {
			lower_copy[lower_index] = To_Lower_ASCII(header_copy[lower_index]);
		}
		lower_copy[header_length] = '\0';

		if (std::strncmp(header_copy, "HTTP/", 5) != 0 || std::strstr(header_copy, " 101 ") == nullptr) {
			std::free(header_copy);
			std::free(lower_copy);
			return -1;
		}

		char const *accept_header = "sec-websocket-accept:";
		char *found_header = std::strstr(lower_copy, accept_header);
		if (found_header == nullptr) {
			std::free(header_copy);
			std::free(lower_copy);
			return -1;
		}

		std::size_t value_offset = static_cast<std::size_t>(found_header - lower_copy) + std::strlen(accept_header);
		while (value_offset < header_length && (header_copy[value_offset] == ' ' || header_copy[value_offset] == '\t')) {
			++value_offset;
		}

		std::size_t value_end = value_offset;
		while (value_end < header_length && header_copy[value_end] != '\r') {
			++value_end;
		}

		const std::size_t expected_length = std::strlen(client->ExpectedAccept);
		const bool accept_matches = (value_end - value_offset == expected_length)
			&& (std::memcmp(header_copy + value_offset, client->ExpectedAccept, expected_length) == 0);

		std::free(header_copy);
		std::free(lower_copy);

		if (!accept_matches) {
			return -1;
		}

		const std::size_t trailing_bytes = client->HandshakeLength - header_length;
		if (trailing_bytes > 0) {
			if (!Append_Buffer(&client->InputBuffer,
				&client->InputLength,
				&client->InputCapacity,
				client->HandshakeBuffer + header_length,
				trailing_bytes)) {
				return -1;
			}
		}

		std::free(client->HandshakeBuffer);
		client->HandshakeBuffer = nullptr;
		client->HandshakeLength = 0;
		client->HandshakeCapacity = 0;
		client->HandshakeDone = true;
		return 1;
	}

	return 0;
}

bool Queue_Incoming_Message(NativeClientType *client, std::uint8_t const *payload, std::size_t payload_length)
{
	std::uint8_t *copy = nullptr;
	if (payload_length > 0) {
		copy = static_cast<std::uint8_t *>(std::malloc(payload_length));
		if (copy == nullptr) {
			return false;
		}
		std::memcpy(copy, payload, payload_length);
	}

	Enqueue_Message(&client->ReceiveHead, &client->ReceiveTail, copy, payload_length);
	return true;
}

int Consume_WebSocket_Frame(NativeClientType *client)
{
	if (client->InputLength < 2) {
		return 0;
	}

	const std::uint8_t first = client->InputBuffer[0];
	const std::uint8_t second = client->InputBuffer[1];
	const bool final_fragment = (first & 0x80U) != 0;
	const std::uint8_t opcode = static_cast<std::uint8_t>(first & 0x0fU);
	const bool masked = (second & 0x80U) != 0;

	if (masked) {
		return -1;
	}

	std::uint64_t payload_length = second & 0x7fU;
	std::size_t offset = 2;

	if (payload_length == 126U) {
		if (client->InputLength < offset + 2) {
			return 0;
		}

		payload_length = (static_cast<std::uint64_t>(client->InputBuffer[offset]) << 8)
			| static_cast<std::uint64_t>(client->InputBuffer[offset + 1]);
		offset += 2;
	} else if (payload_length == 127U) {
		if (client->InputLength < offset + 8) {
			return 0;
		}

		payload_length = 0;
		for (int index = 0; index < 8; ++index) {
			payload_length = (payload_length << 8) | static_cast<std::uint64_t>(client->InputBuffer[offset + index]);
		}
		offset += 8;
	}

	if (payload_length > 16ULL * 1024ULL * 1024ULL) {
		return -1;
	}

	if (client->InputLength < offset + payload_length) {
		return 0;
	}

	std::uint8_t const *payload = client->InputBuffer + offset;

	switch (opcode) {
		case 0x0: {
			if (client->FragmentOpcode == 0) {
				return -1;
			}

			if (!Append_Buffer(&client->FragmentBuffer,
				&client->FragmentLength,
				&client->FragmentCapacity,
				payload,
				static_cast<std::size_t>(payload_length))) {
				return -1;
			}

			if (final_fragment) {
				if (client->FragmentOpcode == 0x2 && !Queue_Incoming_Message(client, client->FragmentBuffer, client->FragmentLength)) {
					return -1;
				}
				client->FragmentLength = 0;
				client->FragmentOpcode = 0;
			}
			break;
		}

		case 0x1:
		case 0x2: {
			if (final_fragment) {
				if (opcode == 0x2 && !Queue_Incoming_Message(client, payload, static_cast<std::size_t>(payload_length))) {
					return -1;
				}
			} else {
				client->FragmentOpcode = opcode;
				client->FragmentLength = 0;
				if (!Append_Buffer(&client->FragmentBuffer,
					&client->FragmentLength,
					&client->FragmentCapacity,
					payload,
					static_cast<std::size_t>(payload_length))) {
					return -1;
				}
			}
			break;
		}

		case 0x8:
			client->CloseRequested = true;
			return -1;

		case 0x9:
			if (!Queue_WebSocket_Frame(client, 0xA, payload, static_cast<std::size_t>(payload_length))) {
				return -1;
			}
			break;

		case 0xA:
			break;

		default:
			return -1;
	}

	return static_cast<int>(offset + payload_length);
}

}

WSClientClass::WSClientClass()
{
	State = STATE_DISCONNECTED;
	Impl = std::calloc(1, sizeof(NativeClientType));
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	client->Socket = INVALID_SOCKET_HANDLE;
}

WSClientClass::~WSClientClass()
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	Close();
	Release_Socket_System(client);
	std::free(Impl);
	Impl = nullptr;
}

bool WSClientClass::Connect(char const *url)
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	Close();

	if (!Acquire_Socket_System(client)) {
		State = STATE_FAILED;
		return false;
	}

	bool secure = false;
	if (!Parse_WebSocket_URL(url, &secure, client->Host, sizeof(client->Host), &client->Port, client->Path, sizeof(client->Path))) {
		State = STATE_FAILED;
		return false;
	}

	if (secure) {
		State = STATE_FAILED;
		return false;
	}

	std::uint32_t resolved_address = 0;
	SocketHostEntry host_entry;
	if (!Socket_Address_From_String(client->Host, &resolved_address)) {
		if (!Socket_Lookup_Host_By_Name(client->Host, &host_entry) || host_entry.AddressCount <= 0) {
			State = STATE_FAILED;
			return false;
		}

		resolved_address = host_entry.Addresses[0];
	}

	client->Socket = Socket_Open(SOCKET_TYPE_STREAM, SOCKET_PROTOCOL_TCP);
	if (client->Socket == INVALID_SOCKET_HANDLE) {
		State = STATE_FAILED;
		return false;
	}

	Socket_Set_NonBlocking(client->Socket, true);
	Socket_Set_Integer_Option(client->Socket, SOCKET_OPTION_TCP_NO_DELAY, 1);

	SocketAddress socket_address;
	socket_address.Address = resolved_address;
	socket_address.Port = Socket_Host_To_Network16(client->Port);

	const int connect_result = Socket_Connect(client->Socket, &socket_address);
	if (connect_result < 0) {
		const int error_code = Socket_Get_Last_Error();
		if (!Socket_Error_Is_Would_Block(error_code)) {
			Close_Socket_Only(client);
			State = STATE_FAILED;
			return false;
		}
		client->ConnectPending = true;
	}

	State = STATE_CONNECTING;
	return true;
}

void WSClientClass::Close(void)
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	Reset_Client_State(client);
	State = STATE_DISCONNECTED;
}

int WSClientClass::Poll(void)
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	if (client->Socket == INVALID_SOCKET_HANDLE) {
		return Receive_Queue_Size();
	}

	if (State == STATE_CONNECTING && (!client->ConnectPending || Socket_Is_Write_Ready(client->Socket))) {
		client->ConnectPending = false;
		if (!client->HandshakeSent && !Begin_Handshake(client)) {
			Reset_Stream_Buffers(client);
			Close_Socket_Only(client);
			State = STATE_FAILED;
			return Receive_Queue_Size();
		}
	}

	if (client->OutputLength > 0 && !Flush_Output_Buffer(client)) {
		Close_Socket_Only(client);
		State = STATE_FAILED;
		return Receive_Queue_Size();
	}

	std::uint8_t temp_buffer[2048];
	for (;;) {
		const int read_result = Socket_Read(client->Socket, temp_buffer, sizeof(temp_buffer));
		if (read_result > 0) {
			if (!client->HandshakeDone) {
				if (!Append_Buffer(&client->HandshakeBuffer,
					&client->HandshakeLength,
					&client->HandshakeCapacity,
					temp_buffer,
					static_cast<std::size_t>(read_result))) {
					Close_Socket_Only(client);
					State = STATE_FAILED;
					return Receive_Queue_Size();
				}
			} else if (!Append_Buffer(&client->InputBuffer,
				&client->InputLength,
				&client->InputCapacity,
				temp_buffer,
				static_cast<std::size_t>(read_result))) {
				Close_Socket_Only(client);
				State = STATE_FAILED;
				return Receive_Queue_Size();
			}
			continue;
		}

		if (read_result == 0) {
			Close_Socket_Only(client);
			State = STATE_DISCONNECTED;
			return Receive_Queue_Size();
		}

		const int error_code = Socket_Get_Last_Error();
		if (Socket_Error_Is_Would_Block(error_code)) {
			break;
		}

		Close_Socket_Only(client);
		State = STATE_FAILED;
		return Receive_Queue_Size();
	}

	if (!client->HandshakeDone && client->HandshakeSent) {
		const int handshake_state = Consume_Handshake_Response(client);
		if (handshake_state < 0) {
			Close_Socket_Only(client);
			State = STATE_FAILED;
			return Receive_Queue_Size();
		}

		if (handshake_state > 0) {
			State = STATE_OPEN;
		}
	}

	while (client->HandshakeDone && client->InputLength > 0) {
		const int consumed = Consume_WebSocket_Frame(client);
		if (consumed < 0) {
			Close_Socket_Only(client);
			if (client->CloseRequested) {
				State = STATE_DISCONNECTED;
			} else {
				State = STATE_FAILED;
			}
			return Receive_Queue_Size();
		}

		if (consumed == 0) {
			break;
		}

		Consume_Buffer_Prefix(client->InputBuffer, &client->InputLength, static_cast<std::size_t>(consumed));
	}

	return Receive_Queue_Size();
}

bool WSClientClass::Send_Binary(void const *data, std::size_t length)
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	if (State != STATE_OPEN || client->Socket == INVALID_SOCKET_HANDLE) {
		return false;
	}

	if (!Queue_WebSocket_Frame(client, 0x2, static_cast<std::uint8_t const *>(data), length)) {
		State = STATE_FAILED;
		return false;
	}

	if (!Flush_Output_Buffer(client)) {
		Close_Socket_Only(client);
		State = STATE_FAILED;
		return false;
	}

	return true;
}

bool WSClientClass::Receive_Binary(void **data, std::size_t *length)
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	return Dequeue_Message(&client->ReceiveHead, &client->ReceiveTail, data, length);
}

int WSClientClass::Receive_Queue_Size(void) const
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	return Count_Messages(client->ReceiveHead);
}

bool WSClientClass::Has_Pending_Output(void) const
{
	NativeClientType *client = static_cast<NativeClientType *>(Impl);
	return client->OutputLength > 0;
}
