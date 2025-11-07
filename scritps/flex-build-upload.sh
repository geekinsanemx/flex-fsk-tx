#!/bin/bash
# arduino-build-upload.sh
# Usage:
#   ./arduino-build-upload.sh sketch.ino [-t ttgo|heltec] [-p /dev/ttyUSB0]
#   ./arduino-build-upload.sh backup-file [-t ttgo|heltec] [-p /dev/ttyUSB0]

set -uo pipefail

# OPTIONS env var is eval'd - ensure it contains safe values only
OPTIONS=${OPTIONS:-""}

# Board defaults (overridden via -t/--type)
BOARD_TYPE="ttgo"
PORT=""
FQBN=""
BOARD_DEFAULT_PORT=""
BUILD_PROPERTIES=()

function check_current_version() {
    CURRENT_VERSION=$(grep -m1 -E '^\s*#define\s+CURRENT_VERSION' "${SKETCH}" | awk '{print$3}' | tr -d '"')

    local backup_dir="./bkp"
    mkdir -p "${backup_dir}"
    local timestamp
    timestamp=$(date +%y%m%d%H%M%S)
    local backup_name

    if [[ -n "${CURRENT_VERSION}" ]]; then
        backup_name="${backup_dir}/$(basename "${SKETCH}").bkp-${CURRENT_VERSION}-${timestamp}"
        /bin/cp -f "${SKETCH}" "${backup_name}"
        echo "Backup created: ${backup_name} (version: ${CURRENT_VERSION})"
    else
        backup_name="${backup_dir}/$(basename "${SKETCH}").bkp-${timestamp}"
        /bin/cp -f "${SKETCH}" "${backup_name}"
        echo "Backup created: ${backup_name} (no version found)"
    fi
}

function handle_non_ino_file() {
    local source_file="$1"
    local current_dir
    current_dir=$(basename "$(pwd)")
    local target_file="${current_dir}.ino"

    if [[ ! -f "${source_file}" ]]; then
        echo "Error: Source file '${source_file}' does not exist!"
        exit 1
    fi

    if [[ -f "${target_file}" ]]; then
        echo "Warning: Target file '${target_file}' already exists!"
        read -p "Overwrite? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Operation cancelled."
            exit 1
        fi

        local backup_dir="./bkp"
        mkdir -p "${backup_dir}"
        local timestamp
        timestamp=$(date +%y%m%d%H%M%S)
        local backup_name="${backup_dir}/$(basename "${target_file}").bkp-${timestamp}"
        /bin/cp -f "${target_file}" "${backup_name}"
        echo "Existing file backed up: ${backup_name}"
    fi

    /bin/cp -f "${source_file}" "${target_file}"
    echo "Copied '${source_file}' to '${target_file}'"
    echo "${target_file}"
}

function check_port_busy() {
    [[ -z "${PORT}" ]] && return

    local using_pid=""
    if using_pid=$(lsof "${PORT}" 2>/dev/null | awk 'NR>1 {print $2}' | head -n1); then
        :
    else
        using_pid=""
    fi

    if [[ -n "${using_pid}" ]]; then
        echo "port ${PORT} is being used by PID: ${using_pid}, will be killed in 10 seconds if not cancelled..."
        read -t 10 -r || true
        if kill -0 "${using_pid}" 2>/dev/null; then
            kill -15 "${using_pid}" || true
        fi
    fi
}

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] sketch.ino"
    exit 1
fi

PARSED_ARGS=$(getopt -o t:p: -l type:,port: -- "$@") || {
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] sketch.ino"
    exit 1
}

eval set -- "${PARSED_ARGS}"

while true; do
    case "$1" in
        -t|--type)
            BOARD_TYPE="${2,,}"
            shift 2
            ;;
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] sketch.ino"
            exit 1
            ;;
    esac
done

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] sketch.ino"
    exit 1
fi

INPUT_FILE="$1"
shift || true

case "${BOARD_TYPE}" in
    ttgo)
        FQBN="esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new,FlashFreq=80,UploadSpeed=921600,DebugLevel=none,EraseFlash=none"
        BOARD_DEFAULT_PORT="/dev/ttyACM0"
        BUILD_PROPERTIES=(
            "build.partitions=min_spiffs"
            "upload.maximum_size=1966080"
        )
        ;;
    heltec|heltec_v2)
        FQBN="esp32:esp32:heltec_wifi_lora_32_V2:CPUFreq=240,UploadSpeed=921600,DebugLevel=none,LORAWAN_REGION=0,LoRaWanDebugLevel=0,LORAWAN_DEVEUI=0,LORAWAN_PREAMBLE_LENGTH=0,EraseFlash=none"
        BOARD_DEFAULT_PORT="/dev/ttyUSB0"
        BUILD_PROPERTIES=()
        BOARD_TYPE="heltec"
        ;;
    *)
        echo "Unsupported board type '${BOARD_TYPE}'. Use 'ttgo' or 'heltec'."
        exit 1
        ;;
esac

if [[ -z "${PORT}" ]]; then
    PORT="${BOARD_DEFAULT_PORT}"
fi

echo "Target board: ${BOARD_TYPE} (${FQBN})"

if [[ "${INPUT_FILE}" == *.ino ]]; then
    SKETCH="${INPUT_FILE}"
    if [[ ! -f "${SKETCH}" ]]; then
        echo "Error: Sketch file '${SKETCH}' does not exist!"
        exit 1
    fi
else
    SKETCH=$(handle_non_ino_file "${INPUT_FILE}")
fi

BUILD_PROPERTIES_ARGS=""
if [[ ${#BUILD_PROPERTIES[@]} -gt 0 ]]; then
    for property in "${BUILD_PROPERTIES[@]}"; do
        BUILD_PROPERTIES_ARGS+=" --build-property ${property}"
    done
fi

check_current_version
check_port_busy

echo "[$(date)] arduino-cli compile --upload -b ${FQBN} ${BUILD_PROPERTIES_ARGS} ${OPTIONS} -p ${PORT} ${SKETCH}"

eval arduino-cli compile --upload -b "${FQBN}" ${BUILD_PROPERTIES_ARGS} ${OPTIONS} -p "${PORT}" "${SKETCH}" -e

if [[ $? -eq 0 ]]; then
    if command -v mate-terminal &>/dev/null; then
        mate-terminal -e "arduino-cli monitor --config 115200 -p ${PORT}"
    elif command -v gnome-terminal &>/dev/null; then
        gnome-terminal -- arduino-cli monitor --config 115200 -p ${PORT}
    else
        echo "No terminal emulator found, run manually: arduino-cli monitor --config 115200 -p ${PORT}"
    fi
fi
