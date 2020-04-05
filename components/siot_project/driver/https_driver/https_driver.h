#ifndef __HTTPS_DRIVER_H__
#define __HTTPS_DRIVER_H__

#define CA_CERT_ENABLE		0
#define HTTPS_CONTENT_DBG	0

#define JSON_STR				"{\"results\":[{\"location\":{\"id\":\"WX4FBXXFKE4F\",\"name\":\"Beijing\",\"country\":\"CN\",\"path\":\"Beijing,Beijing,China\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"daily\":[{\"date\":\"2020-03-22\",\"text_day\":\"Sunny\",\"code_day\":\"0\",\"text_night\":\"Clear\",\"code_night\":\"1\",\"high\":\"22\",\"low\":\"7\",\"rainfall\":\"0.0\",\"precip\":\"\",\"wind_direction\":\"SW\",\"wind_direction_degree\":\"225\",\"wind_speed\":\"25.20\",\"wind_scale\":\"4\",\"humidity\":\"32\"}],\"last_update\":\"2020-03-22T17:25:55+08:00\"}]}"

#define HEFENG_PORT				"443"
#define HEFENG_KEY				"key=0e1abb034d7843449d75488e3149cd1e"
#define HEFENG_HOST				"api.heweather.net"
#define HEFENG_TYPE				"hourly"
#define HEFENG_LOCATION			"location=hefei"
#define HEFENG_LANG				"lang=en"
#define HEFENG_URL				"https://api.heweather.net/s6/weather" "/" HEFENG_TYPE "?" HEFENG_LOCATION "&" HEFENG_KEY "&" HEFENG_LANG

void https_request_by_get(void *pvParameters);

#endif
