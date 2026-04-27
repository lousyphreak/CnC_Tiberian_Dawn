//! Integration test: spawns the server as a subprocess and drives it
//! over raw WebSockets to exercise the opcode table.
//!
//! Build with: `zig build test-integration` (see build.zig).

const std = @import("std");
const posix = std.posix;

const proto = @import("proto.zig");
const ws = @import("ws.zig");

const WOL_PORT: u16 = 19191;
const HTTP_PORT: u16 = 19192;
const AUTH_PORT: u16 = 19193;

fn findServerExe(allocator: std.mem.Allocator) ![]u8 {
    // Placed at zig-out/bin/td-wol-server by `b.installArtifact`.
    return try allocator.dupe(u8, "zig-out/bin/td-wol-server");
}

const Client = struct {
    sock: posix.socket_t,
    rx: std.ArrayList(u8) = .{},
    allocator: std.mem.Allocator,

    fn connect(allocator: std.mem.Allocator, port: u16) !Client {
        const addr = try std.net.Address.parseIp("127.0.0.1", port);
        const fd = try posix.socket(addr.any.family, posix.SOCK.STREAM | posix.SOCK.CLOEXEC, 0);
        try posix.connect(fd, &addr.any, addr.getOsSockLen());
        return .{ .sock = fd, .allocator = allocator };
    }

    fn deinit(self: *Client) void {
        posix.close(self.sock);
        self.rx.deinit(self.allocator);
    }

    fn writeAll(self: *Client, data: []const u8) !void {
        var off: usize = 0;
        while (off < data.len) {
            const n = try posix.write(self.sock, data[off..]);
            if (n == 0) return error.Closed;
            off += n;
        }
    }

    fn handshake(self: *Client) !void {
        const req =
            "GET /ws HTTP/1.1\r\n" ++
            "Host: 127.0.0.1\r\n" ++
            "Upgrade: websocket\r\n" ++
            "Connection: Upgrade\r\n" ++
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" ++
            "Sec-WebSocket-Version: 13\r\n\r\n";
        try self.writeAll(req);
        // read until \r\n\r\n
        while (true) {
            try self.fillOnce();
            if (std.mem.indexOf(u8, self.rx.items, "\r\n\r\n")) |i| {
                const consume = i + 4;
                const rem = self.rx.items.len - consume;
                if (rem > 0) std.mem.copyForwards(u8, self.rx.items[0..rem], self.rx.items[consume..]);
                self.rx.shrinkRetainingCapacity(rem);
                return;
            }
        }
    }

    fn fillOnce(self: *Client) !void {
        var tmp: [4096]u8 = undefined;
        const n = try posix.read(self.sock, &tmp);
        if (n == 0) return error.Closed;
        try self.rx.appendSlice(self.allocator, tmp[0..n]);
    }

    fn readHttpResponse(self: *Client) !HttpResponse {
        while (std.mem.indexOf(u8, self.rx.items, "\r\n\r\n") == null) {
            try self.fillOnce();
        }

        const header_end = std.mem.indexOf(u8, self.rx.items, "\r\n\r\n").? + 4;
        const header_block = self.rx.items[0..header_end];
        const content_length = parseContentLength(header_block) orelse 0;
        while (self.rx.items.len - header_end < content_length) {
            try self.fillOnce();
        }

        const status = try parseStatusCode(header_block);
        const body_end = header_end + content_length;
        const headers = try self.allocator.dupe(u8, header_block);
        errdefer self.allocator.free(headers);
        const body = try self.allocator.dupe(u8, self.rx.items[header_end..body_end]);
        const rem = self.rx.items.len - body_end;
        if (rem > 0) std.mem.copyForwards(u8, self.rx.items[0..rem], self.rx.items[body_end..]);
        self.rx.shrinkRetainingCapacity(rem);
        return .{
            .status = status,
            .headers = headers,
            .body = body,
        };
    }

    fn sendMasked(self: *Client, opcode: ws.Opcode, payload: []const u8) !void {
        var buf: std.ArrayList(u8) = .{};
        defer buf.deinit(self.allocator);
        var b0: u8 = @intFromEnum(opcode);
        b0 |= 0x80; // FIN
        try buf.append(self.allocator, b0);
        if (payload.len < 126) {
            try buf.append(self.allocator, 0x80 | @as(u8, @intCast(payload.len)));
        } else if (payload.len <= 0xFFFF) {
            try buf.append(self.allocator, 0x80 | 126);
            var len_bytes: [2]u8 = undefined;
            std.mem.writeInt(u16, &len_bytes, @intCast(payload.len), .big);
            try buf.appendSlice(self.allocator, &len_bytes);
        } else {
            try buf.append(self.allocator, 0x80 | 127);
            var len_bytes: [8]u8 = undefined;
            std.mem.writeInt(u64, &len_bytes, payload.len, .big);
            try buf.appendSlice(self.allocator, &len_bytes);
        }
        const mask: [4]u8 = .{ 0x11, 0x22, 0x33, 0x44 };
        try buf.appendSlice(self.allocator, &mask);
        for (payload, 0..) |byte, i| {
            try buf.append(self.allocator, byte ^ mask[i & 3]);
        }
        try self.writeAll(buf.items);
    }

    /// Sends one application frame.
    fn sendApp(self: *Client, opcode: proto.Opcode, payload: []const u8) !void {
        const frame = try proto.buildFrame(self.allocator, opcode, 0, payload);
        defer self.allocator.free(frame);
        try self.sendMasked(.binary, frame);
    }

    /// Receives exactly one app frame. Loops until a complete FIN binary
    /// message is assembled; ignores ping/close.
    fn recvApp(self: *Client) !struct { opcode: u16, payload: []u8 } {
        var msg: std.ArrayList(u8) = .{};
        errdefer msg.deinit(self.allocator);
        while (true) {
            // The server never masks its frames, but our ws.parseFrame
            // requires mask=1. Build a tiny parser here.
            while (true) {
                const f = parseServerFrame(self.rx.items) catch |err| switch (err) {
                    error.Incomplete => {
                        try self.fillOnce();
                        continue;
                    },
                    else => return err,
                };
                // consume bytes
                const consume = f.consumed;
                try msg.appendSlice(self.allocator, f.payload);
                const rem = self.rx.items.len - consume;
                if (rem > 0) std.mem.copyForwards(u8, self.rx.items[0..rem], self.rx.items[consume..]);
                self.rx.shrinkRetainingCapacity(rem);
                if (f.opcode == @intFromEnum(ws.Opcode.ping) or f.opcode == @intFromEnum(ws.Opcode.pong)) {
                    msg.clearRetainingCapacity();
                    continue;
                }
                if (f.fin) {
                    const pf = try proto.parse(msg.items);
                    const dup = try self.allocator.dupe(u8, pf.payload);
                    msg.deinit(self.allocator);
                    return .{ .opcode = pf.opcode, .payload = dup };
                }
                break;
            }
        }
    }
};

const ServerFrame = struct {
    fin: bool,
    opcode: u4,
    payload: []const u8,
    consumed: usize,
};

const HttpResponse = struct {
    status: u16,
    headers: []u8,
    body: []u8,

    fn deinit(self: *HttpResponse, allocator: std.mem.Allocator) void {
        allocator.free(self.headers);
        allocator.free(self.body);
    }
};

fn parseServerFrame(buf: []const u8) !ServerFrame {
    if (buf.len < 2) return error.Incomplete;
    const b0 = buf[0];
    const b1 = buf[1];
    const fin = (b0 & 0x80) != 0;
    const opcode: u4 = @truncate(b0 & 0x0F);
    const masked = (b1 & 0x80) != 0;
    if (masked) return error.UnexpectedMask;
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
    if (buf.len < idx + len7) return error.Incomplete;
    return .{
        .fin = fin,
        .opcode = opcode,
        .payload = buf[idx .. idx + len7],
        .consumed = idx + len7,
    };
}

fn spawnServer(
    allocator: std.mem.Allocator,
    port: u16,
    extra_args: []const []const u8,
    with_basic_auth: bool,
) !std.process.Child {
    const exe = try findServerExe(allocator);
    defer allocator.free(exe);
    const port_text = try std.fmt.allocPrint(allocator, "{d}", .{port});
    defer allocator.free(port_text);

    var argv: std.ArrayList([]const u8) = .{};
    defer argv.deinit(allocator);
    try argv.appendSlice(allocator, &.{ exe, "--port", port_text });
    try argv.appendSlice(allocator, extra_args);

    var child = std.process.Child.init(
        argv.items,
        allocator,
    );
    child.stdin_behavior = .Ignore;
    child.stdout_behavior = .Inherit;
    child.stderr_behavior = .Inherit;
    if (with_basic_auth) {
        var env_map = try std.process.getEnvMap(allocator);
        defer env_map.deinit();
        try env_map.put("TD_BASIC_AUTH_USERNAME", "tiberian");
        try env_map.put("TD_BASIC_AUTH_PASSWORD", "change-me");
        child.env_map = &env_map;
        try child.spawn();
    } else {
        try child.spawn();
    }
    // give it time to bind
    std.Thread.sleep(200 * std.time.ns_per_ms);
    return child;
}

fn helloPayload(allocator: std.mem.Allocator, nick: []const u8) ![]u8 {
    var pl: std.ArrayList(u8) = .{};
    const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = allocator };
    try w.writeU32(proto.PROTOCOL_VERSION);
    try w.writeStr(nick);
    return try pl.toOwnedSlice(allocator);
}

test "end-to-end: two clients, game relay" {
    const allocator = std.testing.allocator;

    var child = try spawnServer(allocator, WOL_PORT, &.{}, false);
    defer {
        _ = child.kill() catch {};
    }

    // Host connects and logs in.
    var host = try Client.connect(allocator, WOL_PORT);
    defer host.deinit();
    try host.handshake();
    const hello_h = try helloPayload(allocator, "HOST");
    defer allocator.free(hello_h);
    try host.sendApp(.hello, hello_h);
    const welc_h = try host.recvApp();
    defer allocator.free(welc_h.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.welcome)), welc_h.opcode);
    var r = proto.PayloadReader{ .buf = welc_h.payload };
    const host_cid = try r.readU32();
    _ = try r.readU32();

    // Guest connects and logs in.
    var guest = try Client.connect(allocator, WOL_PORT);
    defer guest.deinit();
    try guest.handshake();
    const hello_g = try helloPayload(allocator, "GUEST");
    defer allocator.free(hello_g);
    try guest.sendApp(.hello, hello_g);
    const welc_g = try guest.recvApp();
    defer allocator.free(welc_g.payload);
    var rg = proto.PayloadReader{ .buf = welc_g.payload };
    const guest_cid = try rg.readU32();
    _ = try rg.readU32();
    try std.testing.expect(host_cid != guest_cid);

    // Both clients join the shared lobby and can see each other.
    var join_lobby_h: std.ArrayList(u8) = .{};
    defer join_lobby_h.deinit(allocator);
    const lobby_hw: proto.PayloadWriter = .{ .buf = &join_lobby_h, .allocator = allocator };
    try lobby_hw.writeStr("td-lobby");
    try host.sendApp(.channel_join, join_lobby_h.items);
    const host_joined = try host.recvApp();
    defer allocator.free(host_joined.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.channel_joined)), host_joined.opcode);
    const host_empty_games = try host.recvApp();
    defer allocator.free(host_empty_games.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), host_empty_games.opcode);

    var join_lobby_g: std.ArrayList(u8) = .{};
    defer join_lobby_g.deinit(allocator);
    const lobby_gw: proto.PayloadWriter = .{ .buf = &join_lobby_g, .allocator = allocator };
    try lobby_gw.writeStr("td-lobby");
    try guest.sendApp(.channel_join, join_lobby_g.items);
    const guest_joined = try guest.recvApp();
    defer allocator.free(guest_joined.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.channel_joined)), guest_joined.opcode);
    var gjr = proto.PayloadReader{ .buf = guest_joined.payload };
    try std.testing.expectEqualSlices(u8, "td-lobby", try gjr.readStr(32));
    try std.testing.expectEqual(@as(u16, 2), try gjr.readU16());
    try std.testing.expectEqual(host_cid, try gjr.readU32());
    try std.testing.expectEqualSlices(u8, "HOST", try gjr.readStr(32));
    try std.testing.expectEqual(guest_cid, try gjr.readU32());
    try std.testing.expectEqualSlices(u8, "GUEST", try gjr.readStr(32));
    const guest_empty_games = try guest.recvApp();
    defer allocator.free(guest_empty_games.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), guest_empty_games.opcode);
    const host_sees_guest = try host.recvApp();
    defer allocator.free(host_sees_guest.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.channel_user_join)), host_sees_guest.opcode);
    var hsg = proto.PayloadReader{ .buf = host_sees_guest.payload };
    try std.testing.expectEqual(guest_cid, try hsg.readU32());
    try std.testing.expectEqualSlices(u8, "GUEST", try hsg.readStr(32));

    // Lobby chat is relayed through the server to all channel members.
    var chat: std.ArrayList(u8) = .{};
    defer chat.deinit(allocator);
    const chatw: proto.PayloadWriter = .{ .buf = &chat, .allocator = allocator };
    try chatw.writeU32(0);
    try chatw.writeStr("hello lobby");
    try guest.sendApp(.chat, chat.items);
    const host_chat = try host.recvApp();
    defer allocator.free(host_chat.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.chat)), host_chat.opcode);
    var hcr = proto.PayloadReader{ .buf = host_chat.payload };
    try std.testing.expectEqual(guest_cid, try hcr.readU32());
    try std.testing.expectEqual(@as(u32, 0), try hcr.readU32());
    try std.testing.expectEqualSlices(u8, "hello lobby", try hcr.readStr(512));
    const guest_chat = try guest.recvApp();
    defer allocator.free(guest_chat.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.chat)), guest_chat.opcode);

    // Host creates game.
    var pl: std.ArrayList(u8) = .{};
    defer pl.deinit(allocator);
    const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = allocator };
    try w.writeStr("testgame");
    try w.writeU8(4);
    try w.writeU32(0xdeadbeef);
    try host.sendApp(.game_create, pl.items);

    // Host receives game_list_reply (broadcast) and game_members.
    const m1 = try host.recvApp();
    defer allocator.free(m1.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), m1.opcode);

    const l1 = try host.recvApp();
    defer allocator.free(l1.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_members)), l1.opcode);

    // Guest only receives the game_list_reply broadcast.
    const gl = try guest.recvApp();
    defer allocator.free(gl.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), gl.opcode);
    var glr = proto.PayloadReader{ .buf = gl.payload };
    const gcount = try glr.readU16();
    try std.testing.expectEqual(@as(u16, 1), gcount);
    const gid = try glr.readU32();

    // Guest joins.
    var jpl: std.ArrayList(u8) = .{};
    defer jpl.deinit(allocator);
    const jw: proto.PayloadWriter = .{ .buf = &jpl, .allocator = allocator };
    try jw.writeU32(gid);
    try guest.sendApp(.game_join, jpl.items);

    // After guest joins, server broadcasts updated game_list_reply to
    // everyone, then sends game_members (host + guest) to both members.
    const gl2 = try guest.recvApp();
    defer allocator.free(gl2.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), gl2.opcode);
    const gm = try guest.recvApp();
    defer allocator.free(gm.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_members)), gm.opcode);

    const hl = try host.recvApp();
    defer allocator.free(hl.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), hl.opcode);
    const hm = try host.recvApp();
    defer allocator.free(hm.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_members)), hm.opcode);

    // Host starts the game. Members are notified, the game list flips to
    // started, and late join attempts are rejected.
    try host.sendApp(.game_start, &.{});
    const hs = try host.recvApp();
    defer allocator.free(hs.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_started)), hs.opcode);
    var hsr = proto.PayloadReader{ .buf = hs.payload };
    try std.testing.expectEqual(@as(u16, 2), try hsr.readU16());
    try std.testing.expectEqual(host_cid, try hsr.readU32());
    try std.testing.expectEqual(guest_cid, try hsr.readU32());
    const gs = try guest.recvApp();
    defer allocator.free(gs.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_started)), gs.opcode);
    const host_started_list = try host.recvApp();
    defer allocator.free(host_started_list.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), host_started_list.opcode);
    const guest_started_list = try guest.recvApp();
    defer allocator.free(guest_started_list.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), guest_started_list.opcode);
    var gslr = proto.PayloadReader{ .buf = guest_started_list.payload };
    try std.testing.expectEqual(@as(u16, 1), try gslr.readU16());
    try std.testing.expectEqual(gid, try gslr.readU32());
    _ = try gslr.readStr(64);
    _ = try gslr.readU32();
    _ = try gslr.readU8();
    try std.testing.expectEqual(@as(u8, 1), try gslr.readU8());

    var late = try Client.connect(allocator, WOL_PORT);
    defer late.deinit();
    try late.handshake();
    const hello_l = try helloPayload(allocator, "LATE");
    defer allocator.free(hello_l);
    try late.sendApp(.hello, hello_l);
    const welc_l = try late.recvApp();
    defer allocator.free(welc_l.payload);
    var late_join: std.ArrayList(u8) = .{};
    defer late_join.deinit(allocator);
    const latew: proto.PayloadWriter = .{ .buf = &late_join, .allocator = allocator };
    try latew.writeU32(gid);
    try late.sendApp(.game_join, late_join.items);
    const late_err = try late.recvApp();
    defer allocator.free(late_err.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.err)), late_err.opcode);
    var ler = proto.PayloadReader{ .buf = late_err.payload };
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.ErrorCode.game_started)), try ler.readU16());

    // Host relays broadcast → guest receives relay_from with host's cid,
    // proving gameplay traffic goes through the server after start.
    try host.sendApp(.relay_broadcast, "hello guest");
    const rf = try guest.recvApp();
    defer allocator.free(rf.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.relay_from)), rf.opcode);
    var rfr = proto.PayloadReader{ .buf = rf.payload };
    const src = try rfr.readU32();
    try std.testing.expectEqual(host_cid, src);
    try std.testing.expectEqualSlices(u8, "hello guest", rfr.readRest());

    // Guest relays private → host.
    var rp: std.ArrayList(u8) = .{};
    defer rp.deinit(allocator);
    const rpw: proto.PayloadWriter = .{ .buf = &rp, .allocator = allocator };
    try rpw.writeU32(host_cid);
    try rpw.writeBytes("direct");
    try guest.sendApp(.relay_private, rp.items);
    const rf2 = try host.recvApp();
    defer allocator.free(rf2.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.relay_from)), rf2.opcode);
    var rf2r = proto.PayloadReader{ .buf = rf2.payload };
    try std.testing.expectEqual(guest_cid, try rf2r.readU32());
    try std.testing.expectEqualSlices(u8, "direct", rf2r.readRest());

    // Guest leaves; host should be notified and remain with a one-member game.
    try guest.sendApp(.game_leave, &.{});

    const pl_host = try host.recvApp();
    defer allocator.free(pl_host.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.peer_left)), pl_host.opcode);

    const gm_after = try host.recvApp();
    defer allocator.free(gm_after.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_members)), gm_after.opcode);
    var gmr = proto.PayloadReader{ .buf = gm_after.payload };
    try std.testing.expectEqual(gid, try gmr.readU32());
    try std.testing.expectEqual(host_cid, try gmr.readU32());
    try std.testing.expectEqual(@as(u16, 1), try gmr.readU16());
    try std.testing.expectEqual(host_cid, try gmr.readU32());
    _ = try gmr.readStr(64);

    const gl_host_after = try host.recvApp();
    defer allocator.free(gl_host_after.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), gl_host_after.opcode);

    const gl_guest_after = try guest.recvApp();
    defer allocator.free(gl_guest_after.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), gl_guest_after.opcode);
}

test "late lobby join receives current game list" {
    const allocator = std.testing.allocator;

    var child = try spawnServer(allocator, WOL_PORT, &.{}, false);
    defer {
        _ = child.kill() catch {};
    }

    var host = try Client.connect(allocator, WOL_PORT);
    defer host.deinit();
    try host.handshake();
    const hello_h = try helloPayload(allocator, "HOST");
    defer allocator.free(hello_h);
    try host.sendApp(.hello, hello_h);
    const welc_h = try host.recvApp();
    defer allocator.free(welc_h.payload);

    var create: std.ArrayList(u8) = .{};
    defer create.deinit(allocator);
    const cw: proto.PayloadWriter = .{ .buf = &create, .allocator = allocator };
    try cw.writeStr("listed-game");
    try cw.writeU8(4);
    try cw.writeU32(0);
    try host.sendApp(.game_create, create.items);
    const host_list = try host.recvApp();
    defer allocator.free(host_list.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), host_list.opcode);
    const host_members = try host.recvApp();
    defer allocator.free(host_members.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_members)), host_members.opcode);

    var late = try Client.connect(allocator, WOL_PORT);
    defer late.deinit();
    try late.handshake();
    const hello_l = try helloPayload(allocator, "LATE");
    defer allocator.free(hello_l);
    try late.sendApp(.hello, hello_l);
    const welc_l = try late.recvApp();
    defer allocator.free(welc_l.payload);

    var join: std.ArrayList(u8) = .{};
    defer join.deinit(allocator);
    const jw: proto.PayloadWriter = .{ .buf = &join, .allocator = allocator };
    try jw.writeStr("td-lobby");
    try late.sendApp(.channel_join, join.items);

    const joined = try late.recvApp();
    defer allocator.free(joined.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.channel_joined)), joined.opcode);

    const list = try late.recvApp();
    defer allocator.free(list.payload);
    try std.testing.expectEqual(@as(u16, @intFromEnum(proto.Opcode.game_list_reply)), list.opcode);
    var lr = proto.PayloadReader{ .buf = list.payload };
    try std.testing.expectEqual(@as(u16, 1), try lr.readU16());
    _ = try lr.readU32();
    try std.testing.expectEqualSlices(u8, "listed-game", try lr.readStr(64));
}

test "http static hosting supports redirect health and range" {
    const allocator = std.testing.allocator;
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();

    try tmp.dir.makePath("web");
    try tmp.dir.makePath("web/icons");
    try tmp.dir.makePath("GameData");
    try tmp.dir.writeFile(.{ .sub_path = "web/tiberian-dawn.html", .data = "<html>tiberian-dawn</html>\n" });
    try tmp.dir.writeFile(.{ .sub_path = "web/site.webmanifest", .data = "{\"name\":\"Tiberian Dawn\"}\n" });
    try tmp.dir.writeFile(.{ .sub_path = "web/icons/favicon-32.png", .data = "PNGDATA" });
    try tmp.dir.writeFile(.{ .sub_path = "GameData/MAIN1.MIX", .data = "0123456789" });

    const tmp_root = try tmp.dir.realpathAlloc(allocator, ".");
    defer allocator.free(tmp_root);
    const web_root = try std.fmt.allocPrint(allocator, "{s}/web", .{tmp_root});
    defer allocator.free(web_root);
    const gamedata_root = try std.fmt.allocPrint(allocator, "{s}/GameData", .{tmp_root});
    defer allocator.free(gamedata_root);

    var child = try spawnServer(
        allocator,
        HTTP_PORT,
        &.{ "--emscripten-dir", web_root, "--gamedata", gamedata_root },
        false,
    );
    defer {
        _ = child.kill() catch {};
    }

    var root_client = try Client.connect(allocator, HTTP_PORT);
    defer root_client.deinit();
    try root_client.writeAll("GET / HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var root_resp = try root_client.readHttpResponse();
    defer root_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 302), root_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, root_resp.headers, "Location: /tiberian-dawn.html\r\n") != null);

    var health_client = try Client.connect(allocator, HTTP_PORT);
    defer health_client.deinit();
    try health_client.writeAll("GET /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var health_resp = try health_client.readHttpResponse();
    defer health_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 200), health_resp.status);
    try std.testing.expectEqualSlices(u8, "ok\n", health_resp.body);

    var range_client = try Client.connect(allocator, HTTP_PORT);
    defer range_client.deinit();
    try range_client.writeAll(
        "GET /GameData/main1.mix HTTP/1.1\r\n" ++
            "Host: 127.0.0.1\r\n" ++
            "Range: bytes=2-5\r\n" ++
            "Connection: close\r\n\r\n",
    );
    var range_resp = try range_client.readHttpResponse();
    defer range_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 206), range_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, range_resp.headers, "Content-Range: bytes 2-5/10\r\n") != null);
    try std.testing.expectEqualSlices(u8, "2345", range_resp.body);

    var compat_client = try Client.connect(allocator, HTTP_PORT);
    defer compat_client.deinit();
    try compat_client.writeAll(
        "GET /gamedata/main1.mix HTTP/1.1\r\n" ++
            "Host: 127.0.0.1\r\n" ++
            "Range: bytes=0-0\r\n" ++
            "Connection: close\r\n\r\n",
    );
    var compat_resp = try compat_client.readHttpResponse();
    defer compat_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 206), compat_resp.status);
    try std.testing.expectEqualSlices(u8, "0", compat_resp.body);

    var icon_client = try Client.connect(allocator, HTTP_PORT);
    defer icon_client.deinit();
    try icon_client.writeAll("GET /icons/favicon-32.png HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var icon_resp = try icon_client.readHttpResponse();
    defer icon_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 200), icon_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, icon_resp.headers, "Content-Type: image/png\r\n") != null);
    try std.testing.expectEqualSlices(u8, "PNGDATA", icon_resp.body);

    var manifest_client = try Client.connect(allocator, HTTP_PORT);
    defer manifest_client.deinit();
    try manifest_client.writeAll("GET /site.webmanifest HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var manifest_resp = try manifest_client.readHttpResponse();
    defer manifest_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 200), manifest_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, manifest_resp.headers, "Content-Type: application/manifest+json; charset=utf-8\r\n") != null);
    try std.testing.expect(std.mem.indexOf(u8, manifest_resp.body, "\"Tiberian Dawn\"") != null);
}

test "http basic auth protects static files but allows websocket upgrade" {
    const allocator = std.testing.allocator;
    var tmp = std.testing.tmpDir(.{});
    defer tmp.cleanup();

    try tmp.dir.makePath("web");
    try tmp.dir.writeFile(.{ .sub_path = "web/tiberian-dawn.html", .data = "<html>tiberian-dawn</html>\n" });

    const tmp_root = try tmp.dir.realpathAlloc(allocator, ".");
    defer allocator.free(tmp_root);
    const web_root = try std.fmt.allocPrint(allocator, "{s}/web", .{tmp_root});
    defer allocator.free(web_root);

    var child = try spawnServer(
        allocator,
        AUTH_PORT,
        &.{ "--emscripten-dir", web_root },
        true,
    );
    defer {
        _ = child.kill() catch {};
    }

    var unauth_client = try Client.connect(allocator, AUTH_PORT);
    defer unauth_client.deinit();
    try unauth_client.writeAll("GET /tiberian-dawn.html HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var unauth_resp = try unauth_client.readHttpResponse();
    defer unauth_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 401), unauth_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, unauth_resp.headers, "WWW-Authenticate: Basic realm=\"Tiberian Dawn\", charset=\"UTF-8\"\r\n") != null);

    var health_client = try Client.connect(allocator, AUTH_PORT);
    defer health_client.deinit();
    try health_client.writeAll("GET /healthz HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n");
    var health_resp = try health_client.readHttpResponse();
    defer health_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 200), health_resp.status);

    var auth_client = try Client.connect(allocator, AUTH_PORT);
    defer auth_client.deinit();
    const auth_header = try basicAuthHeaderValue(allocator, "tiberian", "change-me");
    defer allocator.free(auth_header);
    const auth_request = try std.fmt.allocPrint(
        allocator,
        "GET /tiberian-dawn.html HTTP/1.1\r\nHost: 127.0.0.1\r\nAuthorization: {s}\r\nConnection: close\r\n\r\n",
        .{auth_header},
    );
    defer allocator.free(auth_request);
    try auth_client.writeAll(auth_request);
    var auth_resp = try auth_client.readHttpResponse();
    defer auth_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 200), auth_resp.status);
    try std.testing.expect(std.mem.indexOf(u8, auth_resp.body, "tiberian-dawn") != null);

    var ws_client = try Client.connect(allocator, AUTH_PORT);
    defer ws_client.deinit();
    const ws_request = try std.fmt.allocPrint(
        allocator,
        "GET /ws HTTP/1.1\r\n" ++
            "Host: 127.0.0.1\r\n" ++
            "Upgrade: websocket\r\n" ++
            "Connection: Upgrade\r\n" ++
            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" ++
            "Sec-WebSocket-Version: 13\r\n\r\n",
        .{},
    );
    defer allocator.free(ws_request);
    try ws_client.writeAll(ws_request);
    var ws_resp = try ws_client.readHttpResponse();
    defer ws_resp.deinit(allocator);
    try std.testing.expectEqual(@as(u16, 101), ws_resp.status);
}

fn parseStatusCode(headers: []const u8) !u16 {
    const line_end = std.mem.indexOf(u8, headers, "\r\n") orelse return error.BadResponse;
    var parts = std.mem.splitScalar(u8, headers[0..line_end], ' ');
    _ = parts.next() orelse return error.BadResponse;
    const code = parts.next() orelse return error.BadResponse;
    return std.fmt.parseInt(u16, code, 10);
}

fn parseContentLength(headers: []const u8) ?usize {
    var it = std.mem.splitSequence(u8, headers, "\r\n");
    _ = it.next();
    while (it.next()) |line| {
        if (line.len == 0) break;
        const colon = std.mem.indexOfScalar(u8, line, ':') orelse continue;
        const name = line[0..colon];
        const value = std.mem.trim(u8, line[colon + 1 ..], " \t");
        if (std.ascii.eqlIgnoreCase(name, "Content-Length")) {
            return std.fmt.parseInt(usize, value, 10) catch null;
        }
    }
    return null;
}

fn basicAuthHeaderValue(allocator: std.mem.Allocator, username: []const u8, password: []const u8) ![]u8 {
    const plain = try std.fmt.allocPrint(allocator, "{s}:{s}", .{ username, password });
    defer allocator.free(plain);
    const encoded_len = std.base64.standard.Encoder.calcSize(plain.len);
    const out = try allocator.alloc(u8, "Basic ".len + encoded_len);
    std.mem.copyForwards(u8, out[0.."Basic ".len], "Basic ");
    _ = std.base64.standard.Encoder.encode(out["Basic ".len ..], plain);
    return out;
}
