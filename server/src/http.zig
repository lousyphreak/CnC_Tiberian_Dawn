//! Minimal HTTP/1.1 request parsing and static file serving for the WOL
//! server. Enough to:
//!   - parse GET requests with headers
//!   - identify a WebSocket upgrade request and extract Sec-WebSocket-Key
//!   - serve static files out of the emscripten output directory and
//!     the GameData directory (case-insensitive path resolution)
//!
//! We keep everything buffered in-process (no streaming) because the
//! files the game needs are small (a few MB total per session).

const std = @import("std");

pub const Method = enum { get, head, other };

pub const Request = struct {
    method: Method,
    target: []const u8, // path+query, slice into rx buffer
    path: []const u8, // just the decoded path (points into target or into owned buf)
    host: ?[]const u8 = null,
    authorization: ?[]const u8 = null,
    upgrade_websocket: bool = false,
    ws_key: ?[]const u8 = null,
    ws_version: ?[]const u8 = null,
    range: ?[]const u8 = null,
    keep_alive: bool = true,
    header_end: usize, // consumed bytes from rx buffer
};

pub const ParseError = error{
    Incomplete,
    BadRequest,
    TooLarge,
};

pub const RangeError = error{
    InvalidRange,
    UnsatisfiableRange,
};

pub const ByteRange = struct {
    start: u64,
    end_inclusive: u64,

    pub fn len(self: ByteRange) u64 {
        return (self.end_inclusive - self.start) + 1;
    }
};

pub fn parseRequest(buf: []const u8) ParseError!Request {
    const end_marker = "\r\n\r\n";
    const maybe_end = std.mem.indexOf(u8, buf, end_marker) orelse return error.Incomplete;
    const headers_end = maybe_end + end_marker.len;

    var line_it = std.mem.splitSequence(u8, buf[0..maybe_end], "\r\n");
    const request_line = line_it.next() orelse return error.BadRequest;

    var parts = std.mem.splitScalar(u8, request_line, ' ');
    const method_s = parts.next() orelse return error.BadRequest;
    const target = parts.next() orelse return error.BadRequest;
    _ = parts.next() orelse return error.BadRequest; // version

    const method: Method = if (std.mem.eql(u8, method_s, "GET"))
        .get
    else if (std.mem.eql(u8, method_s, "HEAD"))
        .head
    else
        .other;

    var req: Request = .{
        .method = method,
        .target = target,
        .path = pathOnly(target),
        .header_end = headers_end,
    };

    while (line_it.next()) |line| {
        const colon = std.mem.indexOfScalar(u8, line, ':') orelse continue;
        const name = line[0..colon];
        var value = line[colon + 1 ..];
        value = std.mem.trim(u8, value, " \t");
        if (asciiEqlIgnoreCase(name, "Host")) {
            req.host = value;
        } else if (asciiEqlIgnoreCase(name, "Authorization")) {
            req.authorization = value;
        } else if (asciiEqlIgnoreCase(name, "Upgrade")) {
            if (asciiEqlIgnoreCase(value, "websocket")) req.upgrade_websocket = true;
        } else if (asciiEqlIgnoreCase(name, "Sec-WebSocket-Key")) {
            req.ws_key = value;
        } else if (asciiEqlIgnoreCase(name, "Sec-WebSocket-Version")) {
            req.ws_version = value;
        } else if (asciiEqlIgnoreCase(name, "Range")) {
            req.range = value;
        } else if (asciiEqlIgnoreCase(name, "Connection")) {
            if (asciiContainsIgnoreCase(value, "close")) req.keep_alive = false;
        }
    }
    return req;
}

fn pathOnly(target: []const u8) []const u8 {
    if (std.mem.indexOfScalar(u8, target, '?')) |q| return target[0..q];
    return target;
}

pub fn asciiEqlIgnoreCase(a: []const u8, b: []const u8) bool {
    if (a.len != b.len) return false;
    for (a, b) |x, y| {
        if (std.ascii.toLower(x) != std.ascii.toLower(y)) return false;
    }
    return true;
}

pub fn asciiStartsWithIgnoreCase(haystack: []const u8, needle: []const u8) bool {
    if (haystack.len < needle.len) return false;
    return asciiEqlIgnoreCase(haystack[0..needle.len], needle);
}

fn asciiContainsIgnoreCase(haystack: []const u8, needle: []const u8) bool {
    if (needle.len == 0) return true;
    if (haystack.len < needle.len) return false;
    var i: usize = 0;
    while (i + needle.len <= haystack.len) : (i += 1) {
        if (asciiEqlIgnoreCase(haystack[i .. i + needle.len], needle)) return true;
    }
    return false;
}

pub fn parseSingleRangeHeader(value: []const u8, full_size: u64) RangeError!ByteRange {
    if (full_size == 0) return error.UnsatisfiableRange;
    if (!asciiStartsWithIgnoreCase(value, "bytes=")) return error.InvalidRange;

    const spec = value["bytes=".len..];
    if (spec.len == 0 or std.mem.indexOfScalar(u8, spec, ',') != null) return error.InvalidRange;

    const dash = std.mem.indexOfScalar(u8, spec, '-') orelse return error.InvalidRange;
    const start_text = spec[0..dash];
    const end_text = spec[dash + 1 ..];

    if (start_text.len == 0) {
        const suffix_len = std.fmt.parseInt(u64, end_text, 10) catch return error.InvalidRange;
        if (suffix_len == 0) return error.InvalidRange;
        const actual_len = @min(suffix_len, full_size);
        return .{
            .start = full_size - actual_len,
            .end_inclusive = full_size - 1,
        };
    }

    const start = std.fmt.parseInt(u64, start_text, 10) catch return error.InvalidRange;
    if (start >= full_size) return error.UnsatisfiableRange;

    var end = full_size - 1;
    if (end_text.len != 0) {
        end = std.fmt.parseInt(u64, end_text, 10) catch return error.InvalidRange;
        if (end < start) return error.InvalidRange;
        if (end >= full_size) end = full_size - 1;
    }

    return .{
        .start = start,
        .end_inclusive = end,
    };
}

pub fn mimeFor(path: []const u8) []const u8 {
    const ext_dot = std.mem.lastIndexOfScalar(u8, path, '.') orelse return "application/octet-stream";
    const ext = path[ext_dot + 1 ..];
    const table: []const struct { []const u8, []const u8 } = &.{
        .{ "html", "text/html; charset=utf-8" },
        .{ "htm", "text/html; charset=utf-8" },
        .{ "js", "application/javascript; charset=utf-8" },
        .{ "mjs", "application/javascript; charset=utf-8" },
        .{ "wasm", "application/wasm" },
        .{ "css", "text/css; charset=utf-8" },
        .{ "json", "application/json; charset=utf-8" },
        .{ "webmanifest", "application/manifest+json; charset=utf-8" },
        .{ "txt", "text/plain; charset=utf-8" },
        .{ "ico", "image/x-icon" },
        .{ "png", "image/png" },
        .{ "jpg", "image/jpeg" },
        .{ "jpeg", "image/jpeg" },
        .{ "svg", "image/svg+xml" },
        .{ "data", "application/octet-stream" },
        .{ "map", "application/json; charset=utf-8" },
    };
    for (table) |row| {
        if (asciiEqlIgnoreCase(ext, row[0])) return row[1];
    }
    return "application/octet-stream";
}

/// Normalizes and resolves a URL path onto a filesystem path relative to
/// `root_dir`. Case-insensitive component-by-component lookup, so the
/// game's case-insensitive asset naming just works on Linux too.
///
/// Returns an allocated absolute path to the resolved file, or null if
/// not found. Refuses paths that try to escape the root via "..".
pub fn resolveUnder(
    allocator: std.mem.Allocator,
    root_dir: []const u8,
    url_path: []const u8,
) !?[]u8 {
    // strip leading slashes
    var rel = url_path;
    while (rel.len > 0 and rel[0] == '/') rel = rel[1..];

    // Open root dir.
    var root = std.fs.cwd().openDir(root_dir, .{ .iterate = true }) catch return null;
    defer root.close();

    // Bare root → index.html
    if (rel.len == 0) rel = "index.html";

    var cur_dir = root;
    var owned = false;
    defer if (owned) cur_dir.close();

    var it = std.mem.splitScalar(u8, rel, '/');
    var resolved_components: std.ArrayList([]u8) = .{};
    defer {
        for (resolved_components.items) |c| allocator.free(c);
        resolved_components.deinit(allocator);
    }

    var component = it.next();
    while (component) |c| {
        const next = it.next();
        const is_last = (next == null);
        if (c.len == 0 or std.mem.eql(u8, c, ".")) {
            component = next;
            continue;
        }
        if (std.mem.eql(u8, c, "..")) return null;

        // Find matching entry (case-insensitive) in cur_dir.
        var found_name: ?[]u8 = null;
        var is_dir = false;
        var dir_it = cur_dir.iterate();
        while (try dir_it.next()) |entry| {
            if (asciiEqlIgnoreCase(entry.name, c)) {
                found_name = try allocator.dupe(u8, entry.name);
                is_dir = entry.kind == .directory;
                break;
            }
        }
        const match = found_name orelse return null;
        errdefer allocator.free(match);

        try resolved_components.append(allocator, match);

        if (!is_last) {
            if (!is_dir) return null;
            const sub = cur_dir.openDir(match, .{ .iterate = true }) catch return null;
            if (owned) cur_dir.close();
            cur_dir = sub;
            owned = true;
        } else {
            if (is_dir) {
                // try index.html
                const idx_name = try allocator.dupe(u8, "index.html");
                try resolved_components.append(allocator, idx_name);
            }
        }

        component = next;
    }

    // Build full path string: root_dir + "/" + join components.
    var buf: std.ArrayList(u8) = .{};
    errdefer buf.deinit(allocator);
    try buf.appendSlice(allocator, root_dir);
    for (resolved_components.items) |c| {
        try buf.append(allocator, '/');
        try buf.appendSlice(allocator, c);
    }
    return try buf.toOwnedSlice(allocator);
}

test "parse GET with WS upgrade" {
    const raw = "GET /ws HTTP/1.1\r\nHost: x\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
    const r = try parseRequest(raw);
    try std.testing.expect(r.method == .get);
    try std.testing.expect(r.upgrade_websocket);
    try std.testing.expectEqualSlices(u8, "dGhlIHNhbXBsZSBub25jZQ==", r.ws_key.?);
    try std.testing.expectEqualSlices(u8, "/ws", r.path);
}

test "parse single byte range" {
    const range = try parseSingleRangeHeader("bytes=4-9", 32);
    try std.testing.expectEqual(@as(u64, 4), range.start);
    try std.testing.expectEqual(@as(u64, 9), range.end_inclusive);
    try std.testing.expectEqual(@as(u64, 6), range.len());
}

test "parse open ended byte range clamps to file size" {
    const range = try parseSingleRangeHeader("bytes=5-999", 12);
    try std.testing.expectEqual(@as(u64, 5), range.start);
    try std.testing.expectEqual(@as(u64, 11), range.end_inclusive);
}

test "parse suffix byte range" {
    const range = try parseSingleRangeHeader("bytes=-4", 12);
    try std.testing.expectEqual(@as(u64, 8), range.start);
    try std.testing.expectEqual(@as(u64, 11), range.end_inclusive);
}
