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
#define END_MARKER '#'

#define ECHO_TEST_TXD   (CONFIG_ECHO_UART_TXD)
#define ECHO_TEST_RXD   (CONFIG_ECHO_UART_RXD)

// RTS for RS485 Half-Duplex Mode manages DE/~RE
#define ECHO_TEST_RTS   (CONFIG_ECHO_UART_RTS)

// CTS is not used in RS485 Half-Duplex Mode
#define ECHO_TEST_CTS   (UART_PIN_NO_CHANGE)

#define BUF_SIZE        (127)
#define BAUD_RATE       (CONFIG_ECHO_UART_BAUD_RATE)

// Read packet timeout
#define PACKET_READ_TICS        (100 / portTICK_PERIOD_MS)
#define ECHO_TASK_STACK_SIZE    (2048)
#define ECHO_TASK_PRIO          (10)
#define ECHO_UART_PORT          (CONFIG_ECHO_UART_PORT_NUM)

// Timeout threshold for UART = number of symbols (~10 tics) with unchanged state on receive pin
#define ECHO_READ_TOUT          (3) // 3.5T * 8 = 28 ticks, TOUT=3 -> ~24..33 ticks

// typedef struct {
//     char sensorData[3];
//     char crc[3];
//     char sensorId[20];
// } DataFrame;
#define uart_num ECHO_UART_PORT

static void configure_led(void)
{
    ESP_LOGI(TAG, "Config led!");
    gpio_reset_pin(LED_RED_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_RED_PIN, GPIO_MODE_OUTPUT);

    gpio_reset_pin(LED_YELLOW_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_YELLOW_PIN, GPIO_MODE_OUTPUT);

        gpio_reset_pin(LED_GREEN_PIN);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GREEN_PIN, GPIO_MODE_OUTPUT);
}

void init(void)
{
    // const uart_config_t uart_config = {
    //     .baud_rate = 115200,
    //     .data_bits = UART_DATA_8_BITS,
    //     .parity = UART_PARITY_DISABLE,
    //     .stop_bits = UART_STOP_BITS_1,
    //     .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    //     .source_clk = UART_SCLK_DEFAULT,
    // };
    // // We won't use a buffer for sending data.
    // uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    // uart_param_config(UART_NUM_1, &uart_config);
    // uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_config_t uart_config = {
        .baud_rate = BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 122,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);

    ESP_LOGI(TAG, "Start RS485 application test and configure UART.");

    // Install UART driver (we don't need an event queue here)
    // In this example we don't even use a buffer for sending data.
    ESP_ERROR_CHECK(uart_driver_install(uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));

    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    ESP_LOGI(TAG, "UART set pins, mode and install driver.");

    // Set UART pins as per KConfig settings
    ESP_ERROR_CHECK(uart_set_pin(uart_num, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Set RS485 half duplex mode
    ESP_ERROR_CHECK(uart_set_mode(uart_num, UART_MODE_RS485_HALF_DUPLEX));

    // Set read timeout of UART TOUT feature
    ESP_ERROR_CHECK(uart_set_rx_timeout(uart_num, ECHO_READ_TOUT));

    // Allocate buffers for UART
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);

    ESP_LOGI(TAG, "UART start recieve loop.\r\n");
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
    ESP_LOGE(TAG, "START_MARKER: %d", data[1]);

    ESP_LOGI(TAG, "Call patch request");
    
    https_patch_task(data);

    ESP_LOGI(TAG, "After Call patch request");
    
}

static void rx_task(void *arg)
{
    static const char *RX_TASK_TAG = "RX_TASK";

    ESP_LOGI(RX_TASK_TAG, "Task rx");

    esp_log_level_set(RX_TASK_TAG, ESP_LOG_INFO);
    uint8_t* data = (uint8_t*)malloc(RX_BUF_SIZE + 1);
    int dataIdx = 0;

    while (1) {
        uint8_t receivedByte;
        int len = uart_read_bytes(uart_num, &receivedByte, 1, PACKET_READ_TICS);
        
        if (len > 0 && receivedByte == START_MARKER) {
            ESP_LOG_BUFFER_HEXDUMP(RX_TASK_TAG, data, 28, ESP_LOG_INFO);
            ESP_LOGI(RX_TASK_TAG, "start market found %c", (char)receivedByte);

            // Start marker found, proceed to read until the end marker
            int dataIdx = 0;
            while (1) {
                const int rxBytes = uart_read_bytes(uart_num, data + dataIdx, 1, PACKET_READ_TICS);
                
                ESP_LOGI(RX_TASK_TAG, "data en, len %d, char %c", rxBytes, (char)data[dataIdx] );

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
            // ESP_LOGI(RX_TASK_TAG, "Start marker not found, len %d, char %c", len, (char)receivedByte);
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

    // init rs485
    init();

    // config led
    configure_led();
    BaseType_t taskCreationResult = xTaskCreate(rx_task, "uart_rx_task", 8192, NULL, configMAX_PRIORITIES - 1, &rx_task_handler);
    if (taskCreationResult != pdPASS) {
        ESP_LOGE(TAG, "Failed to create rx_task!");
    }
    else ESP_LOGE(TAG, "success to create rx_task!");
    
    xTaskCreate(https_request_task, "https_request_task", 8192, NULL, configMAX_PRIORITIES - 2, NULL);
}