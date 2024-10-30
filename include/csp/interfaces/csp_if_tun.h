/****************************************************************************
 * **File:** csp/interfaces/csp_if_tun.h
 *
 * **Description:** Tunnel interface.
 ****************************************************************************/
#pragma once

#include <csp/csp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	/* Should be set before calling if_tun_init */
	uint16_t tun_src;
	uint16_t tun_dst;
} csp_if_tun_conf_t;

void csp_if_tun_init(csp_iface_t * iface, csp_if_tun_conf_t * ifconf);

#ifdef __cplusplus
}
#endif
