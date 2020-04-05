#ifndef __SIOT_H__
#define __SIOT_H__

typedef struct _siot_t {
	void *oled;
	void *tcp;
}siot_t;

typedef struct _siot_config_t {	
}siot_config_t;

int siot_instance_create(siot_t **siot, siot_config_t *config);
int siot_instance_destroy(siot_t **siot);

#endif /* __SIOT_H__ */