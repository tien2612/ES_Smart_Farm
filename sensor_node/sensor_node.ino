#pragma pack(push, 1)

struct dht20
{
    char sensorId[20];
    char temp[3];
    char humi[3];
    char crc[20];
};

#pragma pack(pop)

#pragma pack(push, 1)

struct other_sensor
{
    char sensorId[20];
    char sensorData[3];
    char crc[20];
};

#pragma pack(pop)

#define SEND_TIME         10000
#define DHT20             10
#define LIGHT_SENSOR      11
#define SOIL_MOIS         12

int size_dht20_struct = sizeof(struct dht20);
int size_other_struct = sizeof(struct other_sensor);

bool first;

struct Gyro_data_structure dht20_data = {"light", "15", "23", "0x123123"};
struct Gyro_data_structure light_sensor_data = {"light", "31", "55", "0x1234123"};
struct Gyro_data_structure soil_mois_data = {"soil_moistuser", "31", "55", "0x1234123"};

int status = DHT20;
void setup()
{
  Serial.begin(115200);    
  status = DHT20;
}

void loop()
{
  switch (status) {
    case DHT20:
      Serial.write('<'); // Start of frame
      Serial.write((const char *)dht20_data, _size);
      Serial.write('#');  // end of frame
      status = LIGHT_SENSOR;
      break;
    case LIGHT_SENSOR:
        Serial.write('<'); 
        Serial.write((const char *)light_sensor_data, _size);
        Serial.write('#');  // end of frame
        status = SOIL_MOIS;
        break;
    case SOIL_MOIS:
        Serial.write('<'); // Start of frame
        Serial.write((const char *)soil_mois_data, _size);
        Serial.write('#');  // end of frame      
        status = DHT20;
        break;
    default:
      break;
  }

  Serial.println();
  delay(SEND_TIME);
}