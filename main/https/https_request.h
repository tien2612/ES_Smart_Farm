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

#include "esp_tls.h"
#include "sdkconfig.h"
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
#include "esp_crt_bundle.h"
#endif

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER "backend-smartfarm-api.onrender.com"
#define WEB_PORT "443" // https port
#define WEB_URL "https://backend-smartfarm-api.onrender.com/sensor/1" //patch
#define GET_WEB_URL "https://backend-smartfarm-api.onrender.com/sensor/1" //get

#define SERVER_URL_MAX_SZ 256

extern TaskHandle_t get_task_handler;
extern TaskHandle_t patch_task_handler;

extern void https_request_task(void *pvparameters);

extern void https_patch_task(void *pvparameters);


#endif