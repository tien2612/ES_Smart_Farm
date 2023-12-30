#include <Arduino_FreeRTOS.h>
#include "crc.h"
#include "sensor_data.h"
#include <semphr.h>
#include "MemoryFree.h"
#include "pgmStrToRAM.h"
#include "DHT.h"   
#include <SoftwareSerial.h>

const int DHTPIN = 5;      
const int DHTTYPE = DHT22;  

const int SOILPIN = A0;
const int LIGHTPIN = A1;

DHT dht(DHTPIN, DHTTYPE);

int temp = 0;
int humi = 0;
int light = 0;
int soil = 0;

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
            temp = (int)dht.readTemperature(); 
            humi = (int)dht.readHumidity();
            construct_DHT20_Frame(temp, humi);
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
            int value = analogRead(SOILPIN);
            soil = 100 - map(value, 0, 1023, 0, 100);
            construct_SoilMoisture_Frame(soil);
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
            int value = analogRead(LIGHTPIN);
            light = map(value, 0, 1023, 0, 100);
            construct_LightSensor_Frame(light);
            // Release the mutex
            xSemaphoreGive(xMutex);

            vTaskDelay(pdMS_TO_TICKS(LIGHT_TASK_SEND_TIME));
        }

        Serial.println(freeMemory());  // print how much RAM is available in bytes.

    }
}

void setup() {
    Serial.begin(115200);
    RS_Master.begin(115200);  // Start software serial for RS485 communication
    pinMode(DE_RE_PIN, OUTPUT);  // Set DE pin as output
    digitalWrite(DE_RE_PIN, HIGH);

    dht.begin();

    // Create a FreeRTOS task for sending data
    xTaskCreate(sendDHT20Task, "Send DHT22", 300, NULL, 1, NULL);
    xTaskCreate(sendSoilMoistureTask, "Send Moistuser", 300, NULL, 1, NULL);
    xTaskCreate(sendLightTask, "Send light data", 100, NULL, 2, &light_task_handler);

    // Serial.println(getPSTR("Old way to force String to Flash")); // forced to be compiled into and read 	
    // Serial.println(F("New way to force String to Flash")); // forced to be compiled into and read 	
    // Serial.println(F("Free RAM = ")); //F function does the same and is now a built in library, in IDE > 1.0.0
    // Serial.println(freeMemory());  // print how much RAM is available in bytes.
    // // Start the FreeRTOS scheduler
    vTaskStartScheduler();
}

void loop() {
}