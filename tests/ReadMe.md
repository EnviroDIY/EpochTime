# EpochTime tests

The native test suite exercises conversion behavior that does not require
Arduino hardware, including:

- Unix, Y2K, NIST, and GPS epoch conversions and round trips
- Positive and negative UTC-offset normalization
- GPS leap-second boundaries and timestamps wider than 32 bits
- `time_t` and `tm` conversion with a non-UTC processor timezone
- ISO 8601 and custom formatting overloads
- Formatting-buffer truncation and sanity-range boundaries

Run the suite on Windows PowerShell:

```powershell
./tests/run_native_tests.ps1
```

Run the suite on Linux or macOS:

```sh
bash tests/run_native_tests.sh
```

The Mayfly and Stonefly PlatformIO builds remain necessary to verify Arduino
core compatibility. Native tests validate conversion behavior but are not a
substitute for hardware clock testing.
