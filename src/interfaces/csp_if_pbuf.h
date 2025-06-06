#pragma once

#include <csp/csp_types.h>

void csp_pbuf_free(csp_packet_t ** pbufs, csp_packet_t * buffer, int buf_free, int * task_woken);
csp_packet_t * csp_pbuf_new(csp_packet_t ** pbufs, uint32_t id, int * task_woken);
csp_packet_t * csp_pbuf_new_always(csp_packet_t ** pbufs, uint32_t id, int * task_woken);
csp_packet_t * csp_pbuf_find(csp_packet_t ** pbufs, uint32_t id, int * task_woken);
void csp_pbuf_cleanup(csp_packet_t ** pbufs, int * task_woken);
