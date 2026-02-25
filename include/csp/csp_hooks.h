/****************************************************************************
 * **File:** csp/csp_hooks.h
 *
 * **Description:** Hooks that can be implemented in CSP, see Hooks in csp for more information
 ****************************************************************************/
#pragma once

#include <csp/csp_types.h>
#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Hook called when a packet is sent
 * 
 * @param idout     ID of the recipient
 * @param packet    CSP packet to be sent
 * @param iface     Outgoing interface
 * @param via       Next hop address (if applicable)
 * @param from_me   Whether the packet originates from this node
 */
void csp_output_hook(const csp_id_t * idout, csp_packet_t * packet, csp_iface_t * iface, uint16_t via, int from_me);

/**
 * Hook called when a packet is received
 * 
 * @param iface     Interface that received the packet
 * @param packet    Received packet
 */
void csp_input_hook(csp_iface_t * iface, csp_packet_t * packet);

/**
 * Hook called for system reboot
 */
void csp_reboot_hook(void);

/**
 * Hook called for system shutdown
 */
void csp_shutdown_hook(void);

/**
 * Returns the available free memory
 * @return Free memory in bytes
 */
uint32_t csp_memfree_hook(void);

/**
 * Collects process information into a packet
 * 
 * @param packet    Packet to be filled with process info
 * @return          Number of entries written
 */
unsigned int csp_ps_hook(csp_packet_t * packet);

/**
 * Called in case of a fatal error
 * This function must not return, and should reboot the system
 * or the program running CSP to recover responsiveness of the system.
 * 
 * @param msg       Error message
 */
void csp_panic(const char * msg);

/**
 * Decrypt a message (Implement these if you used csp_if_tun)
 * 
 * @param ciphertext_in  Encrypted input data
 * @param ciphertext_len Length of encrypted data
 * @param msg_out        Output buffer for the decrypted message
 * @return               Length of the decrypted data or an error code on failure
 */
int csp_crypto_decrypt(uint8_t * ciphertext_in, uint8_t ciphertext_len, uint8_t * msg_out);

/**
 * Encrypt a message (Implement these if you used csp_if_tun)
 * 
 * @param msg_begin      Plaintext message to encrypt
 * @param msg_len        Length of the message
 * @param ciphertext_out Output buffer for encrypted data
 * @return               Length of the encrypted data or an error code on failure
 */
int csp_crypto_encrypt(uint8_t * msg_begin, uint8_t msg_len, uint8_t * ciphertext_out);

/**
 * Get the current system time
 * 
 * @param time   Structure to be filled with the current time
 */
void csp_clock_get_time(csp_timestamp_t * time);

/**
 * Set the system time
 * 
 * @param time   Structure containing the new time to set
 * @return       0 on success, -1 on failure
 */
int csp_clock_set_time(const csp_timestamp_t * time);

/**
 * Set the system time with local time
 *
 * @param time          Structure containing the new time to set
 * @param local_rx_ns   Local time when time occurred
 * @return              0 on success, -1 on failure
 */
int csp_clock_set_time_w_local_time(const csp_timestamp_t * time, uint64_t local_rx_ns);

/**
 * Is called with local timestamp of when the packet was send.
 *
 * The packet has been freed when this function is called and the packet
 * pointer can only be used for address comparison of the packet send.
 *
 * @param packet      Pointer to the packet that was send
 * @param tx_time_ns  The local time the packet was send
 */
void csp_set_packet_tx_time(const void *packet, uint64_t tx_time_ns);

#ifdef __cplusplus
}
#endif
