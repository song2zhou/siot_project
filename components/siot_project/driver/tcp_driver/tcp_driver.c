#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "cJSON.h"

#include "arch_os.h"
#include "siot_define.h"
#include "oled_driver.h" /* oled_set_show_task() */
#include "tcp_driver.h"

#undef TAG
#define TAG		"tcp_driver"

static void tcp_idle(void *argv)
{
	ESP_LOGI(TAG, "tcp idle...");
	siot_t *siot = (siot_t *)argv;
	tcp_t *tcp = siot->tcp;

	tcp_ctrl_msg_t tcp_ctrl_msg;
	arch_os_queue_recv(tcp->ctrl_msg_q, (void *)&tcp_ctrl_msg, ARCH_OS_WAIT_FOREVER);

	if(tcp_ctrl_msg == TCP_CTRL_OPEN) {
		tcp->state = TCP_STATE_CONNECTING;
	} else {
		tcp->state = TCP_STATE_IDLE;
	}
}

static void tcp_connecting(void *argv)
{
	ESP_LOGI(TAG, "tcp connecting...");
	siot_t *siot = (siot_t *)argv;
	tcp_t *tcp = siot->tcp;

	struct sockaddr_in listen_addr;
	listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	listen_addr.sin_port = htons(tcp->addr_info.listen_addr_info.listen_port);
	listen_addr.sin_family = AF_INET;

	tcp->listen_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (tcp->listen_sockfd < 0) {
		ESP_LOGE(TAG, "create sockfd failed: errno %d", errno);
		tcp->state = TCP_STATE_IDLE;
		return;
	}
	ESP_LOGI(TAG, "socket create success");

	int flag = 1;
	int flag_len = sizeof(int);
	/* set sockfd reusable, or it cannot be binded next time */
	int err = setsockopt(tcp->listen_sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, flag_len);
	if (err != 0) {
		tcp->listen_sockfd = -1;
		tcp->state = TCP_STATE_IDLE;
		ESP_LOGE(TAG, "set sockopt failed: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "socket set opt success");

	err = bind(tcp->listen_sockfd, (struct sockaddr *)&listen_addr, sizeof(listen_addr));
	if (err != 0) {
		tcp->listen_sockfd = -1;
		tcp->state = TCP_STATE_IDLE;
		ESP_LOGE(TAG, "socket bind failed: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "socket bind success");

	err = listen(tcp->listen_sockfd, 1);
	if (err != 0) {
		tcp->listen_sockfd = -1;
		tcp->state = TCP_STATE_IDLE;
		ESP_LOGE(TAG, "socket listen failed: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "socket listening...");

	struct sockaddr_in6 peer_addr; // Large enough for both IPv4 or IPv6
	uint addr_len = sizeof(peer_addr);
	tcp->peer_sockfd = accept(tcp->listen_sockfd, (struct sockaddr *)&peer_addr, &addr_len);
	if (tcp->peer_sockfd < 0) {
		tcp->state = TCP_STATE_IDLE;
		ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
		return;
	}
	ESP_LOGI(TAG, "socket connected");
	tcp->state = TCP_STATE_TOUCHING;
}

static void tcp_touching(void *argv)
{
	ESP_LOGI(TAG, "tcp touching...");
	siot_t *siot = (siot_t *)argv;
	tcp_t *tcp = siot->tcp;
	oled_t *oled = siot->oled;

	while(true) {
		int recv_len = recv(tcp->peer_sockfd, tcp->recv_buf, TCP_RECV_BUF_LEN - 1, 0);

		if (recv_len < 0) {
			ESP_LOGE(TAG, "recv failed: errno %d", errno);
			break;
		} else if(recv_len == 0) {
			ESP_LOGI(TAG, "socket closed");
			break;
		} else { 
			tcp->recv_buf[recv_len] = 0; // Null-terminate whatever we received and treat like a string
			ESP_LOGI(TAG, "recv %u bytes: %s", recv_len, tcp->recv_buf);

			do { /* parse json string, and set oled show task */
				cJSON *json_parse = cJSON_Parse((const char *)(strchr(tcp->recv_buf, '{')));
				if(NULL == json_parse) { continue; }

				cJSON *light = cJSON_GetObjectItem(json_parse, "light");
				if(NULL == light) { continue; }

				cJSON *button = cJSON_GetObjectItem(light, "switch");
				if(NULL == button) { continue;}

				if(button->valueint) {
					oled_set_show_task(oled, 16, 0, 1, (const uint8_t *)"on ", strlen("on "));
				} else {
					oled_set_show_task(oled, 16, 0, 1, (const uint8_t *)"off", strlen("off"));
				}
			} while (false);
			
		}
	}

	tcp->state = TCP_STATE_DISCONNECTING;
}

static void tcp_disconnecting(void *argv)
{
	ESP_LOGI(TAG, "tcp disconnecting...");
	siot_t *siot = (siot_t *)argv;
	tcp_t *tcp = siot->tcp;

	/* close current sockfd */
	shutdown(tcp->listen_sockfd, 2);
	shutdown(tcp->peer_sockfd, 2);
	close(tcp->listen_sockfd);
	close(tcp->peer_sockfd);
	tcp->state = TCP_STATE_IDLE;

	/* reopen for next time */
	tcp_set_ctrl_msg(tcp, TCP_CTRL_OPEN);
}


static arch_os_function_return_t tcp_task(void *argv)
{
	siot_t *siot = (siot_t *)argv;
	tcp_t *tcp = siot->tcp;

	while(tcp->state != TCP_STATE_DEAD) {

	switch(tcp->state){
		case TCP_STATE_IDLE:
			tcp_idle(siot);
			break;
		case TCP_STATE_CONNECTING:
			tcp_connecting(siot);
			break;
		case TCP_STATE_TOUCHING:
			tcp_touching(siot);
			break;
		case TCP_STATE_DISCONNECTING:
			tcp_disconnecting(siot);
			break;
		default:
			tcp->state = TCP_STATE_IDLE;
			break;
		}
	}

	ESP_LOGI(TAG, "tcp task exit");
	vTaskDelete(NULL);

	return NULL;
}

siot_err_t tcp_set_ctrl_msg(tcp_t *tcp, tcp_ctrl_msg_t ctrl_msg)
{
	if(NULL == tcp) {
		ESP_LOGE(TAG, "tcp is null");
		return SIOT_ERR_PARAM;
	}

	arch_os_queue_send(tcp->ctrl_msg_q, (void *)&ctrl_msg, 0);
	return SIOT_OK;
}


siot_err_t tcp_instance_create(siot_t *siot, tcp_t **tcp, tcp_config_t *config)
{
	if (NULL == config) { return SIOT_ERR_PARAM; }

	*tcp = (tcp_t *)malloc(sizeof(tcp_t));
	(*tcp)->addr_info.listen_addr_info.listen_port = config->listen_port;
	(*tcp)->state = config->state;
	memset((*tcp)->recv_buf, 0, TCP_RECV_BUF_LEN);
	(*tcp)->pthread = NULL;
	arch_os_queue_create(&(*tcp)->ctrl_msg_q, TCP_CTRL_MSG_QUEUE_LEN, sizeof(tcp_ctrl_msg_t));

	siot->tcp = *tcp;

	arch_os_thread_create(&(*tcp)->pthread, "tcp_task", tcp_task, 4 * 1024, siot, 2);
	return SIOT_OK;
}

