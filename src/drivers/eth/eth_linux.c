#include <stdint.h>
#include <errno.h>

#include <csp/csp.h>
#include <csp/csp_id.h>
#include <csp/csp_interface.h>
#include <csp/interfaces/csp_if_eth.h>
#include <csp/interfaces/csp_if_eth_pbuf.h>

#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <pthread.h>

/* (netinet/ether.h) protocol setting used for promiscuous mode */
#define ETH_P_ALL	0x0003		/* Every packet (be careful!!!) */

extern bool eth_debug;

typedef struct {
	char name[CSP_IFLIST_NAME_MAX + 1];
	csp_eth_interface_data_t ifdata;
    int sockfd;
    struct ifreq if_idx;
} eth_context_t;

int csp_eth_tx_frame(void * driver_data, csp_eth_header_t *eth_frame, uint64_t * timestamp) {

    const eth_context_t * ctx = (eth_context_t*)driver_data;

    /* Destination socket address */
    struct sockaddr_ll socket_address = {};
    socket_address.sll_ifindex = ctx->if_idx.ifr_ifindex;
    socket_address.sll_halen = CSP_ETH_ALEN;
    memcpy(socket_address.sll_addr, eth_frame->ether_dhost, CSP_ETH_ALEN);

    uint32_t txsize = sizeof(csp_eth_header_t) + be16toh(eth_frame->seg_size);

    /* Setup sendmsg for TX timestamping */
    struct iovec iov = {
        .iov_base = eth_frame,
        .iov_len = txsize
    };

    struct msghdr msg = {
        .msg_name = &socket_address,
        .msg_namelen = sizeof(struct sockaddr_ll),
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = NULL,
        .msg_controllen = 0,
        .msg_flags = 0
    };

    if (sendmsg(ctx->sockfd, &msg, 0) < 0) {
        return CSP_ERR_DRIVER;
    }

    /* Retrieve TX timestamp from error queue */
    char control[CMSG_SPACE(sizeof(struct scm_timestamping))];
    char data[CSP_ETH_BUF_SIZE];
    struct iovec iov_err = {
        .iov_base = data,
        .iov_len = sizeof(data)
    };

    struct msghdr msg_err = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &iov_err,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = sizeof(control),
        .msg_flags = 0
    };

    /* Try to get TX timestamp (non-blocking) */
    int err_result = recvmsg(ctx->sockfd, &msg_err, MSG_ERRQUEUE | MSG_DONTWAIT);
    if (err_result >= 0) {
        for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg_err); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg_err, cmsg)) {
            if (eth_debug) {
                csp_print("TX cmsg: level=%d type=%d\n", cmsg->cmsg_level, cmsg->cmsg_type);
            }
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING) {
                struct scm_timestamping *ts_data = (struct scm_timestamping *)CMSG_DATA(cmsg);
                uint64_t tx_timestamp = 0;
                /* Try hardware timestamp first (ts[2]), then software (ts[0]) */
                tx_timestamp = (uint64_t)ts_data->ts[0].tv_sec * 1000000000ULL + (uint64_t)ts_data->ts[0].tv_nsec;
                if (eth_debug) {
                    csp_print("TX SW timestamp: %llu ns\n", tx_timestamp);
                }
                if (timestamp && tx_timestamp) {
                    *timestamp = tx_timestamp;
                }
                break;
            }
        }
    } else if (eth_debug) {
        csp_print("TX: No timestamp from error queue (errno=%d)\n", errno);
    }

    return CSP_ERR_NONE;
}

void * csp_eth_rx_loop(void * param) {

    eth_context_t * ctx = param;

    static uint8_t recvbuf[CSP_ETH_BUF_SIZE];
    csp_eth_header_t * eth_frame = (csp_eth_header_t *)recvbuf;

    uint64_t timestamp = 0;

    /* Setup for recvmsg to receive timestamp */
    struct iovec iov = {
        .iov_base = recvbuf,
        .iov_len = CSP_ETH_BUF_SIZE
    };
    
    char control[CMSG_SPACE(sizeof(struct scm_timestamping))];
    
    struct msghdr msg = {
        .msg_name = NULL,
        .msg_namelen = 0,
        .msg_iov = &iov,
        .msg_iovlen = 1,
        .msg_control = control,
        .msg_controllen = sizeof(control),
        .msg_flags = 0
    };

    while(1) {

        /* Receive packet segment with timestamp */ 
        int32_t received_len = recvmsg(ctx->sockfd, &msg, 0);
        
        if (received_len < 0) {
            continue;
        }

        /* Extract timestamp from control message */
        timestamp = 0;
        for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
            if (eth_debug) {
                csp_print("RX cmsg: level=%d type=%d\n", cmsg->cmsg_level, cmsg->cmsg_type);
            }
            if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMPING) {
                struct scm_timestamping *ts_data = (struct scm_timestamping *)CMSG_DATA(cmsg);
                /* Try hardware timestamp first (ts[2]), then software (ts[0]) */
                if (ts_data->ts[0].tv_sec || ts_data->ts[0].tv_nsec) {
                    timestamp = (uint64_t)ts_data->ts[0].tv_sec * 1000000000ULL + (uint64_t)ts_data->ts[0].tv_nsec;
                    if (eth_debug) {
                        csp_print("RX SW timestamp: %llu ns\n", timestamp);
                    }
                }
                break;
            }
        }
        
        if (eth_debug && timestamp == 0) {
            csp_print("RX: No timestamp received\n");
        }

        csp_eth_rx(&ctx->ifdata.iface, eth_frame, received_len, NULL, timestamp);
    }

    return NULL;
}

static uint8_t csp_eth_tx_buffer[CSP_ETH_BUF_SIZE];

int csp_eth_init(const char * device, const char * ifname, int mtu, unsigned int node_id, bool promisc, csp_iface_t ** return_iface) {

	eth_context_t * ctx = calloc(1, sizeof(*ctx));
	if (ctx == NULL) {
		return CSP_ERR_NOMEM;
	}
	
	strcpy(ctx->name, ifname);
	ctx->ifdata.iface.name = ctx->name;
    ctx->ifdata.tx_func = &csp_eth_tx_frame;
    ctx->ifdata.tx_buf = (csp_eth_header_t*)&csp_eth_tx_buffer;
    ctx->ifdata.iface.nexthop = &csp_eth_tx,
	ctx->ifdata.iface.addr = node_id;
	ctx->ifdata.iface.driver_data = ctx;
    ctx->ifdata.iface.interface_data = &ctx->ifdata;
    ctx->ifdata.promisc = promisc;

    /* Ether header 14 byte, seg header 4 byte, CSP header 6 byte */
    if (mtu < 24) {
        csp_print("csp_if_eth_init: mtu < 24\n");
        return CSP_ERR_INVAL;
    }


    /**
     * TX SOCKET
     */

    /* Open RAW socket to send on */
    if ((ctx->sockfd = socket(AF_PACKET, SOCK_RAW, htobe16(CSP_ETH_TYPE_CSP))) == -1) {
        perror("socket");
        char exe[1024];
        int count = readlink("/proc/self/exe", exe, sizeof(exe));
        if (count > 0) {
            csp_print("Use command 'sudo setcap cap_net_raw+ep %s'\n", exe);
        }
        return CSP_ERR_INVAL;
    }

    /* Get the index of the interface to send on */
    memset(&ctx->if_idx, 0, sizeof(struct ifreq));
    strncpy(ctx->if_idx.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(ctx->sockfd, SIOCGIFINDEX, &ctx->if_idx) < 0) {
        perror("SIOCGIFINDEX");
        return CSP_ERR_INVAL;
    }

    struct ifreq if_mac;
    /* Get the MAC address of the interface to send on */
    memset(&if_mac, 0, sizeof(struct ifreq));
    strncpy(if_mac.ifr_name, device, IFNAMSIZ-1);
    if (ioctl(ctx->sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
        perror("SIOCGIFHWADDR");
        return CSP_ERR_INVAL;
    }

    memcpy(&ctx->ifdata.if_mac, if_mac.ifr_hwaddr.sa_data, sizeof(ctx->ifdata.if_mac));

    csp_print("INIT %s %s idx %d node %d mac %02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx\n", 
        ifname, device, ctx->if_idx.ifr_ifindex, node_id,
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[0],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[1],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[2],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[3],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[4],
        ((uint8_t *)if_mac.ifr_hwaddr.sa_data)[5]);

    /* Allow the socket to be reused - incase connection is closed prematurely */
    int sockopt;
    if (setsockopt(ctx->sockfd, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof sockopt) == -1) {
        perror("setsockopt");
        close(ctx->sockfd);
        return CSP_ERR_INVAL;
    }

    // Enable TX timestamping
    int ts_flags = SOF_TIMESTAMPING_TX_SOFTWARE | SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    if (setsockopt(ctx->sockfd, SOL_SOCKET, SO_TIMESTAMPING, &ts_flags, sizeof(ts_flags)) == -1) {
        perror("setsockopt SO_TIMESTAMPING");
    } else {
        csp_print("Using hardware timestamping on %s\n", device);
    }

    /* Bind to device */
    if (setsockopt(ctx->sockfd, SOL_SOCKET, SO_BINDTODEVICE, device, IFNAMSIZ-1) == -1)	{
        perror("SO_BINDTODEVICE");
        close(ctx->sockfd);
        return CSP_ERR_INVAL;
    }

    /* fill sockaddr_ll struct to prepare binding */
    struct sockaddr_ll my_addr;
    my_addr.sll_family = AF_PACKET;
    my_addr.sll_protocol = htobe16(CSP_ETH_TYPE_CSP);
    my_addr.sll_ifindex = ctx->if_idx.ifr_ifindex;

    /* bind socket  */
    bind(ctx->sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr_ll));

    ctx->ifdata.tx_mtu = mtu;

    /* Start server thread */
    static pthread_t server_handle;
    pthread_create(&server_handle, NULL, &csp_eth_rx_loop, ctx);

    /**
     * CSP INTERFACE
     */

    /* Register interface */
    csp_iflist_add(&ctx->ifdata.iface);

	if (return_iface) {
		*return_iface = &ctx->ifdata.iface;
	}

    return CSP_ERR_NONE;}
