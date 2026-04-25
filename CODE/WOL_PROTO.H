/*
**	Command & Conquer(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	Shared wire-format helpers for the modern Westwood Online backend.
**	Each application frame is carried inside one WebSocket binary message
**	and begins with this little-endian header:
**
**		uint16_t opcode;
**		uint16_t flags;
**		uint32_t payload_len;
*/
#ifndef WOL_PROTO_H
#define WOL_PROTO_H

#include <cstddef>
#include <cstdint>

namespace WolProto {

static constexpr std::size_t FRAME_HEADER_SIZE = 8;
static constexpr std::uint32_t PROTOCOL_VERSION = 1;
static constexpr std::uint32_t MAX_FRAME_PAYLOAD = 1024U * 1024U;

enum Opcode : std::uint16_t {
	OP_HELLO = 0x0001,
	OP_WELCOME = 0x0002,
	OP_ERROR = 0x0003,

	OP_CHANNEL_LIST = 0x0010,
	OP_CHANNEL_LIST_REPLY = 0x0011,
	OP_CHANNEL_JOIN = 0x0012,
	OP_CHANNEL_JOINED = 0x0013,
	OP_CHANNEL_USER_JOIN = 0x0014,
	OP_CHANNEL_USER_LEAVE = 0x0015,

	OP_CHAT = 0x0020,

	OP_GAME_CREATE = 0x0030,
	OP_GAME_JOIN = 0x0031,
	OP_GAME_LEAVE = 0x0032,
	OP_GAME_START = 0x0033,
	OP_GAME_LIST_REPLY = 0x0034,
	OP_GAME_MEMBERS = 0x0035,
	OP_GAME_STARTED = 0x0036,

	OP_RELAY_BROADCAST = 0x0100,
	OP_RELAY_PRIVATE = 0x0101,
	OP_RELAY_FROM = 0x0102,
	OP_PEER_LEFT = 0x0103,
};

enum ErrorCode : std::uint16_t {
	ERR_OK = 0,
	ERR_BAD_PROTOCOL = 1,
	ERR_BAD_NICKNAME = 2,
	ERR_NOT_LOGGED_IN = 3,
	ERR_NOT_IN_CHANNEL = 4,
	ERR_NOT_IN_GAME = 5,
	ERR_ALREADY_IN_GAME = 6,
	ERR_UNKNOWN_TARGET = 7,
	ERR_GAME_FULL = 8,
	ERR_GAME_STARTED = 9,
	ERR_INTERNAL = 100,
};

struct FrameView {
	std::uint16_t Opcode;
	std::uint16_t Flags;
	std::uint32_t PayloadLength;
	std::uint8_t const *Payload;
};

inline void Write_U8(std::uint8_t *&cursor, std::uint8_t value)
{
	*cursor++ = value;
}

inline void Write_U16(std::uint8_t *&cursor, std::uint16_t value)
{
	cursor[0] = static_cast<std::uint8_t>(value & 0xffU);
	cursor[1] = static_cast<std::uint8_t>((value >> 8) & 0xffU);
	cursor += 2;
}

inline void Write_U32(std::uint8_t *&cursor, std::uint32_t value)
{
	cursor[0] = static_cast<std::uint8_t>(value & 0xffU);
	cursor[1] = static_cast<std::uint8_t>((value >> 8) & 0xffU);
	cursor[2] = static_cast<std::uint8_t>((value >> 16) & 0xffU);
	cursor[3] = static_cast<std::uint8_t>((value >> 24) & 0xffU);
	cursor += 4;
}

inline std::uint8_t Read_U8(std::uint8_t const *&cursor)
{
	return *cursor++;
}

inline std::uint16_t Read_U16(std::uint8_t const *&cursor)
{
	const std::uint16_t value = static_cast<std::uint16_t>(cursor[0])
		| (static_cast<std::uint16_t>(cursor[1]) << 8);
	cursor += 2;
	return value;
}

inline std::uint32_t Read_U32(std::uint8_t const *&cursor)
{
	const std::uint32_t value = static_cast<std::uint32_t>(cursor[0])
		| (static_cast<std::uint32_t>(cursor[1]) << 8)
		| (static_cast<std::uint32_t>(cursor[2]) << 16)
		| (static_cast<std::uint32_t>(cursor[3]) << 24);
	cursor += 4;
	return value;
}

inline void Encode_Frame_Header(std::uint8_t *destination, std::uint16_t opcode, std::uint16_t flags, std::uint32_t payload_length)
{
	destination[0] = static_cast<std::uint8_t>(opcode & 0xffU);
	destination[1] = static_cast<std::uint8_t>((opcode >> 8) & 0xffU);
	destination[2] = static_cast<std::uint8_t>(flags & 0xffU);
	destination[3] = static_cast<std::uint8_t>((flags >> 8) & 0xffU);
	destination[4] = static_cast<std::uint8_t>(payload_length & 0xffU);
	destination[5] = static_cast<std::uint8_t>((payload_length >> 8) & 0xffU);
	destination[6] = static_cast<std::uint8_t>((payload_length >> 16) & 0xffU);
	destination[7] = static_cast<std::uint8_t>((payload_length >> 24) & 0xffU);
}

inline bool Parse_Frame(std::uint8_t const *buffer, std::size_t buffer_length, FrameView *frame)
{
	if (buffer == nullptr || frame == nullptr || buffer_length < FRAME_HEADER_SIZE) {
		return false;
	}

	const std::uint32_t payload_length = static_cast<std::uint32_t>(buffer[4])
		| (static_cast<std::uint32_t>(buffer[5]) << 8)
		| (static_cast<std::uint32_t>(buffer[6]) << 16)
		| (static_cast<std::uint32_t>(buffer[7]) << 24);

	if (payload_length > MAX_FRAME_PAYLOAD || buffer_length < FRAME_HEADER_SIZE + payload_length) {
		return false;
	}

	frame->Opcode = static_cast<std::uint16_t>(buffer[0])
		| (static_cast<std::uint16_t>(buffer[1]) << 8);
	frame->Flags = static_cast<std::uint16_t>(buffer[2])
		| (static_cast<std::uint16_t>(buffer[3]) << 8);
	frame->PayloadLength = payload_length;
	frame->Payload = buffer + FRAME_HEADER_SIZE;
	return true;
}

}

#endif
