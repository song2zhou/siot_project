#ifndef __TCP_DRIVER_H__
#define __TCP_DRIVER_H__

#include "siot_define.h"
#include "arch_os.h"
#include "siot.h"

#define TCP_TASK_STACK_SIZE						4096
#define ADDR_MAX_LEN							128
#define TCP_RECV_BUF_LEN						256

typedef enum {
	TCP_STATE_DEAD = 0,
	TCP_STATE_IDLE,
	TCP_STATE_CONNECTING,
	TCP_STATE_TOUCHING,
	TCP_STATE_DISCONNECTING
}tcp_state_t;

#define TCP_CTRL_MSG_QUEUE_LEN			4
typedef enum {
	TCP_CTRL_OPEN = 0,
	TCP_CTRL_CLOSE,
	TCP_CTRL_EXIT,
}tcp_ctrl_msg_t;

typedef struct _channel_over_tcp_t{
	arch_os_thread_handle_t pthread;
	char recv_buf[TCP_RECV_BUF_LEN];
	tcp_state_t state;
	int listen_sockfd;
	int peer_sockfd;

	struct addr_info {
		struct {
			char listen_addr[ADDR_MAX_LEN];
			uint32_t listen_port;
		}listen_addr_info;

		struct {
			char peer_addr[ADDR_MAX_LEN];
			uint32_t peer_port;
		}peer_addr_info;
	}addr_info;

	QueueHandle_t ctrl_msg_q;
}tcp_t;

#define TCP_CONFIG_DEFAULT_VALUE		{ .state = TCP_STATE_IDLE, .listen_port = 12345 }
typedef struct _channel_over_tcp_config_t{
	tcp_state_t state;
	uint32_t listen_port;
}tcp_config_t;

int tcp_instance_create(siot_t *siot, tcp_t **tcp, tcp_config_t *config);
int tcp_trans_req(tcp_t *tcp, const unsigned char *pbuf, uint32_t len);
siot_err_t tcp_set_ctrl_msg(tcp_t *tcp, tcp_ctrl_msg_t ctrl_msg);
void tcp_instance_destroy(tcp_t* tcp);

#endif /* __TCP_DRIVER_H__ */
