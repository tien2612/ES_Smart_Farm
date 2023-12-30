#include "sensor_data.h"
#include "crc.h"

SoftwareSerial RS_Master(RS_RO, RS_DI);

void construct_DHT20_Frame(uint16_t temp, uint16_t humi) {
    struct dht20 dht20_data = {};
    RS_Master.write('!');
    // delay(10);

    strcpy(dht20_data.nodeId, NODEID);
    strcpy(dht20_data.sensorId, "dht20");
    snprintf(dht20_data.temp, sizeof(dht20_data.temp), "%u", temp);
    snprintf(dht20_data.humi, sizeof(dht20_data.humi), "%u", humi);

    // Serial.println(dht20_data.temp)
    strcpy(dht20_data.crc, "");

    RS_Master.write((uint8_t*)&dht20_data, sizeof(dht20_data));
    // delay(10);
    RS_Master.write('#');  // end of frame
}

void construct_SoilMoisture_Frame(uint16_t data) {
    struct others_sensor data_soil_moisture = {}; 

    RS_Master.write('!'); // Start of frame
    // delay(10);
    strcpy(data_soil_moisture.nodeId, NODEID);
    strcpy(data_soil_moisture.sensorId, "soil_moisture");
    snprintf(data_soil_moisture.sensorData, sizeof(data_soil_moisture.sensorData), "%u", data);
    strcpy(data_soil_moisture.crc, "");

    RS_Master.write((uint8_t*)&data_soil_moisture, sizeof(data_soil_moisture));
    // delay(10);

    RS_Master.write('#');  // end of frame
}

void construct_LightSensor_Frame(uint16_t data) {
    struct others_sensor light_data = {};  

    RS_Master.write('!'); // Start of frame
    // delay(10);
    strcpy(light_data.nodeId, NODEID);
    strcpy(light_data.sensorId, "light");
    snprintf(light_data.sensorData, sizeof(light_data.sensorData), "%u", data);
    strcpy(light_data.crc, "");

    RS_Master.write((uint8_t*)&light_data, sizeof(light_data));
    // delay(10);

    RS_Master.write('#');  // end of frame
}

