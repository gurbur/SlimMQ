#!/bin/bash

# Usage: ./monitor.sh <broker_path> "" <output.csv>
BROKER=$1
OUTFILE=$3
TIMEOUT=30  # seconds

echo "timestamp,cpu_percent,mem_resident_kb,net_rx_kb,net_tx_kb" > "$OUTFILE"

read_net_stats() {
  awk '/eth0/ {print $2, $10}' /proc/net/dev 2>/dev/null
}

read -r rx0 tx0 < <(read_net_stats)
START_TIME=$(date +%s)

# 브로커 실행
$BROKER &
BROKER_PID=$!

# 모니터링 루프
while kill -0 "$BROKER_PID" 2>/dev/null; do
  CURRENT_TIME=$(date +%s)
  ELAPSED=$((CURRENT_TIME - START_TIME))

  if [ "$ELAPSED" -ge "$TIMEOUT" ]; then
    echo "[!] Timeout after $TIMEOUT seconds. Killing broker (PID $BROKER_PID)." >> "$OUTFILE"
    kill -9 "$BROKER_PID"
    break
  fi

  timestamp=$(date +%s)
  cpu=$(ps -p "$BROKER_PID" -o %cpu --no-headers | awk '{print $1}')
  mem=$(grep VmRSS /proc/"$BROKER_PID"/status | awk '{print $2}')
  read -r rx tx < <(read_net_stats)
  rx_diff=$(( (rx - rx0) / 1024 ))
  tx_diff=$(( (tx - tx0) / 1024 ))

  echo "$timestamp,${cpu:-0.0},${mem:-0},$rx_diff,$tx_diff" >> "$OUTFILE"
  sleep 0.5
done

wait "$BROKER_PID" 2>/dev/null || true

