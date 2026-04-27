# syntax=docker/dockerfile:1.7

ARG EMSCRIPTEN_IMAGE=emscripten/emsdk:4.0.14
ARG DEBIAN_IMAGE=debian:bookworm-slim
ARG ZIG_VERSION=0.15.2

FROM ${EMSCRIPTEN_IMAGE} AS emscripten-build

WORKDIR /src

RUN python3 -m pip install --no-cache-dir "cmake>=3.24,<4" ninja

COPY . .

ARG TD_EMSCRIPTEN_DIAGNOSTIC=ON

RUN emcmake cmake -S . -B /tmp/build-emscripten -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DTD_EMSCRIPTEN_LAZY_FETCH_GAMEDATA=ON \
        -DTD_EMSCRIPTEN_PACKAGE_GAMEDATA=OFF \
        -DTD_EMSCRIPTEN_DIAGNOSTIC=${TD_EMSCRIPTEN_DIAGNOSTIC} \
    && cmake --build /tmp/build-emscripten --target tiberian-dawn -j"$(nproc)" \
    && touch /tmp/build-emscripten/tiberian-dawn.wasm.map


FROM emscripten-build AS emscripten-gamedata

RUN python3 docker/filter_emscripten_gamedata.py \
        --manifest /tmp/build-emscripten/tiberian-dawn-assets-manifest.txt \
        --source-root /src/GameData \
        --output-root /tmp/deploy-root/GameData


FROM ${DEBIAN_IMAGE} AS zig-build

ARG ZIG_VERSION

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates curl xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /tmp

RUN curl -fsSL "https://ziglang.org/download/${ZIG_VERSION}/zig-x86_64-linux-${ZIG_VERSION}.tar.xz" -o zig.tar.xz \
    && mkdir -p /opt/zig \
    && tar -xJf zig.tar.xz --strip-components=1 -C /opt/zig

WORKDIR /src/server

COPY server/build.zig server/build.zig.zon ./
COPY server/src ./src

RUN /opt/zig/zig build -Doptimize=ReleaseSafe


FROM ${DEBIAN_IMAGE} AS web-runtime-base

RUN apt-get update \
    && apt-get install -y --no-install-recommends ca-certificates wget \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /srv

COPY --from=zig-build /src/server/zig-out/bin/td-wol-server /usr/local/bin/td-wol-server

EXPOSE 80

HEALTHCHECK CMD wget -q -O /dev/null http://127.0.0.1/healthz || exit 1

ENTRYPOINT ["/usr/local/bin/td-wol-server"]
CMD ["--host", "0.0.0.0", "--port", "80", "--emscripten-dir", "/srv/web"]


FROM web-runtime-base AS web-runtime

COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.html /srv/web/tiberian-dawn.html
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.js /srv/web/tiberian-dawn.js
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.wasm /srv/web/tiberian-dawn.wasm
COPY --from=emscripten-build /tmp/build-emscripten/site.webmanifest /srv/web/site.webmanifest
COPY --from=emscripten-build /tmp/build-emscripten/icons /srv/web/icons
# Source map (present only when TD_EMSCRIPTEN_DIAGNOSTIC=ON; an empty placeholder
# is produced otherwise so this copy step is stable across build modes).
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.wasm.map /srv/web/tiberian-dawn.wasm.map


FROM web-runtime-base AS web-runtime-with-gamedata

COPY --from=emscripten-gamedata /tmp/deploy-root/GameData /srv/GameData
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.html /srv/web/tiberian-dawn.html
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.js /srv/web/tiberian-dawn.js
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.wasm /srv/web/tiberian-dawn.wasm
COPY --from=emscripten-build /tmp/build-emscripten/site.webmanifest /srv/web/site.webmanifest
COPY --from=emscripten-build /tmp/build-emscripten/icons /srv/web/icons
COPY --from=emscripten-build /tmp/build-emscripten/tiberian-dawn.wasm.map /srv/web/tiberian-dawn.wasm.map

CMD ["--host", "0.0.0.0", "--port", "80", "--gamedata", "/srv/GameData", "--emscripten-dir", "/srv/web"]
