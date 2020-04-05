#ifndef __SIOT_DEFINE_H__
#define __SIOT_DEFINE_H__

typedef enum {
	SIOT_OK = 0,
	SIOT_ERR = -1,
	SIOT_ERR_PARAM = -2,
	SIOT_ERR_MEM = -3,
	SIOT_ERR_UNKNOWN = -4,
}siot_err_t;

#define SIOT_ABS(value) ((value) >= 0 ? (value) : -(value))

#endif /* __SIOT_DEFINE_H__ */