#include "sensor_data.h"
#include "crc.h"

void construct_DHT20_Frame(uint16_t temp, uint16_t humi) {
    struct dht20 dht20_data = {};

    // Serial.println(String(temp));

    Serial.write('!');
    strcpy(dht20_data.sensorId, "dht20");
    snprintf(dht20_data.temp, sizeof(dht20_data.temp), "%u", temp);
    snprintf(dht20_data.humi, sizeof(dht20_data.humi), "%u", humi);
    strcpy(dht20_data.crc, "");

    Serial.write((uint8_t*)&dht20_data, sizeof(dht20_data));

    Serial.write('#');  // end of frame
}

void construct_SoilMoisture_Frame(uint16_t data) {
    struct others_sensor data_soil_moisture;  // Declare a variable of type dht20
    Serial.write('!'); // Start of frame

    strcpy(data_soil_moisture.sensorId, "soil_moisture");
    snprintf(data_soil_moisture.sensorData, sizeof(data_soil_moisture.sensorData), "%u", data);
    strcpy(data_soil_moisture.crc, "");

    Serial.write((uint8_t*)&data_soil_moisture, sizeof(data_soil_moisture));

    Serial.write('#');  // end of frame
}

void construct_LightSensor_Frame(uint16_t data) {
    struct others_sensor light_data;  // Declare a variable of type dht20

    Serial.write('!'); // Start of frame

    strcpy(light_data.sensorId, "light");
    snprintf(light_data.sensorData, sizeof(light_data.sensorData), "%u", data);
    strcpy(light_data.crc, "");

    Serial.write((uint8_t*)&light_data, sizeof(light_data));

    Serial.write('#');  // end of frame
}

