$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Get-Command g++ -ErrorAction Stop
$outputDirectory = Join-Path $repoRoot ".pio/native-tests"
$testExecutable = Join-Path $outputDirectory "epoch_time_tests.exe"

New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

& $compiler.Source `
    -std=c++11 `
    -Wall `
    -Wextra `
    -Werror `
    -DEPOCHTIME_DISABLE_STRING_FORMATTING=1 `
    -DEPOCHTIME_ENABLE_STRFTIME=1 `
    -include (Join-Path $repoRoot "tests/native/time_compat.h") `
    -I (Join-Path $repoRoot "src") `
(Join-Path $repoRoot "tests/native/test_epoch_time.cpp") `
(Join-Path $repoRoot "src/EpochTime.cpp") `
    -o $testExecutable

if ($LASTEXITCODE -ne 0)
{
    throw "Native test compilation failed with exit code $LASTEXITCODE."
}

& $testExecutable
if ($LASTEXITCODE -ne 0)
{
    throw "Native tests failed with exit code $LASTEXITCODE."
}
