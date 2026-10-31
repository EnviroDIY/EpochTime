#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
output_directory="${repo_root}/.pio/native-tests"
test_executable="${output_directory}/epoch_time_tests"

mkdir -p "${output_directory}"

g++ \
    -std=c++11 \
    -Wall \
    -Wextra \
    -Werror \
    -DEPOCHTIME_DISABLE_STRING_FORMATTING=1 \
    -DEPOCHTIME_ENABLE_STRFTIME=1 \
    -include "${repo_root}/tests/native/time_compat.h" \
    -I "${repo_root}/src" \
    "${repo_root}/tests/native/test_epoch_time.cpp" \
    "${repo_root}/src/EpochTime.cpp" \
    -o "${test_executable}"

"${test_executable}"
