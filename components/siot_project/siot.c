#include <stdio.h>
#include <stdlib.h>

#include "siot.h"

int siot_instance_create(siot_t **siot, siot_config_t *config)
{
	*siot = (siot_t *)malloc(sizeof(siot_t));

	return 0;
}

int siot_instance_destroy(siot_t **siot)
{
	
	if(NULL == *siot) {
		return -1;		
	}
	
	free(*siot);
	*siot = NULL;
	
	return 0;
}
