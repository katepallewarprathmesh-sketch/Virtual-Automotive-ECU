#!/usr/bin/env bash
set -e

if ! command -v ip >/dev/null 2>&1; then
  echo "ip command not found. Install iproute2."
  exit 1
fi

sudo modprobe vcan
sudo ip link add dev vcan0 type vcan
sudo ip link set up vcan0

echo "Created vcan0 interface."

echo "Use 'candump vcan0' in another terminal to monitor traffic."