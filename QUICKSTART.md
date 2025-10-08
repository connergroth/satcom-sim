# Quick Start Guide

## Build & Run (30 seconds)

```bash
# Build
make

# Run tests
./build/satcom_tests

# Run simulation
./build/satcom --duration-sec 10 --verbose
```

## Common Commands

```bash
# Clean build
make clean && make

# Run tests
make test

# Run with custom parameters
./build/satcom --duration-sec 15 --loss 0.10 --latency-ms 150 --verbose

# Run demo scenarios
cd build && bash ../scripts/run_examples.sh

# Help
./build/satcom --help
```

## Project Structure

```
satcom-sim/
├── include/          # Headers (commands, telemetry, protocol)
├── src/              # Implementation (satellite, ground station, link)
├── tests/            # Unit & integration tests
├── scripts/          # Demo scripts
├── build/            # Build output (gitignored)
├── CMakeLists.txt    # CMake configuration
├── Makefile          # Direct make build
└── README.md         # Full documentation
```

## Key Files

- **Main executable**: `build/satcom`
- **Test executable**: `build/satcom_tests`
- **Telemetry output**: `telemetry.log` (CSV format)

## Quick Test

```bash
# 5-second test with verbose output
./build/satcom --duration-sec 5 --verbose --seed 42

# Expected: ~15-25 telemetry packets, some retries, final metrics
```

## Interview Talking Points

1. **Concurrency**: Satellite & ground station run as separate threads with thread-safe queues
2. **Reliability**: CRC-16 checksums, ACK/NAK protocol, automatic retries with exponential backoff
3. **Realism**: Configurable latency, jitter, and packet loss simulate real RF links
4. **Determinism**: Seeded RNG for reproducible tests and debugging
5. **Production-ready**: Comprehensive testing, metrics, logging, clean OOP design

## Common Scenarios

**Ideal conditions** (low latency, minimal loss):
```bash
./build/satcom --duration-sec 10 --loss 0.01 --latency-ms 50 --jitter-ms 10
```

**Challenging environment** (high loss, high latency):
```bash
./build/satcom --duration-sec 20 --loss 0.20 --latency-ms 200 --jitter-ms 80 --max-retries 5
```

**Fast telemetry** (10 Hz updates):
```bash
./build/satcom --duration-sec 15 --telemetry-rate-hz 10 --verbose
```

## Metrics Explained

- **Telemetry sent/received**: Successfully acknowledged packets
- **Retries**: Number of retry attempts due to lost ACKs
- **NAKs**: Packets rejected due to CRC errors
- **Drop rate**: Percentage of packets lost in the link layer

## Next Steps

- Read [README.md](README.md) for comprehensive documentation
- Explore [include/](include/) headers to understand the architecture
- Run [tests/basic_tests.cpp](tests/basic_tests.cpp) to see validation
- Modify [src/ground_station.cpp](src/ground_station.cpp) to add new commands
