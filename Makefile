CC = gcc
CFLAGS = -Iinclude -lpthread
BUILDDIR = builds

COMMON_SRC = src/transport.c src/packet_handler.c
BROKER_SRC = src/broker.c $(COMMON_SRC) src/topic_table.c src/pending_table.c
BROKER_BIN = $(BUILDDIR)/broker

CLIENT_COMMON_SRC = src/slimmq_client.c $(COMMON_SRC) src/event_queue.c src/qos2_table.c

CLIENT_EXAMPLES = \
    client_publisher \
    client_subscriber \
    client_publisher_qos1 \
    client_subscriber_qos1 \
    client_publisher_qos2 \
    client_subscriber_qos2

CLIENT_TESTS = \
    client_test_perf_qos0 \
    client_test_perf_qos1 \
    client_test_perf_qos2

client_publisher_SRC        = src/client_publisher.c        $(CLIENT_COMMON_SRC)
client_subscriber_SRC       = src/client_subscriber.c       $(CLIENT_COMMON_SRC)
client_publisher_qos1_SRC   = src/client_publisher_qos1.c   $(CLIENT_COMMON_SRC)
client_subscriber_qos1_SRC  = src/client_subscriber_qos1.c  $(CLIENT_COMMON_SRC)
client_publisher_qos2_SRC   = src/client_publisher_qos2.c   $(CLIENT_COMMON_SRC)
client_subscriber_qos2_SRC  = src/client_subscriber_qos2.c  $(CLIENT_COMMON_SRC)

client_test_perf_qos0_SRC   = test/test_perf_qos0.c         $(CLIENT_COMMON_SRC)
client_test_perf_qos1_SRC   = test/test_perf_qos1.c         $(CLIENT_COMMON_SRC)
client_test_perf_qos2_SRC   = test/test_perf_qos2.c         $(CLIENT_COMMON_SRC)

.PHONY: all clean client_examples client_tests

all: $(BROKER_BIN) client_examples client_tests


client_loss_test_qos0_SRC              = test/client_test_loss_qos0.c              $(CLIENT_COMMON_SRC)
client_loss_test_subscriber_qos0_SRC   = test/client_test_loss_subscriber_qos0.c   $(CLIENT_COMMON_SRC)
client_loss_test_qos1_SRC              = test/client_test_loss_qos1.c              $(CLIENT_COMMON_SRC)
client_loss_test_subscriber_qos1_SRC   = test/client_test_loss_subscriber_qos1.c   $(CLIENT_COMMON_SRC)
lossy_broker_SRC                       = test/lossy_broker.c                        $(COMMON_SRC) src/topic_table.c src/pending_table.c

client_loss_tests: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_test_loss_qos0             $(client_loss_test_qos0_SRC)             $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_subscriber_qos0  $(client_loss_test_subscriber_qos0_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_qos1             $(client_loss_test_qos1_SRC)             $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_subscriber_qos1  $(client_loss_test_subscriber_qos1_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/lossy_broker                      $(lossy_broker_SRC)                      $(CFLAGS)

all: client_loss_tests

clean:
	rm -rf $(BUILDDIR)


$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BROKER_BIN): $(BROKER_SRC) | $(BUILDDIR)
	$(CC) -o $@ $^ $(CFLAGS)

client_examples: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_publisher         $(client_publisher_SRC)        $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber        $(client_subscriber_SRC)       $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_publisher_qos1    $(client_publisher_qos1_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber_qos1   $(client_subscriber_qos1_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_publisher_qos2    $(client_publisher_qos2_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber_qos2   $(client_subscriber_qos2_SRC)  $(CFLAGS)

client_tests: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos0    $(client_test_perf_qos0_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos1    $(client_test_perf_qos1_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos2    $(client_test_perf_qos2_SRC)   $(CFLAGS)

clean:
	rm -rf $(BUILDDIR)

