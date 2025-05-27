#!/bin/bash
set -e

cd "$(dirname "$0")"

TEST_DIR="../test"
SRC_DIR="../src"
INCLUDE_DIR="../include"

echo "=== 🔁 Running all SlimMQ tests ==="

CORE_MODULES="$SRC_DIR/packet_handler.c $SRC_DIR/event_queue.c $SRC_DIR/transport.c $SRC_DIR/topic_table.c"

for file in "$TEST_DIR"/test_*.c; do
	exe="${file%.c}"
	exe_name=$(basename "$exe")
	echo "▶️ Building $exe_name..."

	gcc -o "$exe" "$file" $CORE_MODULES -I"INCLUDE_DIR" -lpthread

	echo "🚀 Running $exe_name..."
	"$exe"
	echo "✅ $exe_name completed."
	echo ""
done

echo "✅ All tests completed successfully!"
