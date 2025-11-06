

#include <csp/csp.h>

#include <csp/csp_debug.h>

#include <csp/csp_cmp.h>
#include <endian.h>
#include <csp/arch/csp_time.h>
#include <csp/csp_hooks.h>

int csp_ping(uint16_t node, uint32_t timeout, unsigned int size, uint8_t conn_options) {

	unsigned int i;
	uint32_t start, time, status = 0;

	/* Check if size do not overflow the data max size */
	if (size > CSP_BUFFER_SIZE) {
		return -1;
	}

	/* Counter */
	start = csp_get_ms();

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PING, timeout, conn_options);
	if (conn == NULL)
		return -1;

	/* Prepare data */
	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL)
		goto out;

	/* Set data to increasing numbers */
	packet->length = size;
	for (i = 0; i < size; i++)
		packet->data[i] = i;

	/* Try to send frame */
	csp_send(conn, packet);

	/* Read incoming frame */
	packet = csp_read(conn, timeout);
	if (packet == NULL)
		goto out;

	/* Ensure that the data was actually echoed */
	for (i = 0; i < size; i++) {
		if (packet->data[i] != i % (0xff + 1)) {
			goto out;
		}
	}
	status = 1;

out:
	/* Clean up */
	csp_buffer_free(packet);
	csp_close(conn);

	/* We have a reply */
	time = (csp_get_ms() - start);

	if (status) {
		return time;
	}

	return -1;
}

void csp_ping_noreply(uint16_t node) {

	/* Prepare data */
	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL)
		return;

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PING, 0, CSP_O_CRC32);
	if (conn == NULL) {
		csp_buffer_free(packet);
		return;
	}

	packet->data[0] = 0x55;
	packet->length = 1;

	csp_send(conn, packet);
	csp_close(conn);
}

void csp_reboot(uint16_t node) {
	uint32_t magic_word = htobe32(CSP_REBOOT_MAGIC);
	csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_REBOOT, 0, &magic_word, sizeof(magic_word), NULL, 0, CSP_O_CRC32);
}

void csp_shutdown(uint16_t node) {
	uint32_t magic_word = htobe32(CSP_REBOOT_SHUTDOWN_MAGIC);
	csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_REBOOT, 0, &magic_word, sizeof(magic_word), NULL, 0, CSP_O_CRC32);
}

int csp_sync_time(uint16_t node) {

	/* Prepare data */
	csp_packet_t * packet = csp_buffer_get(0);
	if (packet == NULL)
		return -1;

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_TIME_SYNC, 0, CSP_O_CRC32);
	if (conn == NULL) {
		csp_buffer_free(packet);
		return -1;
	}

	csp_time_sync_t time_sync;
	static uint32_t sync_id = 1;
	time_sync.id = htobe32(sync_id++);
	time_sync.tv_sec = 0;
	time_sync.tv_nsec = 0;
	time_sync.correction = 0;
	packet->length = sizeof(time_sync);
	memcpy(packet->data, &time_sync, sizeof(time_sync));

	csp_timestamp_t fallback_ts;
	csp_clock_get_time(&fallback_ts);

	uint64_t tx_timestamp = 0;
	csp_send_and_get_timestamp(conn, packet, &tx_timestamp);
	if (tx_timestamp == 0) {
		/* No timestamp available */
		csp_print("Warning: No TX timestamp available when syncing time to node %u, using local clock as fallback\n", node);
		tx_timestamp = ((uint64_t)fallback_ts.tv_sec * (uint64_t)1E9) + (uint64_t)fallback_ts.tv_nsec;
	}

	packet = csp_buffer_get(0);
	if (packet == NULL)
		return -1;

	time_sync.tv_sec = htobe32(tx_timestamp / (uint64_t)1E9);
	time_sync.tv_nsec = htobe32(tx_timestamp % (uint64_t)1E9);
	time_sync.correction = 1;

	packet->length = sizeof(time_sync);
	memcpy(packet->data, &time_sync, sizeof(time_sync));

	csp_send(conn, packet);

	csp_close(conn);

	return CSP_ERR_NONE;
}

void csp_ps(uint16_t node, uint32_t timeout) {

	/* Open connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_NORM, node, CSP_PS, 0, CSP_O_CRC32);
	if (conn == NULL) {
		return;
	}

	/* Prepare data */
	csp_packet_t * packet = csp_buffer_get(0);

	/* Check malloc */
	if (packet == NULL) {
		goto out;
	}

	packet->data[0] = 0x55;
	packet->length = 1;

	/* Try to send frame */
	csp_send(conn, packet);

	while (1) {

		/* Read incoming frame */
		packet = csp_read(conn, timeout);
		if (packet == NULL) {
			break;
		}

		/* We have a reply, ensure data is 0 (zero) termianted */
		const unsigned int length = (packet->length < sizeof(packet->data)) ? packet->length : (sizeof(packet->data) - 1);
		packet->data[length] = 0;
		csp_print("%s", packet->data);

		/* Each packet from csp_read must to be freed by user */
		csp_buffer_free(packet);
	}

	csp_print("\r\n");

	/* Clean up */
out:
	csp_buffer_free(packet);
	csp_close(conn);
}

int csp_get_memfree(uint16_t node, uint32_t timeout, uint32_t * size) {

	int status = csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_MEMFREE, timeout, NULL, 0, size, sizeof(*size), CSP_O_CRC32);
	if (status == sizeof(*size)) {
		*size = be32toh(*size);
		return CSP_ERR_NONE;
	}
	*size = 0;
	return CSP_ERR_TIMEDOUT;
}

void csp_memfree(uint16_t node, uint32_t timeout) {

	uint32_t memfree;
	int err = csp_get_memfree(node, timeout, &memfree);
	if (err == CSP_ERR_NONE) {
		csp_print("Free Memory at node %u is %" PRIu32 " bytes\r\n", node, memfree);
	} else {
		csp_print("Network error\r\n");
	}
}

int csp_get_buf_free(uint16_t node, uint32_t timeout, uint32_t * size) {

	int status = csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_BUF_FREE, timeout, NULL, 0, size, sizeof(*size), CSP_O_CRC32);
	if (status == sizeof(*size)) {
		*size = be32toh(*size);
		return CSP_ERR_NONE;
	}
	*size = 0;
	return CSP_ERR_TIMEDOUT;
}

void csp_buf_free(uint16_t node, uint32_t timeout) {

	uint32_t size;
	int err = csp_get_buf_free(node, timeout, &size);
	if (err == CSP_ERR_NONE) {
		csp_print("Free buffers at node %u is %" PRIu32 "\r\n", node, size);
	} else {
		csp_print("Network error\r\n");
	}
}

int csp_get_uptime(uint16_t node, uint32_t timeout, uint32_t * uptime) {

	int status = csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_UPTIME, timeout, NULL, 0, uptime, sizeof(*uptime), CSP_O_CRC32);
	if (status == sizeof(*uptime)) {
		*uptime = be32toh(*uptime);
		return CSP_ERR_NONE;
	}
	*uptime = 0;
	return CSP_ERR_TIMEDOUT;
}

void csp_uptime(uint16_t node, uint32_t timeout) {

	uint32_t uptime;
	int err = csp_get_uptime(node, timeout, &uptime);
	if (err == CSP_ERR_NONE) {
		csp_print("Uptime of node %u is %" PRIu32 " s\r\n", node, uptime);
	} else {
		csp_print("Network error\r\n");
	}
}

int csp_cmp(uint16_t node, uint32_t timeout, uint8_t code, int msg_size, struct csp_cmp_message * msg) {
	msg->type = CSP_CMP_REQUEST;
	msg->code = code;
	int status = csp_transaction_w_opts(CSP_PRIO_NORM, node, CSP_CMP, timeout, msg, msg_size, msg, msg_size, CSP_O_CRC32);
	if (status == 0) {
		return CSP_ERR_TIMEDOUT;
	}

	return CSP_ERR_NONE;
}

int csp_cmp_clock_with_rx_timestamp(uint16_t node, uint32_t timeout, struct csp_cmp_message * msg, uint64_t *rx_timestamp) {
	msg->type = CSP_CMP_REQUEST;
	msg->code = CSP_CMP_CLOCK;
	int status = csp_transaction_w_opts_timestamped(CSP_PRIO_NORM, node, CSP_CMP, timeout, msg, CMP_SIZE(clock), msg, CMP_SIZE(clock), CSP_O_CRC32, rx_timestamp);
	if (status == 0) {
		return CSP_ERR_TIMEDOUT;
	}

	return CSP_ERR_NONE;
}
