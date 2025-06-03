CC = gcc
CFLAGS = -Iinclude -lpthread

COMMON_SRC = src/transport.c src/packet_handler.c

BROKER_SRC = src/broker.c $(COMMON_SRC) src/topic_table.c src/pending_table.c
BROKER_BIN = broker

CLIENT_COMMON_SRC = src/slimmq_client.c $(COMMON_SRC) src/event_queue.c src/qos2_table.c

CLIENT_EXAMPLES = \
    client_publisher \
    client_subscriber \
    client_publisher_qos2

client_publisher_SRC = src/client_publisher.c $(CLIENT_COMMON_SRC)
client_subscriber_SRC = src/client_subscriber.c $(CLIENT_COMMON_SRC)
client_publisher_qos2_SRC = src/client_publisher_qos2.c $(CLIENT_COMMON_SRC)

.PHONY: all clean client_examples

all: $(BROKER_BIN) client_examples

$(BROKER_BIN): $(BROKER_SRC)
	$(CC) -o $@ $^ $(CFLAGS)

$(CLIENT_EXAMPLES):
	$(CC) -o $@ $($@_SRC) $(CFLAGS)

client_examples: $(CLIENT_EXAMPLES)

clean:
	rm -f $(BROKER_BIN) $(CLIENT_EXAMPLES)

