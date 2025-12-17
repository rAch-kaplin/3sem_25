#!/usr/bin/env bash

CLIENT="./build/client"
SERVER="./build/server"

MODE="$1"
shift || true

kill_servers() {
  pkill -f "$SERVER" 2>/dev/null || true
}

trap kill_servers EXIT

PORTS=()
case "$MODE" in
  1) PORTS=(8125) ;;
  2) PORTS=(8125 8126) ;;
  3) PORTS=(8125 8126 8127) ;;
esac

for p in "${PORTS[@]}"; do
  "$SERVER" -p "$p" &
  sleep 0.3
done

SERVER_IP_PORTS=()
for p in "${PORTS[@]}"; do
  SERVER_IP_PORTS+=("127.0.0.1:$p")
done

"$CLIENT" -i "${SERVER_IP_PORTS[@]}" "$@"

