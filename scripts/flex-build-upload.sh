#!/bin/bash
# flex-build-upload.sh
# Version: 1.5.0
# Changelog:
#   1.5.0 - Add check_prerequisites(): validates arduino-cli, the esp32:esp32 board core,
#           and the Arduino libraries required by the requested --enable-* flags before
#           compiling, auto-installing anything missing (core/libraries only — arduino-cli
#           itself must be installed manually, see error message). New --skip-prereqs flag
#           bypasses this check.
#   1.4.0 - Replace --no-imap/--no-chatgpt/--no-gsm with --enable-rtc/--enable-imap/
#           --enable-chatgpt/--enable-gsm: RTC, IMAP, ChatGPT and GSM are now all
#           DISABLED by default and opted into individually via -DENABLE_* compiler
#           defines, without editing config.h
#   1.3.0 - Add --no-imap/--no-chatgpt/--no-gsm to include/exclude optional modules
#           at build time via -DDISABLE_* compiler defines, without editing config.h
#   1.2.0 - Read version from version.h's FIRMWARE_VERSION instead of grepping the
#           sketch for CURRENT_VERSION (flex-fsk-tx-v2's per-subsystem-file layout
#           moved the version macro out of the .ino and renamed it)
#   1.1.0 - Add -u flag for upload control, -e for EraseFlash=all, build-only mode
#   1.0.2 - Fix handle_non_ino_file to work with absolute paths
#   1.0.1 - Support absolute/relative paths, compile from any directory
# Usage:
#   ./flex-build-upload.sh [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] sketch.ino
#   ./flex-build-upload.sh [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] backup-file

set -uo pipefail

# OPTIONS env var is eval'd - ensure it contains safe values only
OPTIONS=${OPTIONS:-""}

# Board defaults (overridden via -t/--type)
BOARD_TYPE="ttgo"
PORT=""
FQBN=""
BOARD_DEFAULT_PORT=""
BUILD_PROPERTIES=()
MODULE_DEFINES=()
DO_UPLOAD=false
ERASE_FLASH="none"
SKIP_PREREQS=false

function check_current_version() {
    local sketch_dir=$(dirname "${SKETCH}")
    local version_file="${sketch_dir}/src/version.h"

    if [[ -f "${version_file}" ]]; then
        CURRENT_VERSION=$(grep -m1 -E '^\s*#define\s+FIRMWARE_VERSION' "${version_file}" | awk '{print$3}' | tr -d '"')
    else
        CURRENT_VERSION=""
    fi

    local backup_dir="${sketch_dir}/bkp"
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

    if [[ ! -f "${source_file}" ]]; then
        echo "Error: Source file '${source_file}' does not exist!"
        exit 1
    fi

    local source_abs=$(realpath "${source_file}")
    local backup_basename=$(basename "${source_abs}")
    local original_filename="${backup_basename%.bkp*}"

    local backup_dir=$(dirname "${source_abs}")
    local target_dir=$(dirname "${backup_dir}")
    local target_file="${target_dir}/${original_filename}"

    if [[ -f "${target_file}" ]]; then
        echo "Warning: Target file '${target_file}' already exists!"
        read -p "Overwrite? (y/N): " -n 1 -r
        echo
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            echo "Operation cancelled."
            exit 1
        fi

        local backup_dir_for_existing="${target_dir}/bkp"
        mkdir -p "${backup_dir_for_existing}"
        local timestamp
        timestamp=$(date +%y%m%d%H%M%S)
        local backup_name="${backup_dir_for_existing}/$(basename "${target_file}").bkp-${timestamp}"
        /bin/cp -f "${target_file}" "${backup_name}"
        echo "Existing file backed up: ${backup_name}"
    fi

    /bin/cp -f "${source_abs}" "${target_file}"
    echo "Restored '${source_abs}' to '${target_file}'"
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

function check_prerequisites() {
    echo "Checking build prerequisites..."

    if ! command -v arduino-cli &>/dev/null; then
        echo "Error: arduino-cli not found in PATH."
        echo "arduino-cli cannot be auto-installed by this script (nothing else can"
        echo "bootstrap it). Install it manually, then re-run this script:"
        echo "  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh"
        exit 1
    fi

    if ! arduino-cli core list 2>/dev/null | awk 'NR>1 {print $1}' | grep -qx "esp32:esp32"; then
        echo "Installing ESP32 board core (esp32:esp32)..."
        arduino-cli core install esp32:esp32 || {
            echo "Error: failed to install esp32:esp32 board core."
            exit 1
        }
    fi

    local required_libs=("RadioLib" "U8g2" "ArduinoJson" "PubSubClient")
    for define in "${MODULE_DEFINES[@]}"; do
        case "${define}" in
            -DENABLE_RTC)  required_libs+=("RTClib") ;;
            -DENABLE_IMAP) required_libs+=("ReadyMail") ;;
            -DENABLE_GSM)  required_libs+=("TinyGSM" "SSLClient") ;;
        esac
    done

    local lib
    for lib in "${required_libs[@]}"; do
        if arduino-cli lib list "${lib}" 2>/dev/null | grep -q "No libraries installed."; then
            echo "Installing missing library: ${lib}..."
            arduino-cli lib install "${lib}" || {
                echo "Error: failed to install library '${lib}'."
                exit 1
            }
        fi
    done

    echo "All prerequisites satisfied."
}

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] sketch.ino"
    exit 1
fi

PARSED_ARGS=$(getopt -o t:p:ue -l type:,port:,upload,erase,enable-rtc,enable-imap,enable-chatgpt,enable-gsm,skip-prereqs -- "$@") || {
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] sketch.ino"
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
        -u|--upload)
            DO_UPLOAD=true
            shift
            ;;
        -e|--erase)
            ERASE_FLASH="all"
            shift
            ;;
        --enable-rtc)
            MODULE_DEFINES+=("-DENABLE_RTC")
            shift
            ;;
        --enable-imap)
            MODULE_DEFINES+=("-DENABLE_IMAP")
            shift
            ;;
        --enable-chatgpt)
            MODULE_DEFINES+=("-DENABLE_CHATGPT")
            shift
            ;;
        --enable-gsm)
            MODULE_DEFINES+=("-DENABLE_GSM")
            shift
            ;;
        --skip-prereqs)
            SKIP_PREREQS=true
            shift
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] sketch.ino"
            exit 1
            ;;
    esac
done

if [[ $# -lt 1 ]]; then
    echo "Usage: $0 [-t ttgo|heltec] [-p /dev/ttyUSBX] [-u] [-e] [--enable-rtc] [--enable-imap] [--enable-chatgpt] [--enable-gsm] [--skip-prereqs] sketch.ino"
    exit 1
fi

INPUT_FILE="$1"
shift || true

if [[ ! -f "${INPUT_FILE}" ]]; then
    echo "Error: Input file '${INPUT_FILE}' does not exist!"
    exit 1
fi

INPUT_FILE=$(realpath "${INPUT_FILE}")

case "${BOARD_TYPE}" in
    ttgo)
        FQBN="esp32:esp32:ttgo-lora32:Revision=TTGO_LoRa32_v21new,FlashFreq=80,UploadSpeed=921600,DebugLevel=none,EraseFlash=${ERASE_FLASH}"
        BOARD_DEFAULT_PORT="/dev/ttyACM0"
        BUILD_PROPERTIES=(
            "build.partitions=min_spiffs"
            "upload.maximum_size=1966080"
            "compiler.cpp.extra_flags=-DTTGO_LORA32_V21"
       )
        ;;
    heltec|heltec_v2)
        FQBN="esp32:esp32:heltec_wifi_lora_32_V2:CPUFreq=240,UploadSpeed=921600,DebugLevel=none,LORAWAN_REGION=0,LoRaWanDebugLevel=0,LORAWAN_DEVEUI=0,LORAWAN_PREAMBLE_LENGTH=0,EraseFlash=${ERASE_FLASH}"
        BOARD_DEFAULT_PORT="/dev/ttyUSB0"
        BUILD_PROPERTIES=(
            "compiler.cpp.extra_flags=-DHELTEC_WIFI_LORA32_V2"
        )
        BOARD_TYPE="heltec"
        ;;
    *)
        echo "Unsupported board type '${BOARD_TYPE}'. Use 'ttgo' or 'heltec'."
        exit 1
        ;;
esac

if [[ ${#MODULE_DEFINES[@]} -gt 0 ]]; then
    last_idx=$(( ${#BUILD_PROPERTIES[@]} - 1 ))
    BUILD_PROPERTIES[$last_idx]="${BUILD_PROPERTIES[$last_idx]} ${MODULE_DEFINES[*]}"
fi

if [[ -z "${PORT}" ]]; then
    PORT="${BOARD_DEFAULT_PORT}"
fi

echo "Target board: ${BOARD_TYPE} (${FQBN})"

if [[ ${#MODULE_DEFINES[@]} -gt 0 ]]; then
    enabled_modules=""
    for define in "${MODULE_DEFINES[@]}"; do
        case "${define}" in
            -DENABLE_RTC)     enabled_modules+="RTC, " ;;
            -DENABLE_IMAP)    enabled_modules+="IMAP, " ;;
            -DENABLE_CHATGPT) enabled_modules+="ChatGPT, " ;;
            -DENABLE_GSM)     enabled_modules+="GSM, " ;;
        esac
    done
    echo "Modules enabled: ${enabled_modules%, }"
else
    echo "Modules enabled: none (RTC, IMAP, ChatGPT, GSM all disabled)"
fi

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
        BUILD_PROPERTIES_ARGS+=" --build-property '${property}'"
    done
fi

if [[ "${SKIP_PREREQS}" != true ]]; then
    check_prerequisites
else
    echo "Skipping prerequisite check (--skip-prereqs)"
fi

check_current_version

if [[ "${DO_UPLOAD}" == true ]]; then
    check_port_busy
    echo "[$(date)] arduino-cli compile --upload -b ${FQBN} ${BUILD_PROPERTIES_ARGS} ${OPTIONS} -p ${PORT} ${SKETCH}"
    eval arduino-cli compile --upload -b "${FQBN}" ${BUILD_PROPERTIES_ARGS} ${OPTIONS} -p "${PORT}" "${SKETCH}" -e
else
    echo "[$(date)] arduino-cli compile -b ${FQBN} ${BUILD_PROPERTIES_ARGS} ${OPTIONS} ${SKETCH}"
    eval arduino-cli compile -b "${FQBN}" ${BUILD_PROPERTIES_ARGS} ${OPTIONS} "${SKETCH}" -e
fi

if [[ $? -eq 0 && "${DO_UPLOAD}" == true ]]; then
    if command -v mate-terminal &>/dev/null; then
        mate-terminal -e "arduino-cli monitor --config 115200 -p ${PORT}"
    elif command -v gnome-terminal &>/dev/null; then
        gnome-terminal -- arduino-cli monitor --config 115200 -p ${PORT}
    else
        echo "No terminal emulator found, run manually: arduino-cli monitor --config 115200 -p ${PORT}"
    fi
fi
