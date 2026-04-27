#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-android}"
ANDROID_PACKAGE_NAME="${ANDROID_PACKAGE_NAME:-com.ea.tiberiandawn}"
ANDROID_ACTIVITY_NAME="${ANDROID_ACTIVITY_NAME:-TiberianDawnActivity}"
LAUNCHA_INSTALL_MODE="${LAUNCHA_INSTALL_MODE:-auto}"

if [[ -f /etc/profile.d/android-ndk.sh ]]; then
    # Arch ships the packaged NDK environment here.
    # shellcheck disable=SC1091
    source /etc/profile.d/android-ndk.sh
fi

if [[ -z "${ANDROID_HOME:-}" && -d /opt/android-sdk ]]; then
    export ANDROID_HOME=/opt/android-sdk
fi
if [[ -z "${ANDROID_SDK_ROOT:-}" && -n "${ANDROID_HOME:-}" ]]; then
    export ANDROID_SDK_ROOT="${ANDROID_HOME}"
fi
if [[ -z "${SDL_ANDROID_HOME:-}" && -n "${ANDROID_SDK_ROOT:-}" ]]; then
    export SDL_ANDROID_HOME="${ANDROID_SDK_ROOT}"
fi

NDK_ROOT="${ANDROID_NDK_HOME:-${ANDROID_NDK_ROOT:-${ANDROID_NDK:-/opt/android-ndk}}}"
TOOLCHAIN_FILE="${ANDROID_TOOLCHAIN_FILE:-${NDK_ROOT}/build/cmake/android.toolchain.cmake}"
ANDROID_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Debug}"
ANDROID_ABI_VALUE="${ANDROID_ABI:-arm64-v8a}"
ANDROID_PLATFORM_VALUE="${ANDROID_PLATFORM:-android-26}"

die() {
    echo "launcha.sh: $*" >&2
    exit 1
}

data_apk_hash() {
    local data_apk="${BUILD_DIR}/tiberian-dawn-data.apk"
    [[ -f "${data_apk}" ]] || return 1
    sha256sum "${data_apk}" | awk '{print $1}'
}

run_adb_install() {
    local code_apk="${BUILD_DIR}/tiberian-dawn-code.apk"
    local data_apk="${BUILD_DIR}/tiberian-dawn-data.apk"

    [[ -f "${code_apk}" ]] || die "missing ${code_apk}; Android APK build did not produce the code package."

    if [[ -f "${data_apk}" ]]; then
        "${ADB}" install-multiple --no-incremental -r "${code_apk}" "${data_apk}"
    else
        "${ADB}" install --no-incremental -r "${code_apk}"
    fi
}

run_adb_code_update() {
    local code_apk="${BUILD_DIR}/tiberian-dawn-code.apk"

    [[ -f "${code_apk}" ]] || die "missing ${code_apk}; Android APK build did not produce the code package."

    "${ADB}" install-multiple --no-incremental -r -p "${ANDROID_PACKAGE_NAME}" "${code_apk}"
}

is_package_installed() {
    "${ADB}" shell pm path "${ANDROID_PACKAGE_NAME}" >/dev/null 2>&1
}

has_installed_data_split() {
    "${ADB}" shell pm path "${ANDROID_PACKAGE_NAME}" | grep -Fq 'split_td_data.apk'
}

[[ -n "${ANDROID_SDK_ROOT:-}" ]] || die "ANDROID_SDK_ROOT is not set and /opt/android-sdk was not found."
[[ -f "${TOOLCHAIN_FILE}" ]] || die "Android toolchain file not found at ${TOOLCHAIN_FILE}."

needs_configure=0
if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
    needs_configure=1
elif ! grep -q 'android\.toolchain\.cmake' "${BUILD_DIR}/CMakeCache.txt"; then
    needs_configure=1
fi

if [[ ${needs_configure} -eq 1 ]]; then
    cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" \
        -DCMAKE_BUILD_TYPE="${ANDROID_BUILD_TYPE}" \
        -DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}" \
        -DANDROID_ABI="${ANDROID_ABI_VALUE}" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM_VALUE}"
fi

echo "==> Building Android APKs"
cmake --build "${BUILD_DIR}" --target tiberian-dawn-apk --parallel

if [[ -n "${ADB_BIN:-}" ]]; then
    ADB="${ADB_BIN}"
elif command -v adb >/dev/null 2>&1; then
    ADB="$(command -v adb)"
elif [[ -x "${ANDROID_SDK_ROOT}/platform-tools/adb" ]]; then
    ADB="${ANDROID_SDK_ROOT}/platform-tools/adb"
else
    die "adb not found; install Android platform-tools or set ADB_BIN."
fi

mapfile -t CONNECTED_DEVICES < <("${ADB}" devices | awk 'NR > 1 && $2 == "device" { print $1 }')
if [[ ${#CONNECTED_DEVICES[@]} -eq 0 ]]; then
    die "no connected Android device detected."
fi

if [[ -n "${ANDROID_SERIAL:-}" ]]; then
    if ! printf '%s\n' "${CONNECTED_DEVICES[@]}" | grep -Fxq "${ANDROID_SERIAL}"; then
        die "ANDROID_SERIAL=${ANDROID_SERIAL} is not in the connected device list."
    fi
elif [[ ${#CONNECTED_DEVICES[@]} -gt 1 ]]; then
    printf 'Connected devices:\n' >&2
    printf '  %s\n' "${CONNECTED_DEVICES[@]}" >&2
    die "multiple Android devices detected; set ANDROID_SERIAL to choose one."
else
    export ANDROID_SERIAL="${CONNECTED_DEVICES[0]}"
fi

LAUNCHA_STATE_DIR="${BUILD_DIR}/.launcha"
mkdir -p "${LAUNCHA_STATE_DIR}"
DATA_SPLIT_HASH_FILE="${LAUNCHA_STATE_DIR}/data-${ANDROID_SERIAL}.sha256"
CURRENT_DATA_SPLIT_HASH=""
if [[ -f "${BUILD_DIR}/tiberian-dawn-data.apk" ]]; then
    CURRENT_DATA_SPLIT_HASH="$(data_apk_hash)"
fi

echo "==> Installing on ${ANDROID_SERIAL}"
case "${LAUNCHA_INSTALL_MODE}" in
    auto)
        if [[ -f "${BUILD_DIR}/tiberian-dawn-data.apk" ]] \
            && is_package_installed \
            && has_installed_data_split \
            && [[ -f "${DATA_SPLIT_HASH_FILE}" ]] \
            && [[ "$(cat "${DATA_SPLIT_HASH_FILE}")" == "${CURRENT_DATA_SPLIT_HASH}" ]]; then
            echo "==> Updating code APK only"
            run_adb_code_update
        else
            if [[ -f "${BUILD_DIR}/tiberian-dawn-data.apk" ]]; then
                echo "==> Installing code + data APKs"
            fi
            run_adb_install
            if [[ -n "${CURRENT_DATA_SPLIT_HASH}" ]]; then
                printf '%s\n' "${CURRENT_DATA_SPLIT_HASH}" > "${DATA_SPLIT_HASH_FILE}"
            fi
        fi
        ;;
    full)
        run_adb_install
        if [[ -n "${CURRENT_DATA_SPLIT_HASH}" ]]; then
            printf '%s\n' "${CURRENT_DATA_SPLIT_HASH}" > "${DATA_SPLIT_HASH_FILE}"
        fi
        ;;
    code)
        run_adb_code_update
        ;;
    *)
        die "unsupported LAUNCHA_INSTALL_MODE=${LAUNCHA_INSTALL_MODE}; use auto, full, or code."
        ;;
esac

echo "==> Launching app"
"${ADB}" shell am start-activity -S "${ANDROID_PACKAGE_NAME}/.${ANDROID_ACTIVITY_NAME}"
