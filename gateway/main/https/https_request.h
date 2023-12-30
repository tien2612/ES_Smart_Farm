#ifndef HTTPS_REQUEST_H
#define HTTPS_REQUEST_H

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "protocol_examples_common.h"
#include "esp_sntp.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "driver/gpio.h"
#include "esp_tls.h"
#include "sdkconfig.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "es-backend-server.onrender.com"
#define WEB_PORT "443" // https port
#define WEB_URL_LIGHT "https://environment-monitoring-73aq.onrender.com/sensor/light" //patch
#define WEB_URL_SOIL_MOIS "https://environment-monitoring-73aq.onrender.com/sensor/soil_moisture" //patch
#define WEB_URL_DHT20 "https://environment-monitoring-73aq.onrender.com/sensor/dht20" //patch

#define GET_WEB_URL_RESULT "https://jsonplaceholder.typicode.com/todos/1" //get
#define GET_WEB_URL_LIGHT "https://environment-monitoring-73aq.onrender.com/sensor/light" //get
#define GET_WEB_URL_SOIL_MOIS "https://environment-monitoring-73aq.onrender.com/sensor/soil_moisture" //get

#define GET_WEB_URL_DHT20 "https://es-backend-server.onrender.com/result" //get

#define SERVER_URL_MAX_SZ 256

#define LED_RED_PIN GPIO_NUM_14
#define LED_YELLOW_PIN GPIO_NUM_27
#define LED_GREEN_PIN GPIO_NUM_16

typedef struct {
    char nodeId[3];
    char sensorId[10];
    char temp[3];
    char humi[3];
    char crc[10];
} DHT20_Data;

typedef struct {
    char nodeId[3];
    char sensorId[20];
    char sensorData[3];
    char crc[10];
} Others_Data;

#define SENSOR_DHT20 "dht20"
#define SENSOR_SOIL_MOISTURE "soil_moisture"
#define SENSOR_LIGHT "light"


extern TaskHandle_t get_task_handler;
extern TaskHandle_t patch_task_handler;
extern TaskHandle_t rx_task_handler;

extern void https_request_task(void *pvparameters);

extern void https_patch_task(uint8_t *data);

extern void http_get_task(void *pvParameters);

#endif