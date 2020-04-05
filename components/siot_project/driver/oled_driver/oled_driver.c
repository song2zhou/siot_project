#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "freertos/queue.h"

#include "arch_os.h"
#include "driver/i2c.h"
#include "oled_driver.h"
#include "font.h"

#define OLED_CHECK_ACK_EN		1
#define OLED_CHECK_ACK_DIS		0

/* set height & width for each char oled showed */
/* we need them when set show position */
static unsigned char OLED_CHAR_HEIGHT = 16;
static unsigned char OLED_CHAR_WIDTH = 8;

static siot_err_t oled_write_cmd(oled_t *oled, unsigned char cmd)
{

	i2c_cmd_handle_t hdr = i2c_cmd_link_create();

	i2c_master_start(hdr);
	/* write addr */
	i2c_master_write_byte(hdr, oled->i2c.i2c_addr, OLED_CHECK_ACK_EN);
	/* write 0x00 before cmd */
	i2c_master_write_byte(hdr, 0x00, OLED_CHECK_ACK_EN);
	/* write cmd */
	i2c_master_write_byte(hdr, cmd, OLED_CHECK_ACK_EN);
	i2c_master_stop(hdr);

	int ret = i2c_master_cmd_begin(oled->i2c.i2c_num, hdr, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(hdr);

	if (ret != ESP_OK) { return SIOT_ERR; }

	return SIOT_OK;
}

static siot_err_t oled_write_dat(oled_t *oled, unsigned char dat)
{
	i2c_cmd_handle_t hdr = i2c_cmd_link_create();

	i2c_master_start(hdr);
	/* write addr */
	i2c_master_write_byte(hdr, oled->i2c.i2c_addr, OLED_CHECK_ACK_EN);
	/* write 0x00 before cmd */
	i2c_master_write_byte(hdr, 0x40, OLED_CHECK_ACK_EN);
	/* write cmd */
	i2c_master_write_byte(hdr, dat, OLED_CHECK_ACK_EN);
	i2c_master_stop(hdr);

	int ret = i2c_master_cmd_begin(oled->i2c.i2c_num, hdr, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(hdr);

	if (ret != ESP_OK) { return SIOT_ERR; }

	return SIOT_OK;
}

static siot_err_t oled_set_show_pos(oled_t *oled, unsigned char pos_x, unsigned char pos_y, bool newline)
{
	if(newline && (pos_x >= OLED_COL_MAX)) { /* if pos_x beyond oled column, we need to set position on new line */
		pos_x %= OLED_COL_MAX;
		pos_y += OLED_CHAR_HEIGHT / 8;
	}

	oled_write_cmd(oled, 0xB0 + pos_y);
	oled_write_cmd(oled, ((pos_x & 0xF0) >> 4) | 0x10);
	oled_write_cmd(oled, pos_x & 0x0F);

	return SIOT_OK;
}

static siot_err_t oled_write_str(oled_t *oled, const unsigned char *pbuf, unsigned int len)
{
	i2c_cmd_handle_t hdr = i2c_cmd_link_create();

	i2c_master_start(hdr);
	/* write addr */
	i2c_master_write_byte(hdr, oled->i2c.i2c_addr, OLED_CHECK_ACK_EN);
	/* write 0x40 before cmd */
	i2c_master_write_byte(hdr, 0x40, OLED_CHECK_ACK_EN);
	/* write cmd */
	i2c_master_write(hdr, (uint8_t *)pbuf, len, OLED_CHECK_ACK_EN);
	i2c_master_stop(hdr);

	int ret = i2c_master_cmd_begin(oled->i2c.i2c_num, hdr, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(hdr);

	if (ret != ESP_OK) { return SIOT_ERR; }

	return SIOT_OK;
}

static siot_err_t oled_hardware_init(oled_t *oled)
{
	/* init oled module */
	oled_write_cmd(oled, 0xAE);
	oled_write_cmd(oled, 0x00);
	oled_write_cmd(oled, 0x10);
	oled_write_cmd(oled, 0x40);
	oled_write_cmd(oled, 0xB0);
	oled_write_cmd(oled, 0x81);
	oled_write_cmd(oled, 0xFF);
	oled_write_cmd(oled, OLED_SCAN_COL_DIR_L2R);
	oled_write_cmd(oled, 0xA6);
	oled_write_cmd(oled, 0xA8);
	oled_write_cmd(oled, 0x3F);
	oled_write_cmd(oled, OLED_SCAN_LINE_DIR_B2T);
	oled_write_cmd(oled, 0xD3);
	oled_write_cmd(oled, 0x00);
	oled_write_cmd(oled, 0xD5);
	oled_write_cmd(oled, 0x80);
	oled_write_cmd(oled, 0xD8);
	oled_write_cmd(oled, 0x05);
	oled_write_cmd(oled, 0xD9);
	oled_write_cmd(oled, 0xF1);
	oled_write_cmd(oled, 0xDA);
	oled_write_cmd(oled, 0x12);
	oled_write_cmd(oled, 0xDB);
	oled_write_cmd(oled, 0x30);
	oled_write_cmd(oled, 0x8D);
	oled_write_cmd(oled, 0x14);
	oled_write_cmd(oled, 0xAF);

	return SIOT_OK;
}

static siot_err_t oled_clear(oled_t *oled)
{
	int ret = SIOT_OK;

	for(int i = 0; i < OLED_PAGE_MAX; i++) {
		oled_write_cmd(oled, 0xB0 + i);
		oled_write_cmd(oled, 0x00);
		oled_write_cmd(oled, 0x10);

		unsigned char clear_data[OLED_COL_MAX] = { 0 };
		memset(clear_data, 0x00, OLED_COL_MAX);
		oled_write_str(oled, clear_data, OLED_COL_MAX);
	}

	return ret;
}

static siot_err_t oled_fill(oled_t *oled)
{
	int ret = SIOT_OK;

	for(int i = 0; i < OLED_PAGE_MAX; i++) {
		oled_write_cmd(oled, 0xB0 + i);
		oled_write_cmd(oled, 0x00);
		oled_write_cmd(oled, 0x10);

		unsigned char fill_data[OLED_COL_MAX] = { 0 };
		memset(fill_data, 0xFF, OLED_COL_MAX);
		oled_write_str(oled, fill_data, OLED_COL_MAX);
	}

	return ret;
}

static siot_err_t oled_show_char_8x16(oled_t *oled, int pos_x, int pos_y, unsigned char chr)
{
	int ret = SIOT_OK;

	OLED_CHAR_HEIGHT = 16;
	OLED_CHAR_WIDTH = 8;

	int val = chr - ' ';
	oled_set_show_pos(oled, pos_x, pos_y, true);
	for(int i = 0; i < OLED_CHAR_WIDTH; i++) {
		oled_write_dat(oled, font_8x16[val * 16 + i]);
	}

	oled_set_show_pos(oled, pos_x, pos_y + OLED_CHAR_HEIGHT / 8 - 1, true);
	for(int i = 0; i < OLED_CHAR_WIDTH; i++) {
		oled_write_dat(oled, font_8x16[val * 16 + i + 8]);
	}

	return ret;
}

static siot_err_t oled_show_char_6x8(oled_t *oled, int pos_x, int pos_y, unsigned char chr)
{
	int ret = SIOT_OK;

	OLED_CHAR_HEIGHT = 8;
	OLED_CHAR_WIDTH = 6;

	int val = chr - ' ';
	oled_set_show_pos(oled, pos_x, pos_y, true);
	for(int i = 0; i < OLED_CHAR_WIDTH; i++) {
		oled_write_dat(oled, font_6x8[val * 6 + i]);
	}

	return ret;
}

static siot_err_t oled_show_str_8x16(oled_t *oled, int pos_x, int pos_y, unsigned char *str, unsigned int len)
{
	int ret = SIOT_OK;
	if(NULL == str || len <= 0) {
		return SIOT_ERR;
	}

	OLED_CHAR_HEIGHT = 16;
	OLED_CHAR_WIDTH = 8;

	for(int i = 0; str[i] && (i < len); i++) {
		oled_show_char_8x16(oled, pos_x + 8 * i, pos_y, str[i]);
	}

	return ret;
}

static siot_err_t oled_show_str_6x8(oled_t *oled, int pos_x, int pos_y, unsigned char *str, unsigned int len)
{
	int ret = SIOT_OK;
	if(NULL == str || len <= 0) {
		return SIOT_ERR;
	}

	OLED_CHAR_HEIGHT = 8;
	OLED_CHAR_WIDTH = 6;

	for(int i = 0; str[i] && (i < len); i++) {
		oled_show_char_6x8(oled, pos_x + 6 * i, pos_y, str[i]);
	}

	return ret;
}

static arch_os_function_return_t oled_task(void *argv)
{
	oled_t *oled = (oled_t *)argv;
	oled_show_task_t oled_task;
	while(true) {

		arch_os_queue_recv(oled->qtask, (void *)&oled_task, ARCH_OS_WAIT_FOREVER);
		//int ret = xQueueReceive(oled->qtask, (void *)&oled_task, (TickType_t)pdMS_TO_TICKS(ARCH_OS_WAIT_FOREVER));

		if(oled_task.size == 16) {
			oled_show_str_8x16(oled, oled_task.pos_x, oled_task.pos_y, oled_task.pbuf, oled_task.len);
		} else {
			oled_show_str_6x8(oled, oled_task.pos_x, oled_task.pos_y, oled_task.pbuf, oled_task.len);
		}
	}

	arch_os_thread_delete(ARCH_OS_THREAD_SELF);
	return NULL;
}


int oled_set_show_task(oled_t *oled, uint32_t size, uint32_t pos_x, uint32_t pos_y, const uint8_t *pbuf, uint32_t len)
{
	oled_show_task_t oled_show_task;

	oled_show_task.size = size;
	oled_show_task.pos_x = pos_x;
	oled_show_task.pos_y = pos_y;
	oled_show_task.pbuf = pbuf;
	oled_show_task.len = len;

	xQueueSend(oled->qtask, (void *)&oled_show_task, (TickType_t)0);

	return SIOT_OK;
}

siot_err_t oled_instance_create(oled_t **oled, oled_cfg_t *cfg)
{
	if(NULL == cfg) { return SIOT_ERR; }
	int ret = 0;

	/* install i2c driver */
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num = cfg->i2c_cfg.i2c_sda_io;
	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
	conf.scl_io_num = cfg->i2c_cfg.i2c_scl_io;
	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = cfg->i2c_cfg.i2c_speed_hz;
	i2c_param_config(cfg->i2c_cfg.i2c_num, &conf);
	ret = i2c_driver_install(cfg->i2c_cfg.i2c_num, conf.mode, 0, 0, 0);

	if (ret != ESP_OK) {
		return SIOT_ERR;
	}

	/* malloc memory for oled struct */
	*oled = (oled_t *)malloc(sizeof(oled_t));
	(*oled)->i2c.i2c_addr = cfg->i2c_cfg.i2c_addr;
	(*oled)->i2c.i2c_num = cfg->i2c_cfg.i2c_num;
	(*oled)->i2c.i2c_scl_io = cfg->i2c_cfg.i2c_scl_io;
	(*oled)->i2c.i2c_sda_io = cfg->i2c_cfg.i2c_sda_io;
	(*oled)->i2c.i2c_speed_hz = cfg->i2c_cfg.i2c_speed_hz;

	/* init oled show task queue */
	arch_os_queue_create(&(*oled)->qtask, OLED_TASK_QUEUE_LEN, sizeof(oled_show_task_t));
	//(*oled)->show_task_q = xQueueCreate(OLED_TASK_QUEUE_LEN, sizeof(oled_show_task_t));

	/* oled hardware init */
	oled_hardware_init(*oled);

	/* clear screen before show */
	oled_clear(*oled);

	/* create oled task */
	arch_os_thread_create(&(*oled)->pthread, "oled_task", oled_task, 4 * 1024, *oled, 10);
	//xTaskCreate(oled_task, "oled_task", 1024 * 4, *oled, 10, NULL);

	return SIOT_OK;
}


