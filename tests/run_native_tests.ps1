$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent $PSScriptRoot
$compiler = Get-Command g++ -ErrorAction Stop
$outputDirectory = Join-Path $repoRoot ".pio/native-tests"

New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null

$configurations = @(
    @{
        Name = "string_on_strftime_on"
        Defines = @(
            "-DEPOCHTIME_ENABLE_STRING_FORMATTING=1"
            "-DEPOCHTIME_ENABLE_STRFTIME=1"
        )
    }
    @{
        Name = "string_on_strftime_off"
        Defines = @(
            "-DEPOCHTIME_ENABLE_STRING_FORMATTING=1"
            "-DEPOCHTIME_ENABLE_STRFTIME=0"
        )
    }
    @{
        Name = "string_off_strftime_on"
        Defines = @(
            "-DEPOCHTIME_DISABLE_STRING_FORMATTING=1"
            "-DEPOCHTIME_ENABLE_STRFTIME=1"
        )
    }
    @{
        Name = "string_off_strftime_off"
        Defines = @(
            "-DEPOCHTIME_DISABLE_STRING_FORMATTING=1"
            "-DEPOCHTIME_ENABLE_STRFTIME=0"
        )
    }
)

foreach ($configuration in $configurations)
{
    $testExecutable = Join-Path $outputDirectory "$($configuration.Name).exe"
    $compilerArguments = @(
        "-std=c++11"
        "-Wall"
        "-Wextra"
        "-Werror"
    ) + $configuration.Defines + @(
        "-include"
        (Join-Path $repoRoot "tests/native/time_compat.h")
        "-I"
        (Join-Path $repoRoot "tests/native")
        "-I"
        (Join-Path $repoRoot "src")
        (Join-Path $repoRoot "tests/native/test_epoch_time.cpp")
        (Join-Path $repoRoot "src/EpochTime.cpp")
        "-o"
        $testExecutable
    )

    & $compiler.Source @compilerArguments
    if ($LASTEXITCODE -ne 0)
    {
        throw "Native test compilation failed for $($configuration.Name) with exit code $LASTEXITCODE."
    }

    & $testExecutable
    if ($LASTEXITCODE -ne 0)
    {
        throw "Native tests failed for $($configuration.Name) with exit code $LASTEXITCODE."
    }
}
