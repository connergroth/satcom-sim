#!/bin/bash
set -e

echo "=== Satellite Simulator Demo Scenarios ==="
echo ""

# Check if executable exists
if [ ! -f "./satcom" ]; then
    echo "Error: satcom executable not found. Please build first:"
    echo "  mkdir -p build && cd build && cmake .. && make"
    exit 1
fi

echo "Scenario 1: Baseline (5% loss, 100ms latency)"
./satcom --duration-sec 10 --loss 0.05 --latency-ms 100 --jitter-ms 30 --telemetry-rate-hz 5 --seed 42
echo ""

echo "---"
echo ""

echo "Scenario 2: High loss (20% loss, 150ms latency)"
./satcom --duration-sec 10 --loss 0.20 --latency-ms 150 --jitter-ms 50 --telemetry-rate-hz 5 --seed 100
echo ""

echo "---"
echo ""

echo "Scenario 3: Fast telemetry (10 Hz, minimal loss)"
./satcom --duration-sec 10 --loss 0.01 --latency-ms 50 --jitter-ms 10 --telemetry-rate-hz 10 --seed 200
echo ""

echo "=== All scenarios complete ==="
echo "Telemetry data saved to: telemetry.log"
