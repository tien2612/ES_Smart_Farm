# Environment Monitoring Project

This project focuses on environmental monitoring, utilizing Arduino and ESP32-IDF for the microcontroller, and React Native for the mobile application. The supported targets include ESP32, ESP32-C2, ESP32-C3, ESP32-C6, ESP32-H2, ESP32-S2, and ESP32-S3.

## How to use run
Before project configuration and build, be sure to set the correct chip target using `idf.py set-target <chip_name>`.

### Build and Flash
#### ESP32-IDF
Build the project and flash it to the board, then run monitor tool to view serial output:
```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.
#### Arduino
Simply flash the code corresponding to each node.

## Mobile Application
To test the mobile application, navigate to mobile foldeer and run the following commands:
```
npm install
npx expo start
```
Then scan the QR code generated with your mobile phone using the Expo Go app to preview and interact with the mobile application.

**Note** If you encounter a "Network response timed out" error in our app, ensure that the running PORT is allowed through your firewall. Adding the PORT to the list of allowed TCP ports can resolve this issue.
### User Interface of our mobile application
![Dashboard](https://i.ibb.co/Rgnrh7V/Dashboard-App.png)
![Dashboard](https://i.ibb.co/7ksSgXD/Analyze-App.png)



