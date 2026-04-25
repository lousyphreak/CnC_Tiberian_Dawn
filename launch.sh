cmake -S . -B build-asan -DTD_ENABLE_ASAN=1 -DTD_ENABLE_UBSAN=1 -DCMAKE_BUILD_TYPE=Debug TD_ENABLE_WOL=1 && \
cmake --build build-asan -j32 && \
TD_TRACE_STARTUP=1 TD_TRACE_NETWORK=1 gdb -batch -ex run --args ./build-asan/tiberian-dawn -gamedata "$(pwd)/GameData" -wol-server ws://127.0.0.1:8070/ws
