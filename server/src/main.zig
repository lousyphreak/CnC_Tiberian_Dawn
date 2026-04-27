const std = @import("std");
const server_mod = @import("server.zig");
const Server = server_mod.Server;
const BasicAuthConfig = server_mod.BasicAuthConfig;

const Args = struct {
    host: []const u8 = "0.0.0.0",
    port: u16 = 8070,
    gamedata: ?[]const u8 = null,
    emscripten: ?[]const u8 = null,
    basic_auth: ?BasicAuthConfig = null,
};

fn getOptionalEnvVarOwned(allocator: std.mem.Allocator, name: []const u8) !?[]const u8 {
    return std.process.getEnvVarOwned(allocator, name) catch |err| switch (err) {
        error.EnvironmentVariableNotFound => null,
        else => return err,
    };
}

fn parseArgs(allocator: std.mem.Allocator) !Args {
    var args = Args{};
    const td_basic_auth_htpasswd = try getOptionalEnvVarOwned(allocator, "TD_BASIC_AUTH_HTPASSWD");
    const ra_basic_auth_htpasswd = try getOptionalEnvVarOwned(allocator, "RA_BASIC_AUTH_HTPASSWD");
    if (td_basic_auth_htpasswd != null or ra_basic_auth_htpasswd != null) {
        std.debug.print("HTPASSWD is no longer supported by the Zig server; use TD_BASIC_AUTH_USERNAME and TD_BASIC_AUTH_PASSWORD instead.\n", .{});
        std.process.exit(2);
    }
    const td_basic_auth_username = try getOptionalEnvVarOwned(allocator, "TD_BASIC_AUTH_USERNAME");
    const td_basic_auth_password = try getOptionalEnvVarOwned(allocator, "TD_BASIC_AUTH_PASSWORD");
    const ra_basic_auth_username = try getOptionalEnvVarOwned(allocator, "RA_BASIC_AUTH_USERNAME");
    const ra_basic_auth_password = try getOptionalEnvVarOwned(allocator, "RA_BASIC_AUTH_PASSWORD");
    if (td_basic_auth_username != null or td_basic_auth_password != null) {
        if (td_basic_auth_username == null or td_basic_auth_password == null) {
            std.debug.print("TD_BASIC_AUTH_USERNAME and TD_BASIC_AUTH_PASSWORD must be set together.\n", .{});
            std.process.exit(2);
        }
        args.basic_auth = .{
            .username = td_basic_auth_username.?,
            .password = td_basic_auth_password.?,
        };
    } else if (ra_basic_auth_username != null or ra_basic_auth_password != null) {
        if (ra_basic_auth_username == null or ra_basic_auth_password == null) {
            std.debug.print("RA_BASIC_AUTH_USERNAME and RA_BASIC_AUTH_PASSWORD must be set together.\n", .{});
            std.process.exit(2);
        }
        args.basic_auth = .{
            .username = ra_basic_auth_username.?,
            .password = ra_basic_auth_password.?,
        };
    }

    var it = try std.process.argsWithAllocator(allocator);
    defer it.deinit();
    _ = it.next(); // argv[0]
    while (it.next()) |raw| {
        if (std.mem.eql(u8, raw, "--host")) {
            args.host = try allocator.dupe(u8, it.next() orelse return error.MissingValue);
        } else if (std.mem.eql(u8, raw, "--port")) {
            const v = it.next() orelse return error.MissingValue;
            args.port = try std.fmt.parseInt(u16, v, 10);
        } else if (std.mem.eql(u8, raw, "--gamedata")) {
            args.gamedata = try allocator.dupe(u8, it.next() orelse return error.MissingValue);
        } else if (std.mem.eql(u8, raw, "--emscripten-dir") or std.mem.eql(u8, raw, "--webroot")) {
            args.emscripten = try allocator.dupe(u8, it.next() orelse return error.MissingValue);
        } else if (std.mem.eql(u8, raw, "--help") or std.mem.eql(u8, raw, "-h")) {
            try std.fs.File.stdout().writeAll(
                \\td-wol-server - Tiberian Dawn WOL replacement server
                \\
                \\Usage:
                \\  td-wol-server [--host HOST] [--port PORT]
                \\                [--gamedata PATH] [--emscripten-dir PATH]
                \\
                \\Endpoints:
                \\  http://HOST:PORT/             static files from --emscripten-dir
                \\  http://HOST:PORT/gamedata/... static files from --gamedata
                \\  ws://HOST:PORT/ws             WOL control plane + relay
                \\
            );
            std.process.exit(0);
        } else {
            std.debug.print("unknown arg: {s}\n", .{raw});
            std.process.exit(2);
        }
    }
    return args;
}

pub fn main() !void {
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};
    defer _ = gpa.deinit();
    const allocator = gpa.allocator();

    var arg_arena = std.heap.ArenaAllocator.init(allocator);
    defer arg_arena.deinit();
    const args = try parseArgs(arg_arena.allocator());

    // Ignore SIGPIPE so writes to a closed socket return EPIPE instead of
    // killing the process.
    const posix = std.posix;
    var sa = posix.Sigaction{
        .handler = .{ .handler = posix.SIG.IGN },
        .mask = posix.sigemptyset(),
        .flags = 0,
    };
    posix.sigaction(posix.SIG.PIPE, &sa, null);

    var srv = try Server.init(allocator, .{
        .host = args.host,
        .port = args.port,
        .gamedata_dir = args.gamedata,
        .emscripten_dir = args.emscripten,
        .basic_auth = args.basic_auth,
    });
    defer srv.deinit();
    try srv.run();
}
