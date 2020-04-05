#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"
#include "esp_system.h"
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "arch_os.h"
#include "oled_driver.h"
#include "https_driver.h"
#include "tcp_driver.h"
#include "siot.h"

#undef	TAG
#define TAG		"app_main"

static void mem_monitor_task(void *params)
{
	while(true) {
		ESP_LOGI(TAG, "memory left: %d", esp_get_free_heap_size());
		arch_os_ms_sleep(5000);
	}
}

static void https_get_request_task(void *params)
{
	https_request_by_get(params);
	vTaskDelete(NULL);
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	siot_t *siot = (siot_t *)ctx;
	tcp_t *tcp = siot->tcp;

	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		ESP_LOGI(TAG, "SYSTEM_EVENT_STA_START");
		break;
	
	case SYSTEM_EVENT_STA_GOT_IP:
		ESP_LOGI("event_handler", "SYSTEM_EVENT_STA_GOT_IP");
		tcp_set_ctrl_msg(tcp, TCP_CTRL_OPEN);
		break;

	case SYSTEM_EVENT_STA_CONNECTED:
		ESP_LOGI("event_handler", "SYSTEM_EVENT_STA_CONNECTED");
		break;

	case SYSTEM_EVENT_STA_DISCONNECTED:
		/* This is a workaround as ESP32 WiFi libs don't currently auto-reassociate. */
		esp_wifi_connect();
		break;

	default:
		break;
	}
	return ESP_OK;
}


static void user_wifi_init(void *argv)
{
	ESP_ERROR_CHECK(nvs_flash_init());
	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, argv));
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
		.sta = {
			.ssid = CONFIG_USER_WIFI_SSID,
			.password = CONFIG_USER_WIFI_PASSWD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{

	siot_t *siot = NULL;
	siot_instance_create(&siot, NULL);

	/* create oled instance */
	oled_t *oled = NULL;
	oled_cfg_t oled_cfg = OLED_INIT_DEFAULT;
	oled_instance_create(&oled, &oled_cfg);
	siot->oled = oled;

	/* create tcp instance */
	tcp_t *tcp = NULL;
	tcp_config_t tcp_config = TCP_CONFIG_DEFAULT_VALUE;
	tcp_instance_create(siot, &tcp, &tcp_config);

	user_wifi_init(siot);

	xTaskCreate(mem_monitor_task, "mem_monitor_task", 1024 * 2, NULL, 4, NULL);
}
