//! RFC 6455 WebSocket framing helpers (server side).
//!
//! We only need:
//!   - parse a client frame from a byte buffer (returning consumed bytes)
//!   - build a server frame into an output buffer (no masking)
//!   - reassemble fragmented client messages into a single payload

const std = @import("std");

pub const Opcode = enum(u4) {
    cont = 0x0,
    text = 0x1,
    binary = 0x2,
    close = 0x8,
    ping = 0x9,
    pong = 0xA,
    _,
};

pub const MAX_MESSAGE: usize = 4 * 1024 * 1024;

pub const Frame = struct {
    fin: bool,
    opcode: Opcode,
    payload: []u8, // mutable because we unmask in place
    /// Total bytes consumed from the input buffer (header + payload).
    consumed: usize,
};

pub const ParseError = error{
    Incomplete,
    MissingMask,
    MessageTooLarge,
};

/// Parses a single frame. Returns `error.Incomplete` if not enough data.
/// `buf` must be mutable because client frames are masked and we
/// unmask in-place.
pub fn parseFrame(buf: []u8) ParseError!Frame {
    if (buf.len < 2) return error.Incomplete;
    const b0 = buf[0];
    const b1 = buf[1];
    const fin = (b0 & 0x80) != 0;
    const opcode: Opcode = @enumFromInt(@as(u4, @truncate(b0 & 0x0F)));
    const masked = (b1 & 0x80) != 0;
    if (!masked) return error.MissingMask;
    var len7: u64 = b1 & 0x7F;
    var idx: usize = 2;
    if (len7 == 126) {
        if (buf.len < idx + 2) return error.Incomplete;
        len7 = std.mem.readInt(u16, buf[idx..][0..2], .big);
        idx += 2;
    } else if (len7 == 127) {
        if (buf.len < idx + 8) return error.Incomplete;
        len7 = std.mem.readInt(u64, buf[idx..][0..8], .big);
        idx += 8;
    }
    if (len7 > MAX_MESSAGE) return error.MessageTooLarge;
    if (buf.len < idx + 4) return error.Incomplete;
    const mask = buf[idx..][0..4].*;
    idx += 4;
    const payload_len: usize = @intCast(len7);
    if (buf.len < idx + payload_len) return error.Incomplete;
    const payload = buf[idx .. idx + payload_len];
    for (payload, 0..) |*byte, i| {
        byte.* ^= mask[i & 0x3];
    }
    return .{
        .fin = fin,
        .opcode = opcode,
        .payload = payload,
        .consumed = idx + payload_len,
    };
}

/// Writes a server frame (unmasked) into `out`. Returns bytes written.
pub fn writeFrame(
    out: *std.ArrayList(u8),
    allocator: std.mem.Allocator,
    opcode: Opcode,
    fin: bool,
    payload: []const u8,
) !void {
    var b0: u8 = @intFromEnum(opcode);
    if (fin) b0 |= 0x80;
    try out.append(allocator, b0);
    if (payload.len < 126) {
        try out.append(allocator, @intCast(payload.len));
    } else if (payload.len <= 0xFFFF) {
        try out.append(allocator, 126);
        var b: [2]u8 = undefined;
        std.mem.writeInt(u16, &b, @intCast(payload.len), .big);
        try out.appendSlice(allocator, &b);
    } else {
        try out.append(allocator, 127);
        var b: [8]u8 = undefined;
        std.mem.writeInt(u64, &b, payload.len, .big);
        try out.appendSlice(allocator, &b);
    }
    try out.appendSlice(allocator, payload);
}

/// Computes the Sec-WebSocket-Accept response header value.
pub fn handshakeAccept(out: *[28]u8, client_key: []const u8) void {
    const guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    var hasher = std.crypto.hash.Sha1.init(.{});
    hasher.update(client_key);
    hasher.update(guid);
    var digest: [20]u8 = undefined;
    hasher.final(&digest);
    const enc = std.base64.standard.Encoder;
    _ = enc.encode(out, &digest);
}

test "handshake accept matches rfc6455 example" {
    var out: [28]u8 = undefined;
    handshakeAccept(&out, "dGhlIHNhbXBsZSBub25jZQ==");
    try std.testing.expectEqualSlices(u8, "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", &out);
}

test "parse simple masked binary frame" {
    // fin=1 opcode=2 (binary), masked, len=5, mask=0x01020304, payload="hello"
    var frame: [11]u8 = .{ 0x82, 0x85, 0x01, 0x02, 0x03, 0x04, 0, 0, 0, 0, 0 };
    const payload = "hello";
    for (payload, 0..) |b, i| frame[6 + i] = b ^ frame[2 + (i & 3)];
    const f = try parseFrame(&frame);
    try std.testing.expect(f.fin);
    try std.testing.expectEqual(Opcode.binary, f.opcode);
    try std.testing.expectEqualSlices(u8, "hello", f.payload);
    try std.testing.expectEqual(@as(usize, 11), f.consumed);
}
