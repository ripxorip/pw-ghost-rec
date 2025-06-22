#!/usr/bin/env bash

set -e

# Build the project with ninja
if ! ninja -C _out; then
    echo "Build failed. Exiting." >&2
    exit 1
fi

echo "Build succeeded. Starting application..."

# Path to the built binary
BIN="_out/src/pw-ghost-rec"

# Check if pw-link is available
if ! command -v pw-link &> /dev/null; then
    echo "pw-link not found. Please install pipewire-utils." >&2
    exit 1
fi

# Start the program and get its PID
$BIN &
PID=$!

echo "Started $BIN with PID $PID"

# Wait a moment for the node to appear in PipeWire
sleep 1

# Hardcoded node and port names based on pw-link -l output
MIC_NODE="alsa_input.pci-0000_00_1f.3.analog-stereo"
MIC_PORT="capture_FL"
APP_NODE="pw-ghost-rec"
APP_PORT="input"

# Connect microphone to app
pw-link "$MIC_NODE:$MIC_PORT" "$APP_NODE:$APP_PORT"

echo "Routing $MIC_NODE:$MIC_PORT -> $APP_NODE:$APP_PORT"

# Trap Ctrl+C to clean up
trap 'echo "Stopping..."; kill $PID; exit 0' SIGINT

# Wait for the program to finish
wait $PID
