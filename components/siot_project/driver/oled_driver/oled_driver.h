#ifndef __OLED_DRIVER_H__
#define __OLED_DRIVER_H__

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"
#include "arch_os.h"
#include "siot_define.h"

/* set oled hardware parameters */
#define OLED_COL_MAX				128
#define OLED_PAGE_MAX				8

/* oled module i2c address */
#define OLED_ADDR					0x78
/* command to set oled display on/off */
#define OLED_DIS_OFF				0xAE
#define OLED_DIS_ON					0xAF
/* value: 0b 001x xxxx \n */
/* 0x40-0x7F means start display on 0-63 line */
#define OLED_DIS_START_LINE			0x40
/* value: 0b 1011 xxxx \n */
/* set start display page address, value: 0xB0-0xB8 \n */
/* and page 0xB8 won't be display */
#define OLED_DIS_START_PAGE			0xB0
/* set display scan direction */
#define OLED_SCAN_COL_DIR_L2R		0xA1	/* scan colume from left to right */
#define OLED_SCAN_COL_DIR_R2L		0xA0	/* scan colume from right to left */
#define OLED_SCAN_LINE_DIR_T2B		0xC0	/* scan line from top to bottom */
#define OLED_SCAN_LINE_DIR_B2T		0xC8	/* scan line from bottom to top */

/* set display frequence and divide ratio \n */
/*     the following paramter will be (0b xxxx yyyy) \n */
/*     (xxxx) means frequence, (yyyy) means divide ratio \n */
/* you can use NUMS_2_FREQ_DIV to assembly this parameter */
#define OLED_SET_FREQ_AND_DIV		0xD5

/* set display parameters */
#define OLED_DIS_NORNAL_1			0xA4
#define OLED_DIS_ALL				0xA5

#define OLED_DIS_NORMAL_2			0xA6
#define OLED_DIS_REVERSE			0xA7

/* set memory addressing parameters */
#define OLED_MEM_ADDR_MODE			0x20
#define OLED_HORIZION_ADDR_MODE		0x00
#define OLED_VERTICAL_ADDR_MODE		0x01
#define OLED_PAGE_ADDR_MODE			0x10
#define OLED_PAGE_START_ADDR		0xB0


#define NUM_2_COL_HIGH_ADDR(n)		(((n) < 128 && (n) >= 0) ? (0x01 | ((n) >> 4)) : 0x10)
#define NUM_2_COL_LOW_ADDR(n)		(((n) < 128 && (n) >= 0) ? (0x00 | ((n) & 0x0F)) : 0x00)
#define NUMS_2_FREQ_DIV(f, d)		((((f) & 0x0F) << 4) | ((d) & 0x0F))

#define OLED_INIT_DEFAULT \
{ \
	.i2c_cfg = \
	{ \
		.i2c_num = I2C_NUM_1, \
		.i2c_scl_io = 18, \
		.i2c_sda_io = 19, \
		.i2c_speed_hz = 100000, \
		.i2c_addr = 0x78, \
	} \
}

typedef struct _oled_cfg_t {
	struct i2c_cfg_t {
		int i2c_num;
		int i2c_scl_io;
		int i2c_sda_io;
		int i2c_speed_hz;
		unsigned char i2c_addr;
	}i2c_cfg;
} oled_cfg_t;

#define OLED_TASK_QUEUE_LEN			4
typedef struct _oled_show_task_t {
	uint32_t pos_x;
	uint32_t pos_y;
	uint8_t size;
	uint8_t *pbuf;
	uint32_t len;
} oled_show_task_t;

typedef struct _oled_t {
	arch_os_thread_handle_t pthread;
	arch_os_queue_handle_t qtask;
	void *ctx;
	struct i2c_t {
		int i2c_num; /* I2C_NUM_1: sda: 18, scl: 19*/
		int i2c_scl_io;
		int i2c_sda_io;
		int i2c_speed_hz;
		unsigned char i2c_addr;
	}i2c;
}oled_t;

int oled_set_show_task(oled_t *oled, uint32_t size, uint32_t pos_x, uint32_t pos_y, const uint8_t *pbuf, uint32_t len);
siot_err_t oled_instance_create(oled_t **oled, oled_cfg_t *cfg);

#endif /* __OLED_DRIVER_H__ */
