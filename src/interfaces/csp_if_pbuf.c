#include "csp_if_pbuf.h"

#include <string.h>

#include <csp/csp_buffer.h>
#include <csp/csp_error.h>
#include <csp/arch/csp_time.h>
#include "csp/csp_types.h"

/* Buffer element timeout in ms */
#define PBUF_TIMEOUT_MS 1000

void csp_pbuf_free(csp_packet_t ** pbufs, csp_packet_t * buffer, int buf_free, int * task_woken) {

	csp_packet_t * packet = *pbufs;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if (packet == buffer) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				*pbufs = packet->next;
			}

			if (buf_free) {
				if (task_woken == NULL) {
					csp_buffer_free(packet);
				} else {
					csp_buffer_free_isr(packet);
				}
			}

		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_pbuf_new_always(csp_packet_t ** pbufs, uint32_t id, int * task_woken) {

	csp_pbuf_cleanup(pbufs, task_woken);

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = (task_woken) ? csp_buffer_get_always_isr() : csp_buffer_get_always();

	packet->last_used = now;
	packet->cfpid = id;
	packet->remain = 0;

	/* Insert at beginning, because easy */
	packet->next = *pbufs;
	*pbufs = packet;

	return packet;
}

csp_packet_t * csp_pbuf_new(csp_packet_t ** pbufs, uint32_t id, int * task_woken) {

	csp_pbuf_cleanup(pbufs, task_woken);

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = (task_woken) ? csp_buffer_get_isr(0) : csp_buffer_get(0);

	packet->last_used = now;
	packet->cfpid = id;
	packet->remain = 0;

	/* Insert at beginning, because easy */
	packet->next = *pbufs;
	*pbufs = packet;

	return packet;
}

void csp_pbuf_cleanup(csp_packet_t ** pbufs, int * task_woken) {

	uint32_t now = (task_woken) ? csp_get_ms_isr() : csp_get_ms();

	csp_packet_t * packet = *pbufs;
	csp_packet_t * prev = NULL;

	while (packet) {

		/* Perform cleanup in used pbufs */
		if (now - packet->last_used > PBUF_TIMEOUT_MS) {

			/* Erase from list prev->next = next */
			if (prev) {
				prev->next = packet->next;
			} else {
				*pbufs = packet->next;
			}

			if (task_woken == NULL) {
				csp_buffer_free(packet);
			} else {
				csp_buffer_free_isr(packet);
			}

		}

		prev = packet;
		packet = packet->next;
	}

}

csp_packet_t * csp_pbuf_find(csp_packet_t ** pbufs, uint32_t id, int * task_woken) {

	csp_packet_t * packet = *pbufs;
	while (packet) {

		if (packet->cfpid == id) {
			packet->last_used = (task_woken) ? csp_get_ms_isr() : csp_get_ms();
			return packet;
		}
		packet = packet->next;
	}

	return NULL;

}
