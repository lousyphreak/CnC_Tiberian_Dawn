#!/usr/bin/env python3

import argparse
import email.utils
import mimetypes
import os
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlsplit
import webbrowser


class RangeRequestHandler(SimpleHTTPRequestHandler):
    server_version = "TDRangeHTTP/1.0"

    def translate_path(self, path):
        path = urlsplit(path).path
        path = unquote(path)
        parts = [part for part in path.split("/") if part and part not in (".", "..")]
        result = Path(self.server.root)
        for part in parts:
            result /= part
        return str(result)

    def end_headers(self):
        self.send_header("Accept-Ranges", "bytes")
        super().end_headers()

    def send_head(self):
        path = self.translate_path(self.path)
        if os.path.isdir(path):
            return super().send_head()
        if not os.path.exists(path):
            self.send_error(HTTPStatus.NOT_FOUND, "File not found")
            return None

        file_size = os.path.getsize(path)
        start = 0
        end = file_size - 1
        status = HTTPStatus.OK
        range_header = self.headers.get("Range")

        if range_header:
            if not range_header.startswith("bytes="):
                self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
                return None
            range_spec = range_header[6:].split(",", 1)[0].strip()
            first, separator, last = range_spec.partition("-")
            try:
                if first:
                    start = int(first)
                    end = int(last) if separator and last else file_size - 1
                elif separator and last:
                    suffix_length = int(last)
                    if suffix_length <= 0:
                        raise ValueError
                    start = max(file_size - suffix_length, 0)
                    end = file_size - 1
                else:
                    raise ValueError
            except ValueError:
                self.send_error(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
                return None

            if start < 0 or end < start or start >= file_size:
                self.send_response(HTTPStatus.REQUESTED_RANGE_NOT_SATISFIABLE)
                self.send_header("Content-Range", f"bytes */{file_size}")
                self.send_header("Content-Length", "0")
                self.end_headers()
                return None
            end = min(end, file_size - 1)
            status = HTTPStatus.PARTIAL_CONTENT

        content_length = end - start + 1 if file_size else 0
        encoded = os.fsencode(path)
        file = open(encoded, "rb")
        file.seek(start)
        self.range_remaining = content_length

        self.send_response(status)
        self.send_header("Content-Type", self.guess_type(path))
        self.send_header("Content-Length", str(content_length))
        self.send_header("Last-Modified", self.date_time_string(os.path.getmtime(path)))
        if status == HTTPStatus.PARTIAL_CONTENT:
            self.send_header("Content-Range", f"bytes {start}-{end}/{file_size}")
        self.end_headers()
        return file

    def copyfile(self, source, outputfile):
        remaining = getattr(self, "range_remaining", None)
        if remaining is None:
            return super().copyfile(source, outputfile)
        while remaining > 0:
            chunk = source.read(min(64 * 1024, remaining))
            if not chunk:
                break
            outputfile.write(chunk)
            remaining -= len(chunk)

    def guess_type(self, path):
        guessed = mimetypes.guess_type(path)[0]
        if guessed:
            return guessed
        if path.lower().endswith(".wasm"):
            return "application/wasm"
        return "application/octet-stream"


def main():
    parser = argparse.ArgumentParser(description="Serve the Emscripten build with HTTP Range support.")
    parser.add_argument("--root", default=".", help="Directory to serve.")
    parser.add_argument("--host", default="127.0.0.1", help="Host interface to bind.")
    parser.add_argument("--port", type=int, default=8088, help="Port to listen on.")
    parser.add_argument("--open", default="", help="Path to open in the default browser.")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    ThreadingHTTPServer.allow_reuse_address = True
    server = ThreadingHTTPServer((args.host, args.port), RangeRequestHandler)
    server.root = str(root)
    url = f"http://{args.host}:{args.port}{args.open}"
    print(f"Serving {root} at http://{args.host}:{args.port}/")
    if args.open:
        print(f"Opening {url}")
        webbrowser.open(url)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
