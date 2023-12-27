/*
 * HTTPS GET Example using plain Mbed TLS sockets
 *
 * Contacts the howsmyssl.com API via TLS v1.2 and reads a JSON
 * response.
 *
 * Adapted from the ssl_client1 example in Mbed TLS.
 *
 * SPDX-FileCopyrightText: The Mbed TLS Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPDX-FileContributor: 2015-2022 Espressif Systems (Shanghai) CO LTD
 */

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

#include "time_sync.h"
#include "wifi/wifi.h"
#include "https/https_request.h"
#include "driver/uart.h"
#include "driver/gpio.h"
static const char *TAG = "uart_events";

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)
#define START_MARKER '!'

// typedef struct {
//     char sensorData[3];
//     char crc[3];
//     char sensorId[20];
// } DataFrame;

void init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
}

int sendData(const char* logName, const char* data)
{
    const int len = strlen(data);
    const int txBytes = uart_write_bytes(UART_NUM_1, data, len);
    ESP_LOGI(logName, "Wrote %d bytes", txBytes);
    return txBytes;
}

static void tx_task(void *arg)
{
    static const char *TX_TASK_TAG = "TX_TASK";
    esp_log_level_set(TX_TASK_TAG, ESP_LOG_INFO);
    while (1) {
        sendData(TX_TASK_TAG, "From ESP32-IDF");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void receiveDataFrame(uint8_t *data)
{
    char startMarker = START_MARKER;
    ESP_LOGE(TAG, "START_MARKER: %d", data[1]);

    // DataFrame *receivedFrame = (DataFrame *)(data);  // Skip the start marker
    // ESP_LOGI(TAG, "Received Data Frame:");
    // ESP_LOGI(TAG, "Sensor ID: %s", receivedFrame->sensorId);
    // ESP_LOGI(TAG, "Sensor Data: %s", receivedFrame->sensorData);
    // ESP_LOGI(TAG, "CRC: %s", receivedFrame->crc);
    ESP_LOGI(TAG, "Call patch request");
    
    https_patch_task(data);

    ESP_LOGI(TAG, "After Call patch request");
    
}

// static void rx_task(void *arg)
// {
//     static const char *RX_TASK_TAG = "RX_TASK";

//     ESP_LOGI(RX_TASK_TAG, "Task rx");

//     esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
//     uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
//     int dataIdx = 0;
//     bool frameStartDetected = false;

//     while (1) {
//         const int rxBytes = uart_read_bytes(UART_NUM_1, data + dataIdx, RX_BUF_SIZE - dataIdx, 1000 / portTICK_PERIOD_MS);

//         if (rxBytes > 0) {
//             for (int i = 0; i < rxBytes; ++i) {
//                 ESP_LOGI(RX_TASK_TAG, "data: %c", data[dataIdx + i]);

//                 if (data[dataIdx + i] == (char)START_MARKER) {

//                     // Found the start marker, reset the data index
//                     dataIdx = 0;
//                     frameStartDetected = true;
//                 }
//             }

//             if (frameStartDetected) {
//                 dataIdx += rxBytes;
//                 if (dataIdx >= sizeof(DataFrame) + 1) {
//                     // Full frame received, process it
//                     data[dataIdx] = 0;
//                     receiveDataFrame(data + 1);  // Skip the start marker
//                     ESP_LOGI(RX_TASK_TAG, "Read %d bytes: '%s'", dataIdx, data + 1);
//                     ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data + 1, dataIdx - 1, ESP_LOG_INFO);
//                     dataIdx = 0;
//                     frameStartDetected = false;
//                 }
//             } else {
//                 // No start marker detected, discard the data
//                 dataIdx = 0;
//             }
//         } else {
//             ESP_LOGI(RX_TASK_TAG, "No data");
//         }
//     }
//     free(data);
// }


static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";

    ESP_LOGI(RX_TASK_TAG, "Task rx");

    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE + 1);
    int dataIdx = 0;

    const char END_MARKER = '#';

    while (1) {
        uint8_t startByte;
        const int rxBytesStartMarker = uart_read_bytes(UART_NUM_1, &startByte, 1, 1000 / portTICK_PERIOD_MS);

        if (rxBytesStartMarker > 0 && startByte == START_MARKER) {
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, 28, ESP_LOG_INFO);

            // Start marker found, proceed to read until the end marker
            int dataIdx = 0;
            while (1) {
                const int rxBytes = uart_read_bytes(UART_NUM_1, data + dataIdx, 1, 1000 / portTICK_PERIOD_MS);
                if (rxBytes > 0) {
                    if (data[dataIdx] == END_MARKER) {
                        // End marker found, break out of the loop
                        break;
                    }
                    dataIdx += rxBytes;
                    // if (dataIdx >= sizeof(DataFrame)) {
                    //     // Buffer full, reset index
                    //     dataIdx = 0;
                    // }
                } else {
                    // No data received, handle accordingly
                    ESP_LOGI(RX_TASK_TAG, "No data");
                    break;
                }
            }
            // Process the received frame between start and end markers
            ESP_LOGE(RX_TASK_TAG, "Buffer length %d ", dataIdx);
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, dataIdx, ESP_LOG_INFO);

            receiveDataFrame(data);
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

            ESP_LOGI(RX_TASK_TAG, "Read %d bytes", dataIdx);
        } else {
            // Start marker not found, handle accordingly
            ESP_LOGI(RX_TASK_TAG, "Start marker not found");
            vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a delay or handle the situation based on your requirements
        }
    }
    free(data);
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    wifi_init_sta();

    // xTaskCreate(&https_request_task, "https_get_task", 8192, NULL, 5, &get_task_handler);
    // xTaskCreate(&https_patch_task, "https_patch_task", 8192, NULL, 5, &patch_task_handler);
    ESP_LOGE(TAG, "After!");
    init();
    ESP_LOGE(TAG, "Before!");


    BaseType_t taskCreationResult = xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES - 1, &rx_task_handler);
    if (taskCreationResult != pdPASS) {
        ESP_LOGE(TAG, "Failed to create rx_task!");
    }
    else ESP_LOGE(TAG, "success to create rx_task!");
    
    // xTaskCreate(tx_task, "uart_tx_task", 1024 * 2, NULL, configMAX_PRIORITIES - 2, NULL);
}