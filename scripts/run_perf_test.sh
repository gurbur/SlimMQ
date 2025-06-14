#!/bin/bash

set -e

EXPERIMENTS=3
BUILD_DIR="/slimmq/builds"
RESULT_DIR="./results"
DOCKER_DIR="./docker"
NETWORK_NAME="slimmq_test_net"

mkdir -p "$RESULT_DIR"
docker network create "$NETWORK_NAME" >/dev/null 2>&1 || true

declare -A CASE1=( ["name"]="tcp_qos0" ["broker"]="$BUILD_DIR/broker.tcp" ["client"]="$BUILD_DIR/client_test_perf_qos0.tcp" ["make"]="make tcp" ["dockerfile"]="$DOCKER_DIR/Dockerfile.tcp" )
declare -A CASE2=( ["name"]="udp_qos0" ["broker"]="$BUILD_DIR/broker.udp" ["client"]="$BUILD_DIR/client_test_perf_qos0.udp" ["make"]="make udp" ["dockerfile"]="$DOCKER_DIR/Dockerfile.udp" )
declare -A CASE3=( ["name"]="udp_qos1" ["broker"]="$BUILD_DIR/broker.udp" ["client"]="$BUILD_DIR/client_test_perf_qos1.udp" ["make"]="make udp" ["dockerfile"]="$DOCKER_DIR/Dockerfile.udp" )

ALL_CASES=(CASE1 CASE2 CASE3)

run_case() {
  local -n CASE=$1
  local CASE_NAME=${CASE["name"]}
  local BROKER_BIN=${CASE["broker"]}
  local CLIENT_BIN=${CASE["client"]}
  local MAKE_CMD=${CASE["make"]}
  local DOCKERFILE=${CASE["dockerfile"]}

  echo "[*] Building Docker image for $CASE_NAME..."
  docker build -f "$DOCKERFILE" -t slimmq_test_$CASE_NAME .

  for ((i = 1; i <= $EXPERIMENTS; i++)); do
    echo "[*] Running test $i for $CASE_NAME..."

    BROKER_CONTAINER="broker_${CASE_NAME}_$i"

    # 브로커 컨테이너 실행 (monitor 포함)
    docker run --rm -d \
      --name "$BROKER_CONTAINER" \
      --network "$NETWORK_NAME" \
      -v "$(pwd)/results:/slimmq/results" \
      slimmq_test_$CASE_NAME /bin/bash -c "
        cd /slimmq &&
        $MAKE_CMD &&
        ./scripts/monitor.sh \"$BROKER_BIN\" \"\" \"/slimmq/results/result_${CASE_NAME}_${i}.csv\" > \"/slimmq/results/result_${CASE_NAME}_${i}.log\"
      "

    sleep 2  # 브로커 준비 시간

    # 브로커 컨테이너 IP 조회
    BROKER_IP=$(docker inspect -f '{{range.NetworkSettings.Networks}}{{.IPAddress}}{{end}}' "$BROKER_CONTAINER")
    echo "    └─ Detected broker IP: $BROKER_IP"

    # 클라이언트 컨테이너 실행 (IP + 포트 지정)
    docker run --rm \
      --name client_${CASE_NAME}_$i \
      --network "$NETWORK_NAME" \
      slimmq_test_$CASE_NAME /bin/bash -c "
        cd /slimmq &&
        $MAKE_CMD &&
        $CLIENT_BIN -ip $BROKER_IP -p 9000
      "

    # 브로커 종료 대기
    docker wait "$BROKER_CONTAINER" >/dev/null

    echo "[+] Test $i for $CASE_NAME completed."
  done
}

for CASE in "${ALL_CASES[@]}"; do
  run_case $CASE
done

echo "[✓] All tests completed. Results saved in ./results"

