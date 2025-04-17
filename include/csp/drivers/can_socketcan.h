/****************************************************************************
 * **File:** csp/drivers/csp_socketcan.h
 *
 * **Description:** Socket CAN driver (Linux).
 *
 * .. note:: This driver requires the libsocketcan library.
 ****************************************************************************/
#pragma once

#include <csp/interfaces/csp_if_can.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open CAN socket and add CSP interface.
 *
 * Parameters:
 * @param[in] device CAN device name (Linux device).
 * @param[in] ifname CSP interface name, use #CSP_IF_CAN_DEFAULT_NAME for default name.
 * @param[in] node_id CSP address of the interface.
 * @param[in] netmask Netmask size of the interface.
 * @param[in] broadcast_size Number of broadcast nodes accepted on the subnet.
 * @param[in] bitrate if different from 0, it will be attempted to change the
 *            bitrate on the CAN device - this may require increased OS privileges.
 * @param[in] promisc if true, receive all CAN frames. If false a filter
 *                    is set on the CAN device, using device->addr
 * @param[out] return_iface the added interface.
 * @return The added interface, or NULL in case of failure.
 */
int csp_can_socketcan_open_and_add_interface(const char * device, const char * ifname, unsigned int node_id, unsigned int mask, unsigned int broadcast_size, int bitrate, bool promisc, csp_iface_t ** return_iface);

/**
 * Stop the Rx thread and free resources (testing).
 *
 * .. note:: This will invalidate CSP, because an interface can't be removed.
 *			 This is primarily for testing.
 *
 * Parameters:
 * @param[in] iface interface to stop.
 * @return #CSP_ERR_NONE on success, otherwise an error code.
 */
int csp_can_socketcan_stop(csp_iface_t * iface);

#ifdef __cplusplus
}
#endif
