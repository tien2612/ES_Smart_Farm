#ifndef SENSOR_DATA
#define SENSOR_DATA

#include "Arduino.h"

#define DHT20_TASK_SEND_TIME 30000
#define LIGHT_TASK_SEND_TIME 20000
#define SOIL_MOISTURE_TASK_SEND_TIME 30000

// #define DHTPIN 2

struct dht20
{
    char sensorId[10];
    char temp[3];
    char humi[3];
    char crc[10];
};

#pragma pack(pop)

#pragma pack(push, 1)

struct others_sensor
{
    char sensorId[20];
    char sensorData[3];
    char crc[10];
};

#pragma pack(pop)

extern void construct_DHT20_Frame(uint16_t temp, uint16_t humi);
extern void construct_SoilMoisture_Frame(uint16_t data);
extern void construct_LightSensor_Frame(uint16_t data);


#endif
