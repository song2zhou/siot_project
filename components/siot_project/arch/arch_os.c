#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"

#include "siot_define.h"
#include "arch_os.h"

#undef TAG
#define TAG "arch_os"

/*************************** TIME RELATED ***************************/
#define ARCH_OS_MS2TICK(ms)      ((ms)/portTICK_PERIOD_MS)
#define ARCH_OS_TICK2MS(tick)    ((tick)*portTICK_PERIOD_MS)

static uint32_t tick_high = 0;
static bool tick_half_flag = false;
static uint32_t arch_os_tick_now( void )
{
	uint32_t tick = xTaskGetTickCount();

	if(tick > 0x7FFFFFFF) { /* set half flag if tick over 0x7FFFFFFF */
		tick_half_flag = true;
	} else if (true == tick_half_flag) { /* add tick_high if overflow */
		tick_half_flag = false;
		tick_high++;
	}

	return tick;
}

uint32_t arch_os_ms_now( void )
{
    return (ARCH_OS_TICK2MS(arch_os_tick_now()));
}

void  arch_os_ms_sleep( uint32_t ms )
{
	vTaskDelay(ARCH_OS_MS2TICK(ms));
}

static uint32_t arch_os_tick_elapsed(uint32_t tick)
{
	return (arch_os_tick_now() - tick);
}

uint32_t arch_os_ms_elapsed(uint32_t last_ms)
{
    return (ARCH_OS_TICK2MS(arch_os_tick_elapsed(ARCH_OS_MS2TICK(last_ms))));
}

/************************** THREAD RELATED **************************/
int arch_os_thread_create(arch_os_thread_handle_t* pthread, const char* name, arch_os_function_return_t (*function)(void* arg), uint32_t stack_size, void* arg, int priority )
{
	if( pdPASS == xTaskCreate( (pdTASK_CODE)function, (const char* const)name, (unsigned short) (stack_size/sizeof( portSTACK_TYPE )), arg, priority, pthread ) ){
		ESP_LOGI(TAG, "create handle = %x, name = %s, prio = %d", (uint32_t)(*pthread), name, priority);
		return SIOT_OK;
	}

	ESP_LOGE(TAG, "create failed, name = %s, prio = %d", name, priority);
	return SIOT_ERR;
}


int arch_os_thread_delete(arch_os_thread_handle_t thread)
{
	if(NULL == thread) {
		ESP_LOGI(TAG, "delete handle = %x", (uint32_t)xTaskGetCurrentTaskHandle());
	} else {
		ESP_LOGI(TAG, "delete handle = %x", (uint32_t)thread);
	}

	vTaskDelete(thread);
	return SIOT_OK;
}

/************************** MUTEX RELATED **************************/
int arch_os_mutex_create(arch_os_mutex_handle_t* phandle)
{
	*phandle = xSemaphoreCreateMutex();
	if(*phandle) {
		return SIOT_OK;
	} else{
		return SIOT_ERR;
	}
}

int arch_os_mutex_delete(arch_os_mutex_handle_t handle)
{
	vSemaphoreDelete(handle);
	return SIOT_OK;
}

int arch_os_mutex_put(arch_os_mutex_handle_t handle)
{
	int ret;
	if (xPortInIsrContext()) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		ret = xSemaphoreGiveFromISR(handle, &xHigherPriorityTaskWoken);
	} else {
		ret = xSemaphoreGive(handle);
	}

	return ret == pdTRUE ? SIOT_OK : SIOT_ERR;
}

int arch_os_mutex_get(arch_os_mutex_handle_t handle, uint32_t wait_ms)
{
	int ret;
	if (xPortInIsrContext()) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		ret = xSemaphoreTakeFromISR(handle, &xHigherPriorityTaskWoken);
	} else {
		ret = xSemaphoreTake(handle, ARCH_OS_MS2TICK(wait_ms));
	}

	return ret == pdTRUE ? SIOT_OK : SIOT_ERR;
}

/************************** QUEUE RELATED **************************/
int arch_os_queue_create(arch_os_queue_handle_t *pqhandle, uint32_t q_len, uint32_t item_size)
{
	*pqhandle = xQueueCreate(q_len, item_size);
	if(*pqhandle){
		return SIOT_OK;
	} else {
		return SIOT_ERR;
	}
}

int arch_os_queue_delete(arch_os_queue_handle_t qhandle)
{
	vQueueDelete(qhandle);
	return SIOT_OK;
}

int arch_os_queue_send(arch_os_queue_handle_t qhandle, const void *msg, uint32_t wait_ms)
{
	int ret;
	if (xPortInIsrContext()) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		ret = xQueueSendToBackFromISR(qhandle, msg, &xHigherPriorityTaskWoken);
	} else {
		ret = xQueueSendToBack(qhandle, msg, ARCH_OS_MS2TICK(wait_ms));
	}

	return ret == pdTRUE ? SIOT_OK : SIOT_ERR;
}

int arch_os_queue_recv(arch_os_queue_handle_t qhandle, void *msg, uint32_t wait_ms)
{
	int ret;
	if (xPortInIsrContext()) {
		portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
		ret = xQueueReceiveFromISR(qhandle, msg, &xHigherPriorityTaskWoken);
	} else {
		ret = xQueueReceive(qhandle, msg, ARCH_OS_MS2TICK(wait_ms));
	}

	return ret == pdTRUE ? SIOT_OK : SIOT_ERR;
}

/************************** HARDWARE RELATED **************************/
void arch_os_reboot(void)
{
	esp_restart();
}

uint32_t arch_os_get_free_heap_size()
{
	return esp_get_free_heap_size();
}


