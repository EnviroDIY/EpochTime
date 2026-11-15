#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="${repo_root}/.pio/native-tests"

mkdir -p "${output_directory}"

run_configuration() {
    local name="$1"
    local string_define="$2"
    local strftime_define="$3"
    local test_executable="${output_directory}/${name}"

    g++ \
        -std=c++11 \
        -Wall \
        -Wextra \
        -Werror \
        "${string_define}" \
        "${strftime_define}" \
        -include "${repo_root}/tests/native/time_compat.h" \
        -I "${repo_root}/tests/native" \
        -I "${repo_root}/src" \
        "${repo_root}/tests/native/test_epoch_time.cpp" \
        "${repo_root}/src/EpochTime.cpp" \
        -o "${test_executable}"

    "${test_executable}"
}

run_configuration "string_on_strftime_on" \
    "-DEPOCHTIME_ENABLE_STRING_FORMATTING=1" \
    "-DEPOCHTIME_ENABLE_STRFTIME=1"
run_configuration "string_on_strftime_off" \
    "-DEPOCHTIME_ENABLE_STRING_FORMATTING=1" \
    "-DEPOCHTIME_ENABLE_STRFTIME=0"
run_configuration "string_off_strftime_on" \
    "-DEPOCHTIME_DISABLE_STRING_FORMATTING=1" \
    "-DEPOCHTIME_ENABLE_STRFTIME=1"
run_configuration "string_off_strftime_off" \
    "-DEPOCHTIME_DISABLE_STRING_FORMATTING=1" \
    "-DEPOCHTIME_ENABLE_STRFTIME=0"
