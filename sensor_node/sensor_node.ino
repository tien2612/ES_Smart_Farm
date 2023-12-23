#include <Arduino_FreeRTOS.h>
#include "crc.h"
#include "sensor_data.h"
#include <semphr.h>
#include "MemoryFree.h"
#include "pgmStrToRAM.h"

TaskHandle_t light_task_handler;
TaskHandle_t soil_mois_task_handler;
TaskHandle_t dht20_task_handler;

SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

void sendDHT20Task(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            // Access the shared UART
            // Send the struct over UART
            construct_DHT20_Frame(20, 50);
            // Release the mutex
            xSemaphoreGive(xMutex);

            vTaskDelay(pdMS_TO_TICKS(DHT20_TASK_SEND_TIME));
        }

        Serial.println(freeMemory());  // print how much RAM is available in bytes.
    }
}

void sendSoilMoistureTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {
        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdPASS) {
            // Access the shared UART
            // Send the struct over UART
            construct_SoilMoisture_Frame(55);
            // Release the mutex
            xSemaphoreGive(xMutex);

            vTaskDelay(pdMS_TO_TICKS(SOIL_MOISTURE_TASK_SEND_TIME));
        }

        Serial.println(freeMemory());  // print how much RAM is available in bytes.

    }
}

void sendLightTask(void *pvParameters) {
    (void)pvParameters;

    for (;;) {

        if (xSemaphoreTake(xMutex, portMAX_DELAY) == pdTRUE) {
            // Access the shared UART
            // Send the struct over UART
            construct_LightSensor_Frame(30);
            // Release the mutex
            xSemaphoreGive(xMutex);

            vTaskDelay(pdMS_TO_TICKS(LIGHT_TASK_SEND_TIME));
        }

        Serial.println(freeMemory());  // print how much RAM is available in bytes.

    }
}

void setup() {
    Serial.begin(115200);
    // dht.begin();

    // Create a FreeRTOS task for sending data
    xTaskCreate(sendDHT20Task, "Send DHT22", 512, NULL, 1, NULL);
    xTaskCreate(sendSoilMoistureTask, "Send Moistuser", 100, NULL, 1, &soil_mois_task_handler);
    xTaskCreate(sendLightTask, "Send light data", 200, NULL, 1, &light_task_handler);

    Serial.println(getPSTR("Old way to force String to Flash")); // forced to be compiled into and read 	
    Serial.println(F("New way to force String to Flash")); // forced to be compiled into and read 	
    Serial.println(F("Free RAM = ")); //F function does the same and is now a built in library, in IDE > 1.0.0
    Serial.println(freeMemory());  // print how much RAM is available in bytes.
    // // Start the FreeRTOS scheduler
    vTaskStartScheduler();
}

void loop() {
}
