#!/bin/sh

set -eu

script_dir="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
build_dir="${script_dir}/build-em"
cache_file="${build_dir}/CMakeCache.txt"

if ! command -v emcmake >/dev/null 2>&1; then
    echo "emcmake was not found in PATH." >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "python3 was not found in PATH." >&2
    exit 1
fi

if [ -f "${cache_file}" ]; then
    if ! grep -q '^EMSCRIPTEN:INTERNAL=1$' "${cache_file}" || \
       ! grep -q '^CMAKE_BUILD_TYPE:STRING=Debug$' "${cache_file}"; then
        cmake -E rm -rf "${build_dir}"
    fi
fi


emcmake cmake -S "${script_dir}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DTD_EMSCRIPTEN_PACKAGE_GAMEDATA=OFF \
    -DTD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA=ON

# cmake --build "${build_dir}" --target clean
cmake --build "${build_dir}" -j20
exec python3 "${script_dir}/TOOLS/emscripten_range_server.py" \
    --root "${script_dir}" \
    --port 8088 \
    --open "/build-em/tiberian-dawn.html"
