
#include <csp/csp_types.h>
#include <csp/csp_hooks.h>
#include "csp_macro.h"

__weak void csp_clock_get_time(csp_timestamp_t * time) {
	time->tv_sec = 0;
	time->tv_nsec = 0;
}

__weak int csp_clock_set_time(const csp_timestamp_t * time) {
	(void)time; /* Avoid compiler warnings about unused parameter */
	return CSP_ERR_NOTSUP;
}

__weak int csp_clock_set_time_w_local_time(const csp_timestamp_t * time, uint64_t local_rx_ns) {
	(void)time;
	(void)local_rx_ns;
	return CSP_ERR_NOTSUP;
}

__weak void csp_set_packet_tx_time(const void *packet, uint64_t tx_time_ns) {
	(void)packet;
	(void)tx_time_ns;
}
