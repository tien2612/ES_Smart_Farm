// #ifndef SENSOR_DATA
// #define SENSOR_DATA

// #include "Arduino.h"
// #include <SoftwareSerial.h>

// #define DHT20_TASK_SEND_TIME 30000
// #define LIGHT_TASK_SEND_TIME 20000
// #define SOIL_MOISTURE_TASK_SEND_TIME 40000

// #define RS_RO 2  // RX pin of MAX485 connected to pin 2 on Arduino
// #define RS_DI 3  // TX pin of MAX485 connected to pin 3 on Arduino
// #define DE_RE_PIN 4  // DE (Data Enable) pin of MAX485 connected to pin 4 on Arduino

// // #define NODEID "1"
// #define NODEID "2"

// extern SoftwareSerial RS_Master;

// // #define DHTPIN 2

// struct dht20
// {
//     char nodeId[3];
//     char sensorId[10];
//     char temp[3];
//     char humi[3];
//     char crc[10];
// };

// #pragma pack(pop)

// #pragma pack(push, 1)

// struct others_sensor
// {
//     char nodeId[3];
//     char sensorId[20];
//     char sensorData[3];
//     char crc[10];
// };

// #pragma pack(pop)

// extern void construct_DHT20_Frame(uint16_t temp, uint16_t humiz);
// extern void construct_SoilMoisture_Frame(uint16_t data);
// extern void construct_LightSensor_Frame(uint16_t data);


// #endif
#include <DHT.h> // Gọi thư viện DHT22

const int DHTPIN = A2; //Đọc dữ liệu từ DHT22 ở chân A3 trên mạch Arduino
const int DHTTYPE = DHT22; //Khai báo loại cảm biến, có 2 loại là DHT11 và DHT22

DHT dht(DHTPIN, DHTTYPE);

void setup()
{
    Serial.begin(115200);
    Serial.println("Arduino.vn!");
    dht.begin(); // Khởi động cảm biến
}

void loop()
{
    float h = dht.readHumidity(); //Đọc độ ẩm
    float t = dht.readTemperature(); //Đọc nhiệt độ

    Serial.println("Arduino.vn");
    Serial.print("Nhiet do: ");
    Serial.println(t); //Xuất nhiệt độ
    Serial.print("Do am: ");
    Serial.println(h); //Xuất độ ẩm

    Serial.println(); //Xuống hàng
}