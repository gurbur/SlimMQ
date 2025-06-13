CC = gcc
CFLAGS = -Iinclude -lpthread
BUILDDIR = builds

# Transport type can be udp or tcp (default: udp)
TRANSPORT ?= udp

ifeq ($(TRANSPORT),udp)
TRANSPORT_SRC = src/transport_udp.c
CLIENT_TRANSPORT_SRC = src/slimmq_client_udp.c
else ifeq ($(TRANSPORT),tcp)
TRANSPORT_SRC = src/transport_tcp.c
CLIENT_TRANSPORT_SRC = src/slimmq_client_tcp.c
else
$(error Invalid TRANSPORT type: must be udp or tcp)
endif

COMMON_SRC = $(TRANSPORT_SRC) src/packet_handler.c
ifeq ($(TRANSPORT),udp)
BROKER_SRC = src/broker_udp.c $(COMMON_SRC) src/topic_table.c src/pending_table.c
else ifeq ($(TRANSPORT),tcp)
BROKER_SRC = src/broker_tcp.c $(COMMON_SRC) src/topic_table.c src/pending_table.c
else
$(error Invalid TRANSPORT type: must be udp or tcp)
endif
BROKER_BIN = $(BUILDDIR)/broker.$(TRANSPORT)

CLIENT_COMMON_SRC = $(CLIENT_TRANSPORT_SRC) $(COMMON_SRC) src/event_queue.c src/qos2_table.c

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

client_loss_test_qos0_SRC              = test/client_test_loss_qos0.c              $(CLIENT_COMMON_SRC)
client_loss_test_subscriber_qos0_SRC   = test/client_test_loss_subscriber_qos0.c   $(CLIENT_COMMON_SRC)
client_loss_test_qos1_SRC              = test/client_test_loss_qos1.c              $(CLIENT_COMMON_SRC)
client_loss_test_subscriber_qos1_SRC   = test/client_test_loss_subscriber_qos1.c   $(CLIENT_COMMON_SRC)
lossy_broker_SRC                       = test/lossy_broker.c                        $(COMMON_SRC) src/topic_table.c src/pending_table.c

.PHONY: all udp tcp clean client_examples client_tests client_loss_tests

all: udp

udp:
	$(MAKE) TRANSPORT=udp build_all

tcp:
	$(MAKE) TRANSPORT=tcp build_all

build_all: $(BROKER_BIN) client_examples client_tests client_loss_tests

client_examples: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_publisher.$(TRANSPORT)         $(client_publisher_SRC)        $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber.$(TRANSPORT)        $(client_subscriber_SRC)       $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_publisher_qos1.$(TRANSPORT)    $(client_publisher_qos1_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber_qos1.$(TRANSPORT)   $(client_subscriber_qos1_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_publisher_qos2.$(TRANSPORT)    $(client_publisher_qos2_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_subscriber_qos2.$(TRANSPORT)   $(client_subscriber_qos2_SRC)  $(CFLAGS)

client_tests: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos0.$(TRANSPORT)    $(client_test_perf_qos0_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos1.$(TRANSPORT)    $(client_test_perf_qos1_SRC)   $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_perf_qos2.$(TRANSPORT)    $(client_test_perf_qos2_SRC)   $(CFLAGS)

client_loss_tests: | $(BUILDDIR)
	$(CC) -o $(BUILDDIR)/client_test_loss_qos0.$(TRANSPORT)             $(client_loss_test_qos0_SRC)             $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_subscriber_qos0.$(TRANSPORT)  $(client_loss_test_subscriber_qos0_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_qos1.$(TRANSPORT)             $(client_loss_test_qos1_SRC)             $(CFLAGS)
	$(CC) -o $(BUILDDIR)/client_test_loss_subscriber_qos1.$(TRANSPORT)  $(client_loss_test_subscriber_qos1_SRC)  $(CFLAGS)
	$(CC) -o $(BUILDDIR)/lossy_broker.$(TRANSPORT)                      $(lossy_broker_SRC)                      $(CFLAGS)

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BROKER_BIN): $(BROKER_SRC) | $(BUILDDIR)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(BUILDDIR)

