#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "../include/slimmq_client.h"
#include "../include/transport.h"
#include "../include/packet_handler.h"
#include "../include/slim_msg.h"
#include "../include/event_queue.h"
#include "../include/qos2_table.h"

#define MAX_PACKET_SIZE 2048

static void* listener_loop(void* arg) {
	slimmq_client_t* client = (slimmq_client_t*)arg;

	uint8_t buffer[MAX_PACKET_SIZE];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	slim_msg_header_t header;
	char topic_buf[256];
	uint8_t payload_buf[MAX_PACKET_SIZE];

	while(client->running) {
		int len = recv_bytes(client->sockfd, buffer,
												sizeof(buffer),
												(struct sockaddr*)&from, &fromlen);
		if (len <= 0) continue;

		if (deserialize_message(buffer, len, &header,
														topic_buf,
														sizeof(topic_buf),
														payload_buf,
														sizeof(payload_buf)) != 0) {
			continue;
		}

		switch (header.msg_type) {
			case MSG_ACK:
				event_queue_push(&client->event_queue, MSG_ACK, header.msg_id, topic_buf, NULL, 0);
				break;

			case MSG_CONTROL: {
				control_type_t ctrl_type;
				char ctrl_data[256];

				if (deserialize_control_message(buffer, len, &header, &ctrl_type, ctrl_data, sizeof(ctrl_data)) == 0) {
					if (ctrl_type == CONTROL_RECEIVED) {
						qos2_table_set(header.msg_id, QOS2_CLIENT_STATE_WAIT_COMPLETE);
					} else if (ctrl_type == CONTROL_COMPLETE) {
						qos2_table_set(header.msg_id, QOS2_CLIENT_STATE_COMPLETED);
					}
				}
				break;
			}

			case MSG_PUBLISH:
				event_queue_push(&client->event_queue, MSG_PUBLISH, header.msg_id, topic_buf, payload_buf, header.payload_length - (1 + strlen(topic_buf)));
				break;

			default:
				break;
		}
	}
	return NULL;
}

slimmq_client_t* slimmq_connect(const char* broker_ip, uint16_t port) {
	slimmq_client_t* client = calloc(1, sizeof(slimmq_client_t));
	if (!client) return NULL;

	client->sockfd = init_socket(NULL, 0);  // ephemeral port
	client->qos_level = QOS_AT_MOST_ONCE;
	if (client->sockfd < 0) {
			free(client);
			return NULL;
	}

	memset(&client->broker_addr, 0, sizeof(client->broker_addr));
	client->broker_addr.sin_family = AF_INET;
	client->broker_addr.sin_port = htons(port);
	if (inet_pton(AF_INET, broker_ip,
				&client->broker_addr.sin_addr) <= 0) {
			close(client->sockfd);
			free(client);
			return NULL;
	}

	client->next_msg_id = 1;

	event_queue_init(&client->event_queue);
	client->running = 1;
	pthread_create(&client->listener_thread, NULL,
			listener_loop, client);

	return client;
}

void slimmq_close(slimmq_client_t* client) {
	if (!client) return;
	
	client->running = 0;
	pthread_cancel(client->listener_thread);
	pthread_join(client->listener_thread, NULL);

	event_queue_destroy(&client->event_queue);
	close(client->sockfd);

	if (client->qos_level == QOS_EXACTLY_ONCE) {
		qos2_table_destroy();
	}

	free(client);
}

int slimmq_subscribe(slimmq_client_t* client, const char* topic) {
	slim_msg_header_t header = {
		.version = 1,
		.msg_type = MSG_SUBSCRIBE,
		.qos_level = QOS_AT_MOST_ONCE,
		.msg_id = client->next_msg_id++,
		.payload_length = 1 + strlen(topic),
		.topic_id = 0,
		.frag_id = 0,
		.frag_total = 1,
		.batch_size = 1,
		.client_node_count = 1
	};

	uint8_t buffer[MAX_PACKET_SIZE];
	int len = serialize_message(&header, topic, NULL, 0,
			buffer, sizeof(buffer));
	if (len < 0) return -1;

	return send_bytes(client->sockfd,
			(struct sockaddr*)&client->broker_addr,
			sizeof(client->broker_addr), buffer, len);
}

int slimmq_publish(slimmq_client_t* client, const char* topic,
										const void* data, size_t data_len) {
	if (!client || !topic) return -1;

	slim_msg_header_t header = {
		.version = 1,
		.msg_type = MSG_PUBLISH,
		.qos_level = client->qos_level,
		.msg_id = client->next_msg_id++,
		.payload_length = 1 + strlen(topic) + data_len,
		.topic_id = 0,
		.frag_id = 0,
		.frag_total = 1,
		.batch_size = 1,
		.client_node_count = 1
	};

	uint8_t buffer[MAX_PACKET_SIZE];
	int len = serialize_message(&header, topic, data, data_len,
			buffer, sizeof(buffer));
	if (len < 0) return -1;

	int retries = 0;

	while (retries <= client->max_retries) {
		int sent = send_bytes(client->sockfd,
				(struct sockaddr*)&client->broker_addr,
				sizeof(client->broker_addr), buffer, len);

		if (sent < 0) return -1;

		if (header.qos_level == QOS_AT_MOST_ONCE)
			return 0;

		if (header.qos_level == QOS_AT_LEAST_ONCE) {
			for (int i = 0; i < client->retry_timeout_ms / 100; ++i) {
				usleep(100 * 1000);
				if (event_queue_wait_ack(&client->event_queue,
																	header.msg_id) == 0) {
					return 0;
				}
			}
			retries++;
			continue;
		}

		if (header.qos_level == QOS_EXACTLY_ONCE) {
			for (int i = 0; i < client->retry_timeout_ms / 100; ++i) {
				usleep(100 * 1000);
				if (qos2_table_get_state(header.msg_id)
						== QOS2_CLIENT_STATE_WAIT_COMPLETE) {
					break;
				}
			}
			if (qos2_table_get_state(header.msg_id)
					!= QOS2_CLIENT_STATE_WAIT_COMPLETE) {
				retries++;
				continue;
			}

			slim_msg_header_t rel_hdr = header;
			rel_hdr.msg_type = MSG_CONTROL;
			rel_hdr.payload_length = 1;

			uint8_t ctrl_buf[256];
			int ctrl_len = serialize_control_message(&rel_hdr,
																								CONTROL_RELEASE,
																								NULL, 0,
																								ctrl_buf,
																								sizeof(ctrl_buf));
			if (ctrl_len > 0) {
				send_bytes(client->sockfd,
										(struct sockaddr*)&client->broker_addr,
										sizeof(client->broker_addr),
										ctrl_buf, ctrl_len);
			}

			for (int i = 0; i < client->retry_timeout_ms / 100; ++i) {
				usleep(100 * 1000);
				if (qos2_table_get_state(header.msg_id)
						== QOS2_CLIENT_STATE_COMPLETED) {
					qos2_table_remove(header.msg_id);
					return 0;
				}
			}

			retries++;
		}
	}
	fprintf(stderr, "[CLIENT] Failed to publish (qos=%d) msg_id=%u\n",
					client->qos_level, header.msg_id);
	return -1;
}

int slimmq_receive(slimmq_client_t* client, char* out_topic,
		size_t topic_buf_size, void* out_data, size_t data_buf_size) {
	uint8_t buffer[MAX_PACKET_SIZE];
	struct sockaddr_in from;
	socklen_t fromlen = sizeof(from);
	slim_msg_header_t header;

	int received = recv_bytes(client->sockfd, buffer,
			sizeof(buffer), (struct sockaddr*)&from,
			&fromlen);
	if (received < 0) return -1;

	return deserialize_message(buffer, received,
			&header, out_topic, topic_buf_size, out_data,
			data_buf_size);
}

int slimmq_next_event(slimmq_client_t* client, char* out_topic,
		size_t topic_buf_size, void** out_data, size_t* out_data_len) {
	slimmq_event_t evt;
	if (event_queue_pop(&client->event_queue, &evt) != 0) return -1;

	strncpy(out_topic, evt.topic, topic_buf_size - 1);
	out_topic[topic_buf_size - 1] = '\0';

	*out_data = evt.data;
	*out_data_len = evt.data_len;

	return 0;
}

void slimmq_set_qos(slimmq_client_t* client, uint8_t qos_level) {
	if (!client) return;

	client->qos_level = qos_level;

	if (qos_level == QOS_EXACTLY_ONCE) {
		qos2_table_init();
	}
}

void slimmq_set_retry_policy(slimmq_client_t* client, int timeout_ms, int max_retries) {
	if (client) {
		client->retry_timeout_ms = timeout_ms;
		client->max_retries = max_retries;
	}
}
