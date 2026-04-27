//! Application-layer framing carried inside WebSocket binary messages.
//!
//! Frame layout (little-endian):
//!   u16 opcode | u16 flags | u32 payload_len | u8[payload_len] payload
//!
//! One WebSocket binary message carries exactly one frame.

const std = @import("std");

pub const HEADER_SIZE: usize = 8;
pub const PROTOCOL_VERSION: u32 = 1;
pub const MAX_PAYLOAD: u32 = 1 * 1024 * 1024;

pub const Opcode = enum(u16) {
    hello = 0x0001,
    welcome = 0x0002,
    err = 0x0003,

    channel_list = 0x0010,
    channel_list_reply = 0x0011,
    channel_join = 0x0012,
    channel_joined = 0x0013,
    channel_user_join = 0x0014,
    channel_user_leave = 0x0015,

    chat = 0x0020,

    game_create = 0x0030,
    game_join = 0x0031,
    game_leave = 0x0032,
    game_start = 0x0033,
    game_list_reply = 0x0034,
    game_members = 0x0035,
    game_started = 0x0036,

    relay_broadcast = 0x0100,
    relay_private = 0x0101,
    relay_from = 0x0102,
    peer_left = 0x0103,

    _,
};

pub const ErrorCode = enum(u16) {
    ok = 0,
    bad_protocol = 1,
    bad_nickname = 2,
    not_logged_in = 3,
    not_in_channel = 4,
    not_in_game = 5,
    already_in_game = 6,
    unknown_target = 7,
    game_full = 8,
    game_started = 9,
    internal = 100,
    _,
};

pub const Frame = struct {
    opcode: u16,
    flags: u16,
    payload: []const u8,
};

pub fn encodeHeader(dest: *[HEADER_SIZE]u8, opcode: u16, flags: u16, payload_len: u32) void {
    std.mem.writeInt(u16, dest[0..2], opcode, .little);
    std.mem.writeInt(u16, dest[2..4], flags, .little);
    std.mem.writeInt(u32, dest[4..8], payload_len, .little);
}

pub const ParseError = error{
    ShortHeader,
    PayloadTooLarge,
    Truncated,
};

pub fn parse(buf: []const u8) ParseError!Frame {
    if (buf.len < HEADER_SIZE) return error.ShortHeader;
    const opcode = std.mem.readInt(u16, buf[0..2], .little);
    const flags = std.mem.readInt(u16, buf[2..4], .little);
    const len = std.mem.readInt(u32, buf[4..8], .little);
    if (len > MAX_PAYLOAD) return error.PayloadTooLarge;
    if (buf.len < HEADER_SIZE + len) return error.Truncated;
    return .{
        .opcode = opcode,
        .flags = flags,
        .payload = buf[HEADER_SIZE .. HEADER_SIZE + len],
    };
}

/// Writes a complete frame into `out_writer`.
pub fn writeFrame(writer: anytype, opcode: Opcode, flags: u16, payload: []const u8) !void {
    if (payload.len > MAX_PAYLOAD) return error.PayloadTooLarge;
    var hdr: [HEADER_SIZE]u8 = undefined;
    encodeHeader(&hdr, @intFromEnum(opcode), flags, @intCast(payload.len));
    try writer.writeAll(&hdr);
    try writer.writeAll(payload);
}

/// Builds a frame into a newly-allocated buffer (header + payload).
pub fn buildFrame(
    allocator: std.mem.Allocator,
    opcode: Opcode,
    flags: u16,
    payload: []const u8,
) ![]u8 {
    if (payload.len > MAX_PAYLOAD) return error.PayloadTooLarge;
    const buf = try allocator.alloc(u8, HEADER_SIZE + payload.len);
    encodeHeader(buf[0..HEADER_SIZE], @intFromEnum(opcode), flags, @intCast(payload.len));
    @memcpy(buf[HEADER_SIZE..], payload);
    return buf;
}

// ----- typed payload helpers -----

/// Writer into a `std.ArrayList(u8)`-shaped byte buffer.
pub const PayloadWriter = struct {
    buf: *std.ArrayList(u8),
    allocator: std.mem.Allocator,

    pub fn writeU8(self: PayloadWriter, v: u8) !void {
        try self.buf.append(self.allocator, v);
    }
    pub fn writeU16(self: PayloadWriter, v: u16) !void {
        var b: [2]u8 = undefined;
        std.mem.writeInt(u16, &b, v, .little);
        try self.buf.appendSlice(self.allocator, &b);
    }
    pub fn writeU32(self: PayloadWriter, v: u32) !void {
        var b: [4]u8 = undefined;
        std.mem.writeInt(u32, &b, v, .little);
        try self.buf.appendSlice(self.allocator, &b);
    }
    /// utf-8 string with u16 length prefix.
    pub fn writeStr(self: PayloadWriter, s: []const u8) !void {
        if (s.len > std.math.maxInt(u16)) return error.StringTooLong;
        try self.writeU16(@intCast(s.len));
        try self.buf.appendSlice(self.allocator, s);
    }
    pub fn writeBytes(self: PayloadWriter, s: []const u8) !void {
        try self.buf.appendSlice(self.allocator, s);
    }
};

pub const PayloadReader = struct {
    buf: []const u8,
    pos: usize = 0,

    pub const Error = error{OutOfRange};

    fn need(self: *PayloadReader, n: usize) Error!void {
        if (self.pos + n > self.buf.len) return error.OutOfRange;
    }
    pub fn remaining(self: *const PayloadReader) usize {
        return self.buf.len - self.pos;
    }
    pub fn readU8(self: *PayloadReader) Error!u8 {
        try self.need(1);
        const v = self.buf[self.pos];
        self.pos += 1;
        return v;
    }
    pub fn readU16(self: *PayloadReader) Error!u16 {
        try self.need(2);
        const v = std.mem.readInt(u16, self.buf[self.pos..][0..2], .little);
        self.pos += 2;
        return v;
    }
    pub fn readU32(self: *PayloadReader) Error!u32 {
        try self.need(4);
        const v = std.mem.readInt(u32, self.buf[self.pos..][0..4], .little);
        self.pos += 4;
        return v;
    }
    pub fn readStr(self: *PayloadReader, max_len: usize) Error![]const u8 {
        const len = try self.readU16();
        if (len > max_len) return error.OutOfRange;
        try self.need(len);
        const s = self.buf[self.pos .. self.pos + len];
        self.pos += len;
        return s;
    }
    pub fn readRest(self: *PayloadReader) []const u8 {
        const s = self.buf[self.pos..];
        self.pos = self.buf.len;
        return s;
    }
};

test "encode/parse header roundtrip" {
    var hdr: [HEADER_SIZE]u8 = undefined;
    encodeHeader(&hdr, 0x0102, 0, 5);
    var buf: [HEADER_SIZE + 5]u8 = undefined;
    @memcpy(buf[0..HEADER_SIZE], &hdr);
    @memcpy(buf[HEADER_SIZE..], "hello");
    const f = try parse(&buf);
    try std.testing.expectEqual(@as(u16, 0x0102), f.opcode);
    try std.testing.expectEqualSlices(u8, "hello", f.payload);
}

test "parse rejects oversize" {
    var hdr: [HEADER_SIZE]u8 = undefined;
    encodeHeader(&hdr, 1, 0, MAX_PAYLOAD + 1);
    try std.testing.expectError(error.PayloadTooLarge, parse(&hdr));
}
