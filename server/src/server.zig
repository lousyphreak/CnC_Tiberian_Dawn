//! Non-blocking single-threaded server event loop tying everything
//! together: HTTP static serving, WebSocket upgrade, WOL hub routing.

const std = @import("std");
const posix = std.posix;

const proto = @import("proto.zig");
const ws = @import("ws.zig");
const http = @import("http.zig");

const RX_LIMIT: usize = 1 * 1024 * 1024; // 1 MiB per-connection rx cap
const TX_SOFT_LIMIT: usize = 8 * 1024 * 1024; // 8 MiB per-connection tx cap

const log = std.log.scoped(.server);

pub const BasicAuthConfig = struct {
    username: []const u8,
    password: []const u8,
};

pub const Config = struct {
    host: []const u8 = "0.0.0.0",
    port: u16 = 8070,
    gamedata_dir: ?[]const u8 = null,
    emscripten_dir: ?[]const u8 = null,
    basic_auth: ?BasicAuthConfig = null,
};

pub const Server = struct {
    allocator: std.mem.Allocator,
    cfg: Config,
    basic_auth_header: ?[]u8 = null,
    listener_fd: posix.socket_t,
    /// All active connections, indexed by slot. `null` = free slot.
    conns: std.ArrayList(?*Conn) = .{},
    /// Next client id (monotonic).
    next_client_id: u32 = 1,
    next_game_id: u32 = 1,
    /// Channels by lowercased name.
    channels: std.StringHashMapUnmanaged(*Channel) = .empty,
    games: std.AutoHashMapUnmanaged(u32, *Game) = .empty,
    running: bool = true,

    pub fn init(allocator: std.mem.Allocator, cfg: Config) !Server {
        const fd = try openListener(cfg.host, cfg.port);
        var self = Server{
            .allocator = allocator,
            .cfg = cfg,
            .listener_fd = fd,
        };
        errdefer posix.close(fd);
        if (cfg.basic_auth) |auth| {
            self.basic_auth_header = try buildBasicAuthHeader(allocator, auth.username, auth.password);
        }
        return self;
    }

    pub fn deinit(self: *Server) void {
        if (self.basic_auth_header) |header| self.allocator.free(header);
        for (self.conns.items) |maybe_c| {
            if (maybe_c) |c| {
                c.deinit(self.allocator);
                self.allocator.destroy(c);
            }
        }
        self.conns.deinit(self.allocator);

        var ch_it = self.channels.iterator();
        while (ch_it.next()) |e| {
            self.allocator.free(e.key_ptr.*);
            e.value_ptr.*.deinit(self.allocator);
            self.allocator.destroy(e.value_ptr.*);
        }
        self.channels.deinit(self.allocator);

        var g_it = self.games.iterator();
        while (g_it.next()) |e| {
            e.value_ptr.*.deinit(self.allocator);
            self.allocator.destroy(e.value_ptr.*);
        }
        self.games.deinit(self.allocator);

        posix.close(self.listener_fd);
    }

    pub fn run(self: *Server) !void {
        var pfds: std.ArrayList(posix.pollfd) = .{};
        defer pfds.deinit(self.allocator);

        while (self.running) {
            pfds.clearRetainingCapacity();
            try pfds.append(self.allocator, .{
                .fd = self.listener_fd,
                .events = posix.POLL.IN,
                .revents = 0,
            });
            for (self.conns.items) |maybe_c| {
                if (maybe_c) |c| {
                    var events: i16 = posix.POLL.IN;
                    if (c.tx.items.len > 0) events |= posix.POLL.OUT;
                    try pfds.append(self.allocator, .{
                        .fd = c.fd,
                        .events = events,
                        .revents = 0,
                    });
                }
            }

            const nready = posix.poll(pfds.items, 1000) catch |err| return err;
            if (nready == 0) continue;

            // Listener
            if (pfds.items[0].revents & posix.POLL.IN != 0) {
                self.acceptAll() catch |e| log.warn("accept: {t}", .{e});
            }

            // Connections
            var pi: usize = 1;
            var ci: usize = 0;
            while (ci < self.conns.items.len) : (ci += 1) {
                const maybe_c = self.conns.items[ci];
                if (maybe_c == null) continue;
                const c = maybe_c.?;
                if (pi >= pfds.items.len) break;
                defer pi += 1;
                const pfd = pfds.items[pi];
                if (pfd.revents == 0) continue;

                const closed = (pfd.revents & (posix.POLL.HUP | posix.POLL.ERR | posix.POLL.NVAL)) != 0;
                if (pfd.revents & posix.POLL.IN != 0) {
                    self.handleReadable(c) catch {
                        c.wants_close = true;
                    };
                }
                if (!c.wants_close and (pfd.revents & posix.POLL.OUT) != 0) {
                    self.flushConn(c) catch {
                        c.wants_close = true;
                    };
                }
                if (closed) {
                    c.peer_closed = true;
                    c.wants_close = true;
                }
                discardPendingTxIfClosing(c);
                if (c.wants_close and c.tx.items.len == 0) {
                    self.removeConn(ci);
                }
            }
        }
    }

    fn acceptAll(self: *Server) !void {
        while (true) {
            var addr: posix.sockaddr.storage = undefined;
            var alen: posix.socklen_t = @sizeOf(@TypeOf(addr));
            const fd = posix.accept(
                self.listener_fd,
                @ptrCast(&addr),
                &alen,
                posix.SOCK.NONBLOCK | posix.SOCK.CLOEXEC,
            ) catch |err| switch (err) {
                error.WouldBlock => return,
                else => return err,
            };
            try self.addConn(fd);
        }
    }

    fn addConn(self: *Server, fd: posix.socket_t) !void {
        const c = try self.allocator.create(Conn);
        errdefer self.allocator.destroy(c);
        c.* = .{ .fd = fd };
        for (self.conns.items, 0..) |slot, i| {
            if (slot == null) {
                self.conns.items[i] = c;
                return;
            }
        }
        try self.conns.append(self.allocator, c);
    }

    fn removeConn(self: *Server, index: usize) void {
        const c = self.conns.items[index] orelse return;
        self.handleClientGone(c);
        posix.close(c.fd);
        c.deinit(self.allocator);
        self.allocator.destroy(c);
        self.conns.items[index] = null;
    }

    fn handleReadable(self: *Server, c: *Conn) !void {
        var tmp: [16 * 1024]u8 = undefined;
        while (true) {
            const n = posix.read(c.fd, &tmp) catch |err| switch (err) {
                error.WouldBlock => return,
                else => {
                    c.peer_closed = true;
                    c.wants_close = true;
                    return;
                },
            };
            if (n == 0) {
                c.peer_closed = true;
                c.wants_close = true;
                return;
            }
            if (c.rx.items.len + n > RX_LIMIT) {
                c.wants_close = true;
                return;
            }
            try c.rx.appendSlice(self.allocator, tmp[0..n]);
            try self.processConn(c);
        }
    }

    fn processConn(self: *Server, c: *Conn) !void {
        while (true) {
            switch (c.state) {
                .http_wait => {
                    const req = http.parseRequest(c.rx.items) catch |err| switch (err) {
                        error.Incomplete => return,
                        else => {
                            try self.writeHttpSimple(c, 400, "Bad Request", "Bad Request\n");
                            c.wants_close_after_flush = true;
                            return;
                        },
                    };
                    // consume bytes
                    const advance = req.header_end;
                    try self.handleRequest(c, req);
                    // shift remaining rx
                    const remaining = c.rx.items.len - advance;
                    if (remaining > 0) {
                        std.mem.copyForwards(u8, c.rx.items[0..remaining], c.rx.items[advance..]);
                    }
                    c.rx.shrinkRetainingCapacity(remaining);
                },
                .ws => {
                    const f = ws.parseFrame(c.rx.items) catch |err| switch (err) {
                        error.Incomplete => return,
                        else => {
                            c.wants_close = true;
                            return;
                        },
                    };
                    try self.handleWsFrame(c, f);
                    const remaining = c.rx.items.len - f.consumed;
                    if (remaining > 0) {
                        std.mem.copyForwards(u8, c.rx.items[0..remaining], c.rx.items[f.consumed..]);
                    }
                    c.rx.shrinkRetainingCapacity(remaining);
                },
                .closing => return,
            }
            if (c.rx.items.len == 0) return;
        }
    }

    fn handleRequest(self: *Server, c: *Conn, req: http.Request) !void {
        if (std.mem.eql(u8, req.path, "/healthz")) {
            try self.writeHttpBody(c, 200, "OK", "text/plain; charset=utf-8", "ok\n", "no-store", req.keep_alive, null);
            c.wants_close_after_flush = !req.keep_alive;
            return;
        }
        if (req.method == .other) {
            try self.writeHttpSimple(c, 405, "Method Not Allowed", "Method Not Allowed\n");
            c.wants_close_after_flush = true;
            return;
        }
        if (!self.authorizeRequest(req)) {
            try self.writeHttpUnauthorized(c, req.keep_alive);
            c.wants_close_after_flush = !req.keep_alive;
            return;
        }
        if (std.mem.eql(u8, req.path, "/")) {
            try self.writeHttpRedirect(c, "/tiberian-dawn.html", req.keep_alive);
            c.wants_close_after_flush = !req.keep_alive;
            return;
        }
        if (std.mem.eql(u8, req.path, "/ws")) {
            if (!req.upgrade_websocket or req.ws_key == null) {
                try self.writeHttpSimple(c, 400, "Bad Request", "expected websocket upgrade\n");
                c.wants_close_after_flush = true;
                return;
            }
            var accept: [28]u8 = undefined;
            ws.handshakeAccept(&accept, req.ws_key.?);
            var hdr: std.ArrayList(u8) = .{};
            defer hdr.deinit(self.allocator);
            try hdr.writer(self.allocator).print(
                "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {s}\r\n\r\n",
                .{accept},
            );
            try c.tx.appendSlice(self.allocator, hdr.items);
            c.state = .ws;
            c.client_id = self.next_client_id;
            self.next_client_id += 1;
            log.info("ws upgrade ok, cid={d}", .{c.client_id});
            return;
        }

        // static file serving
        try self.serveStatic(c, req);
    }

    fn serveStatic(self: *Server, c: *Conn, req: http.Request) !void {
        // Map /gamedata/... → gamedata_dir; everything else → emscripten_dir.
        var root: ?[]const u8 = null;
        var sub_path = req.path;
        var cache_control: []const u8 = "no-cache, no-store, must-revalidate";
        if (http.asciiStartsWithIgnoreCase(req.path, "/gamedata/")) {
            root = self.cfg.gamedata_dir;
            sub_path = req.path["/gamedata".len..];
            cache_control = "no-cache, no-store, must-revalidate";
        } else {
            root = self.cfg.emscripten_dir;
        }
        if (root == null) {
            try self.writeHttpSimple(c, 404, "Not Found", "no root configured\n");
            c.wants_close_after_flush = true;
            return;
        }

        const resolved = http.resolveUnder(self.allocator, root.?, sub_path) catch null;
        if (resolved == null) {
            try self.writeHttpSimple(c, 404, "Not Found", "not found\n");
            c.wants_close_after_flush = true;
            return;
        }
        defer self.allocator.free(resolved.?);

        const file = std.fs.cwd().openFile(resolved.?, .{}) catch {
            try self.writeHttpSimple(c, 404, "Not Found", "not found\n");
            c.wants_close_after_flush = true;
            return;
        };
        defer file.close();
        const stat = file.stat() catch {
            try self.writeHttpSimple(c, 500, "Internal Server Error", "stat failed\n");
            c.wants_close_after_flush = true;
            return;
        };
        const total_size: u64 = @intCast(stat.size);
        const maybe_range = if (req.range) |raw_range|
            http.parseSingleRangeHeader(raw_range, total_size) catch |err| switch (err) {
                error.InvalidRange, error.UnsatisfiableRange => {
                    try self.writeHttpRangeNotSatisfiable(c, total_size, req.keep_alive);
                    c.wants_close_after_flush = !req.keep_alive;
                    return;
                },
            }
        else
            null;

        const body_len: usize = @intCast(if (maybe_range) |range| range.len() else total_size);
        const body = try self.allocator.alloc(u8, body_len);
        defer self.allocator.free(body);
        if (body.len > 0) {
            const body_start: u64 = if (maybe_range) |range| range.start else 0;
            const nread = try file.preadAll(body, body_start);
            if (nread != body.len) return error.EndOfStream;
        }

        const content_type = http.mimeFor(resolved.?);
        try self.writeStaticHeaders(c, req.keep_alive, content_type, cache_control, total_size, maybe_range);
        if (req.method == .get) try c.tx.appendSlice(self.allocator, body);
        if (!req.keep_alive) c.wants_close_after_flush = true;
    }

    fn authorizeRequest(self: *Server, req: http.Request) bool {
        const expected = self.basic_auth_header orelse return true;
        return req.authorization != null and std.mem.eql(u8, req.authorization.?, expected);
    }

    fn writeStaticHeaders(
        self: *Server,
        c: *Conn,
        keep_alive: bool,
        content_type: []const u8,
        cache_control: []const u8,
        total_size: u64,
        maybe_range: ?http.ByteRange,
    ) !void {
        var hdr: std.ArrayList(u8) = .{};
        defer hdr.deinit(self.allocator);

        if (maybe_range) |range| {
            try hdr.writer(self.allocator).print(
                "HTTP/1.1 206 Partial Content\r\nContent-Type: {s}\r\nContent-Length: {d}\r\nContent-Range: bytes {d}-{d}/{d}\r\nAccept-Ranges: bytes\r\nCache-Control: {s}\r\nConnection: {s}\r\n\r\n",
                .{
                    content_type,
                    range.len(),
                    range.start,
                    range.end_inclusive,
                    total_size,
                    cache_control,
                    if (keep_alive) "keep-alive" else "close",
                },
            );
        } else {
            try hdr.writer(self.allocator).print(
                "HTTP/1.1 200 OK\r\nContent-Type: {s}\r\nContent-Length: {d}\r\nAccept-Ranges: bytes\r\nCache-Control: {s}\r\nConnection: {s}\r\n\r\n",
                .{
                    content_type,
                    total_size,
                    cache_control,
                    if (keep_alive) "keep-alive" else "close",
                },
            );
        }
        try c.tx.appendSlice(self.allocator, hdr.items);
    }

    fn writeHttpSimple(self: *Server, c: *Conn, code: u16, reason: []const u8, body: []const u8) !void {
        try self.writeHttpBody(c, code, reason, "text/plain; charset=utf-8", body, "no-store", false, null);
    }

    fn writeHttpBody(
        self: *Server,
        c: *Conn,
        code: u16,
        reason: []const u8,
        content_type: []const u8,
        body: []const u8,
        cache_control: []const u8,
        keep_alive: bool,
        extra_headers: ?[]const u8,
    ) !void {
        var hdr: std.ArrayList(u8) = .{};
        defer hdr.deinit(self.allocator);
        try hdr.writer(self.allocator).print(
            "HTTP/1.1 {d} {s}\r\nContent-Type: {s}\r\nContent-Length: {d}\r\nCache-Control: {s}\r\nConnection: {s}\r\n",
            .{
                code,
                reason,
                content_type,
                body.len,
                cache_control,
                if (keep_alive) "keep-alive" else "close",
            },
        );
        if (extra_headers) |headers| try hdr.appendSlice(self.allocator, headers);
        try hdr.appendSlice(self.allocator, "\r\n");
        try c.tx.appendSlice(self.allocator, hdr.items);
        try c.tx.appendSlice(self.allocator, body);
    }

    fn writeHttpRedirect(self: *Server, c: *Conn, location: []const u8, keep_alive: bool) !void {
        var extra: std.ArrayList(u8) = .{};
        defer extra.deinit(self.allocator);
        try extra.writer(self.allocator).print("Location: {s}\r\n", .{location});
        try self.writeHttpBody(c, 302, "Found", "text/plain; charset=utf-8", "", "no-store", keep_alive, extra.items);
    }

    fn writeHttpUnauthorized(self: *Server, c: *Conn, keep_alive: bool) !void {
        try self.writeHttpBody(
            c,
            401,
            "Unauthorized",
            "text/plain; charset=utf-8",
            "Authentication Required\n",
            "no-store",
            keep_alive,
            "WWW-Authenticate: Basic realm=\"Tiberian Dawn\", charset=\"UTF-8\"\r\n",
        );
    }

    fn writeHttpRangeNotSatisfiable(self: *Server, c: *Conn, total_size: u64, keep_alive: bool) !void {
        var extra: std.ArrayList(u8) = .{};
        defer extra.deinit(self.allocator);
        try extra.writer(self.allocator).print("Content-Range: bytes */{d}\r\nAccept-Ranges: bytes\r\n", .{total_size});
        try self.writeHttpBody(
            c,
            416,
            "Range Not Satisfiable",
            "text/plain; charset=utf-8",
            "Range Not Satisfiable\n",
            "no-store",
            keep_alive,
            extra.items,
        );
    }

    fn flushConn(self: *Server, c: *Conn) !void {
        while (c.tx.items.len > 0) {
            const n = posix.write(c.fd, c.tx.items) catch |err| switch (err) {
                error.WouldBlock => return,
                else => {
                    c.peer_closed = true;
                    c.wants_close = true;
                    return;
                },
            };
            if (n == 0) {
                c.wants_close = true;
                return;
            }
            const rem = c.tx.items.len - n;
            if (rem > 0) std.mem.copyForwards(u8, c.tx.items[0..rem], c.tx.items[n..]);
            c.tx.shrinkRetainingCapacity(rem);
        }
        if (c.wants_close_after_flush) c.wants_close = true;
        _ = self;
    }

    fn discardPendingTxIfClosing(c: *Conn) void {
        if (c.tx.items.len == 0) return;
        if (c.peer_closed or (c.wants_close and !c.wants_close_after_flush)) {
            c.tx.clearRetainingCapacity();
        }
    }

    // ---- WebSocket / WOL hub ----

    fn handleWsFrame(self: *Server, c: *Conn, f: ws.Frame) !void {
        switch (f.opcode) {
            .close => {
                // echo close
                try self.sendWs(c, .close, &.{});
                c.wants_close_after_flush = true;
            },
            .ping => try self.sendWs(c, .pong, f.payload),
            .pong => {},
            .text => c.wants_close = true,
            .binary, .cont => {
                // Reassemble.
                if (f.opcode != .cont) c.ws_msg.clearRetainingCapacity();
                try c.ws_msg.appendSlice(self.allocator, f.payload);
                if (f.fin) {
                    try self.handleAppMessage(c, c.ws_msg.items);
                    c.ws_msg.clearRetainingCapacity();
                }
            },
            else => c.wants_close = true,
        }
    }

    fn handleAppMessage(self: *Server, c: *Conn, msg: []const u8) !void {
        const frame = proto.parse(msg) catch {
            try self.sendError(c, .bad_protocol, "bad frame");
            return;
        };
        const op: proto.Opcode = @enumFromInt(frame.opcode);
        var pr: proto.PayloadReader = .{ .buf = frame.payload };
        switch (op) {
            .hello => try self.handleHello(c, &pr),
            .channel_list => try self.sendChannelList(c),
            .channel_join => try self.handleChannelJoin(c, &pr),
            .chat => try self.handleChat(c, &pr),
            .game_create => try self.handleGameCreate(c, &pr),
            .game_join => try self.handleGameJoin(c, &pr),
            .game_leave => try self.handleGameLeave(c),
            .game_start => try self.handleGameStart(c),
            .relay_broadcast => try self.handleRelayBroadcast(c, frame.payload),
            .relay_private => try self.handleRelayPrivate(c, &pr),
            else => try self.sendError(c, .bad_protocol, "unknown opcode"),
        }
    }

    fn handleHello(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        const ver = pr.readU32() catch {
            try self.sendError(c, .bad_protocol, "bad hello");
            return;
        };
        const nick = pr.readStr(32) catch {
            try self.sendError(c, .bad_nickname, "bad nickname");
            return;
        };
        if (ver != proto.PROTOCOL_VERSION) {
            try self.sendError(c, .bad_protocol, "protocol version mismatch");
            return;
        }
        if (nick.len == 0) {
            try self.sendError(c, .bad_nickname, "empty nickname");
            return;
        }
        if (c.nick) |old| self.allocator.free(old);
        c.nick = try self.allocator.dupe(u8, nick);
        c.logged_in = true;

        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        try w.writeU32(proto.PROTOCOL_VERSION);
        try self.sendFrame(c, .welcome, 0, pl.items);
        log.info("cid={d} logged in as '{s}'", .{ c.client_id, c.nick.? });
    }

    fn requireLogin(self: *Server, c: *Conn) !bool {
        if (!c.logged_in) {
            try self.sendError(c, .not_logged_in, "hello required");
            return false;
        }
        return true;
    }

    fn sendChannelList(self: *Server, c: *Conn) !void {
        if (!try self.requireLogin(c)) return;
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        const count: u16 = @intCast(self.channels.count());
        try w.writeU16(count);
        var it = self.channels.iterator();
        while (it.next()) |e| {
            try w.writeStr(e.key_ptr.*);
            try w.writeU16(@intCast(e.value_ptr.*.members.items.len));
        }
        try self.sendFrame(c, .channel_list_reply, 0, pl.items);
    }

    fn handleChannelJoin(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        if (!try self.requireLogin(c)) return;
        const name = pr.readStr(32) catch {
            try self.sendError(c, .bad_protocol, "bad channel name");
            return;
        };
        if (name.len == 0) {
            try self.sendError(c, .bad_protocol, "empty channel name");
            return;
        }
        try self.leaveChannel(c);

        const lower = try self.allocator.alloc(u8, name.len);
        for (name, 0..) |b, i| lower[i] = std.ascii.toLower(b);
        const gop = try self.channels.getOrPut(self.allocator, lower);
        if (!gop.found_existing) {
            const ch = try self.allocator.create(Channel);
            ch.* = .{
                .name = try self.allocator.dupe(u8, name),
            };
            gop.value_ptr.* = ch;
        } else {
            self.allocator.free(lower);
        }
        const ch = gop.value_ptr.*;
        try ch.members.append(self.allocator, c.client_id);
        c.channel_key = gop.key_ptr.*;

        // Send CHANNEL_JOINED to the new member with current member list.
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeStr(ch.name);
        try w.writeU16(@intCast(ch.members.items.len));
        for (ch.members.items) |cid| {
            const other = self.findConnByCid(cid);
            try w.writeU32(cid);
            const nn: []const u8 = if (other != null and other.?.nick != null) other.?.nick.? else "";
            try w.writeStr(nn);
        }
        try self.sendFrame(c, .channel_joined, 0, pl.items);
        try self.sendGameList(c);

        // Notify others.
        var np: std.ArrayList(u8) = .{};
        defer np.deinit(self.allocator);
        const nw: proto.PayloadWriter = .{ .buf = &np, .allocator = self.allocator };
        try nw.writeU32(c.client_id);
        try nw.writeStr(c.nick.?);
        for (ch.members.items) |cid| {
            if (cid == c.client_id) continue;
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .channel_user_join, 0, np.items);
        }
    }

    fn leaveChannel(self: *Server, c: *Conn) !void {
        const key = c.channel_key orelse return;
        const ch = self.channels.get(key) orelse {
            c.channel_key = null;
            return;
        };
        var i: usize = 0;
        while (i < ch.members.items.len) : (i += 1) {
            if (ch.members.items[i] == c.client_id) {
                _ = ch.members.swapRemove(i);
                break;
            }
        }
        // Notify remaining.
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        for (ch.members.items) |cid| {
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .channel_user_leave, 0, pl.items);
        }
        c.channel_key = null;

        if (ch.members.items.len == 0) {
            _ = self.channels.remove(key);
            self.allocator.free(key);
            ch.deinit(self.allocator);
            self.allocator.destroy(ch);
        }
    }

    fn handleChat(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        if (!try self.requireLogin(c)) return;
        const target = pr.readU32() catch return;
        const text = pr.readStr(512) catch return;

        // Build CHAT payload (source_cid, target, text) and relay.
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        try w.writeU32(target);
        try w.writeStr(text);

        if (target != 0) {
            if (self.findConnByCid(target)) |oc| {
                try self.sendFrame(oc, .chat, 0, pl.items);
            }
            return;
        }
        // channel broadcast
        const key = c.channel_key orelse {
            try self.sendError(c, .not_in_channel, "chat without channel");
            return;
        };
        const ch = self.channels.get(key) orelse return;
        for (ch.members.items) |cid| {
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .chat, 0, pl.items);
        }
    }

    fn handleGameCreate(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        if (!try self.requireLogin(c)) return;
        if (c.game_id != 0) {
            try self.sendError(c, .already_in_game, "already in a game");
            return;
        }
        const name = pr.readStr(64) catch return;
        const max_players = pr.readU8() catch return;
        const scen_digest: u32 = pr.readU32() catch 0;

        const g = try self.allocator.create(Game);
        g.* = .{
            .id = self.next_game_id,
            .name = try self.allocator.dupe(u8, name),
            .host_id = c.client_id,
            .max_players = max_players,
            .scen_digest = scen_digest,
        };
        self.next_game_id += 1;
        try g.members.append(self.allocator, c.client_id);
        try self.games.put(self.allocator, g.id, g);
        c.game_id = g.id;

        try self.broadcastGameList();
        try self.sendGameMembers(g);
    }

    fn handleGameJoin(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        if (!try self.requireLogin(c)) return;
        if (c.game_id != 0) {
            try self.sendError(c, .already_in_game, "already in a game");
            return;
        }
        const gid = pr.readU32() catch return;
        const g = self.games.get(gid) orelse {
            try self.sendError(c, .unknown_target, "no such game");
            return;
        };
        if (g.started) {
            try self.sendError(c, .game_started, "game already started");
            return;
        }
        if (g.max_players != 0 and g.members.items.len >= g.max_players) {
            try self.sendError(c, .game_full, "game full");
            return;
        }
        try g.members.append(self.allocator, c.client_id);
        c.game_id = g.id;
        try self.broadcastGameList();
        try self.sendGameMembers(g);
    }

    fn handleGameLeave(self: *Server, c: *Conn) !void {
        if (c.game_id == 0) return;
        try self.removeFromGame(c);
        try self.broadcastGameList();
    }

    fn handleGameStart(self: *Server, c: *Conn) !void {
        if (!try self.requireLogin(c)) return;
        const g = self.games.get(c.game_id) orelse {
            try self.sendError(c, .not_in_game, "no game");
            return;
        };
        if (g.host_id != c.client_id) {
            try self.sendError(c, .not_in_game, "not host");
            return;
        }
        g.started = true;

        // Send GAME_STARTED with seating order (host first).
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU16(@intCast(g.members.items.len));
        for (g.members.items) |cid| try w.writeU32(cid);
        for (g.members.items) |cid| {
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .game_started, 0, pl.items);
        }
        try self.broadcastGameList();
    }

    fn handleRelayBroadcast(self: *Server, c: *Conn, payload: []const u8) !void {
        if (!try self.requireLogin(c)) return;
        // Build RELAY_FROM once: u32 source_cid | caller payload
        var out: std.ArrayList(u8) = .{};
        defer out.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &out, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        try w.writeBytes(payload);

        // Broadcast to the union of game members and channel members so both
        // gameplay multicast AND lobby discovery work. A host that has created
        // a game is still in the lobby channel and must be able to reply to
        // NET_QUERY_GAME from clients that haven't joined the game yet. Once
        // the match is started, relay gameplay only to actual game members.
        var seen: std.AutoHashMap(u32, void) = .init(self.allocator);
        defer seen.deinit();
        var delivered = false;
        var allow_channel_broadcast = true;
        if (self.games.get(c.game_id)) |g| {
            allow_channel_broadcast = !g.started;
            for (g.members.items) |cid| {
                if (cid == c.client_id) continue;
                if ((try seen.getOrPut(cid)).found_existing) continue;
                if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .relay_from, 0, out.items);
                delivered = true;
            }
        }
        if (allow_channel_broadcast) if (c.channel_key) |key| if (self.channels.get(key)) |ch| {
            for (ch.members.items) |cid| {
                if (cid == c.client_id) continue;
                if ((try seen.getOrPut(cid)).found_existing) continue;
                if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .relay_from, 0, out.items);
                delivered = true;
            }
        };
        if (!delivered and c.channel_key == null and c.game_id == 0) {
            try self.sendError(c, .not_in_game, "relay without game or channel");
        }
    }

    fn handleRelayPrivate(self: *Server, c: *Conn, pr: *proto.PayloadReader) !void {
        if (!try self.requireLogin(c)) return;
        const target = pr.readU32() catch return;
        const rest = pr.readRest();

        // Target must share a game or a channel with us.
        var in_scope = false;
        if (self.games.get(c.game_id)) |g| {
            for (g.members.items) |cid| if (cid == target) {
                in_scope = true;
                break;
            };
        }
        if (!in_scope) if (c.channel_key) |key| if (self.channels.get(key)) |ch| {
            for (ch.members.items) |cid| if (cid == target) {
                in_scope = true;
                break;
            };
        };
        if (!in_scope) {
            try self.sendError(c, .unknown_target, "target not in game or channel");
            return;
        }
        const oc = self.findConnByCid(target) orelse return;

        var out: std.ArrayList(u8) = .{};
        defer out.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &out, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        try w.writeBytes(rest);
        try self.sendFrame(oc, .relay_from, 0, out.items);
    }

    fn handleClientGone(self: *Server, c: *Conn) void {
        // Leave channel + game, notifying peers.
        self.leaveChannel(c) catch {};
        if (c.game_id != 0) {
            self.removeFromGame(c) catch {};
            self.broadcastGameList() catch {};
        }
    }

    fn removeFromGame(self: *Server, c: *Conn) !void {
        const gid = c.game_id;
        if (gid == 0) return;
        c.game_id = 0;
        const g = self.games.get(gid) orelse return;

        var i: usize = 0;
        while (i < g.members.items.len) : (i += 1) {
            if (g.members.items[i] == c.client_id) {
                _ = g.members.swapRemove(i);
                break;
            }
        }

        // notify remaining members with PEER_LEFT
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU32(c.client_id);
        for (g.members.items) |cid| {
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .peer_left, 0, pl.items);
        }

        if (g.members.items.len == 0) {
            _ = self.games.remove(gid);
            g.deinit(self.allocator);
            self.allocator.destroy(g);
            return;
        } else if (g.host_id == c.client_id and g.members.items.len > 0) {
            g.host_id = g.members.items[0];
        }
        try self.sendGameMembers(g);
    }

    fn sendGameMembers(self: *Server, g: *Game) !void {
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU32(g.id);
        try w.writeU32(g.host_id);
        try w.writeU16(@intCast(g.members.items.len));
        for (g.members.items) |cid| {
            const other = self.findConnByCid(cid);
            try w.writeU32(cid);
            const nn: []const u8 = if (other != null and other.?.nick != null) other.?.nick.? else "";
            try w.writeStr(nn);
        }
        for (g.members.items) |cid| {
            if (self.findConnByCid(cid)) |oc| try self.sendFrame(oc, .game_members, 0, pl.items);
        }
    }

    fn broadcastGameList(self: *Server) !void {
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        try self.buildGameListPayload(&pl);
        // Send to every logged-in client.
        for (self.conns.items) |maybe_c| {
            if (maybe_c) |cc| {
                if (cc.state == .ws and cc.logged_in) {
                    try self.sendFrame(cc, .game_list_reply, 0, pl.items);
                }
            }
        }
    }

    fn sendGameList(self: *Server, c: *Conn) !void {
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        try self.buildGameListPayload(&pl);
        try self.sendFrame(c, .game_list_reply, 0, pl.items);
    }

    fn buildGameListPayload(self: *Server, pl: *std.ArrayList(u8)) !void {
        const w: proto.PayloadWriter = .{ .buf = pl, .allocator = self.allocator };
        const count: u16 = @intCast(self.games.count());
        try w.writeU16(count);
        var it = self.games.iterator();
        while (it.next()) |e| {
            const g = e.value_ptr.*;
            try w.writeU32(g.id);
            try w.writeStr(g.name);
            try w.writeU32(g.host_id);
            try w.writeU8(g.max_players);
            try w.writeU8(if (g.started) 1 else 0);
            try w.writeU16(@intCast(g.members.items.len));
        }
    }

    fn findConnByCid(self: *Server, cid: u32) ?*Conn {
        for (self.conns.items) |maybe_c| {
            if (maybe_c) |cc| if (cc.client_id == cid) return cc;
        }
        return null;
    }

    fn sendError(self: *Server, c: *Conn, code: proto.ErrorCode, msg: []const u8) !void {
        var pl: std.ArrayList(u8) = .{};
        defer pl.deinit(self.allocator);
        const w: proto.PayloadWriter = .{ .buf = &pl, .allocator = self.allocator };
        try w.writeU16(@intFromEnum(code));
        try w.writeStr(msg);
        try self.sendFrame(c, .err, 0, pl.items);
    }

    fn sendFrame(self: *Server, c: *Conn, opcode: proto.Opcode, flags: u16, payload: []const u8) !void {
        const app = try proto.buildFrame(self.allocator, opcode, flags, payload);
        defer self.allocator.free(app);
        try self.sendWs(c, .binary, app);
    }

    fn sendWs(self: *Server, c: *Conn, opcode: ws.Opcode, payload: []const u8) !void {
        if (c.tx.items.len > TX_SOFT_LIMIT) {
            c.wants_close = true;
            return;
        }
        try ws.writeFrame(&c.tx, self.allocator, opcode, true, payload);
    }
};

pub const Channel = struct {
    name: []u8,
    members: std.ArrayList(u32) = .{},

    pub fn deinit(self: *Channel, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        self.members.deinit(allocator);
    }
};

pub const Game = struct {
    id: u32,
    name: []u8,
    host_id: u32,
    max_players: u8,
    scen_digest: u32,
    started: bool = false,
    members: std.ArrayList(u32) = .{},

    pub fn deinit(self: *Game, allocator: std.mem.Allocator) void {
        allocator.free(self.name);
        self.members.deinit(allocator);
    }
};

pub const ConnState = enum { http_wait, ws, closing };

pub const Conn = struct {
    fd: posix.socket_t,
    state: ConnState = .http_wait,
    rx: std.ArrayList(u8) = .{},
    tx: std.ArrayList(u8) = .{},
    ws_msg: std.ArrayList(u8) = .{},
    wants_close: bool = false,
    wants_close_after_flush: bool = false,
    peer_closed: bool = false,
    // WOL state
    client_id: u32 = 0,
    nick: ?[]u8 = null,
    logged_in: bool = false,
    channel_key: ?[]const u8 = null,
    game_id: u32 = 0,

    pub fn deinit(self: *Conn, allocator: std.mem.Allocator) void {
        self.rx.deinit(allocator);
        self.tx.deinit(allocator);
        self.ws_msg.deinit(allocator);
        if (self.nick) |n| allocator.free(n);
    }
};

fn openListener(host: []const u8, port: u16) !posix.socket_t {
    const addr = try std.net.Address.parseIp(host, port);
    const fd = try posix.socket(
        addr.any.family,
        posix.SOCK.STREAM | posix.SOCK.NONBLOCK | posix.SOCK.CLOEXEC,
        0,
    );
    errdefer posix.close(fd);
    try posix.setsockopt(fd, posix.SOL.SOCKET, posix.SO.REUSEADDR, &std.mem.toBytes(@as(c_int, 1)));
    try posix.bind(fd, &addr.any, addr.getOsSockLen());
    try posix.listen(fd, 128);
    log.info("listening on {s}:{d}", .{ host, port });
    return fd;
}

fn buildBasicAuthHeader(allocator: std.mem.Allocator, username: []const u8, password: []const u8) ![]u8 {
    const plain = try std.fmt.allocPrint(allocator, "{s}:{s}", .{ username, password });
    defer allocator.free(plain);

    const encoded_len = std.base64.standard.Encoder.calcSize(plain.len);
    const header = try allocator.alloc(u8, "Basic ".len + encoded_len);
    std.mem.copyForwards(u8, header[0.."Basic ".len], "Basic ");
    _ = std.base64.standard.Encoder.encode(header["Basic ".len..], plain);
    return header;
}

test "discardPendingTxIfClosing drops queued bytes for fatal close" {
    var conn = Conn{ .fd = 0 };
    defer conn.deinit(std.testing.allocator);

    try conn.tx.appendSlice(std.testing.allocator, "pending");
    conn.wants_close = true;
    Server.discardPendingTxIfClosing(&conn);
    try std.testing.expectEqual(@as(usize, 0), conn.tx.items.len);
}

test "discardPendingTxIfClosing keeps graceful close payloads" {
    var conn = Conn{ .fd = 0 };
    defer conn.deinit(std.testing.allocator);

    try conn.tx.appendSlice(std.testing.allocator, "pending");
    conn.wants_close_after_flush = true;
    Server.discardPendingTxIfClosing(&conn);
    try std.testing.expectEqualStrings("pending", conn.tx.items);
}

test "discardPendingTxIfClosing drops queued bytes after peer hangup" {
    var conn = Conn{ .fd = 0 };
    defer conn.deinit(std.testing.allocator);

    try conn.tx.appendSlice(std.testing.allocator, "pending");
    conn.peer_closed = true;
    Server.discardPendingTxIfClosing(&conn);
    try std.testing.expectEqual(@as(usize, 0), conn.tx.items.len);
}

test "discardPendingTxIfClosing drops graceful-close payloads after peer eof" {
    var conn = Conn{ .fd = 0 };
    defer conn.deinit(std.testing.allocator);

    try conn.tx.appendSlice(std.testing.allocator, "pending");
    conn.wants_close = true;
    conn.wants_close_after_flush = true;
    conn.peer_closed = true;
    Server.discardPendingTxIfClosing(&conn);
    try std.testing.expectEqual(@as(usize, 0), conn.tx.items.len);
}
