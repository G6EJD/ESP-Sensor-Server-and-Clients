/* ESP8266 provides a sensor client that uploads sensor data to a sensor server
 The 'MIT License (MIT) Copyright (c) 2018 by David Bird'.
 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, 
 distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
 following conditions: 
   The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
   software use is visible to an end-user.
   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
*/
#ifdef ESP8266
  #include <ESP8266WiFi.h>       // Built-in
  #include <ESP8266WiFiMulti.h>  // Built-in
#else
  #include <WiFi.h>       // Built-in
  #include <WiFiMulti.h>  // Built-in
#endif
#include <WiFiClient.h> // Built-in
#include <Wire.h>       // Built-in
#include "DHT.h"        // https://github.com/adafruit/DHT-sensor-library

// Uncomment whatever type you're using!
#define DHTTYPE DHT11   // DHT 11
//#define DHTTYPE DHT22   // DHT 22 (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)

// Connect pin 1 (on the left) of the sensor to +5V
// NOTE: If using a board with 3.3V logic like an Arduino Due connect pin 1
// to 3.3V instead of 5V!
// Connect pin 2 of the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor

// Local ESP web-server address
String UploadData;
float  sensor_temperature, sensor_humidity, sensor_pressure, sensor_spare;

/*    Instructions:
 *    1. Check which pin your sensor is conencted to, if not a WEMOS D1 Mini, then change D4 to the pin number
 *    2. Change 'SensorNumber' to your requirements
 *    3. Change 'SensorType' to match your sensor e.g. DHT11 or DHT22
 *    4. Adjust the DHT sensor type on line 8-10 above
 *    5. Adjust sleep time as required. Maximum of 1-hour on an ESP8266
 *    6. Change the sensorserver host address to match your server's setting
*/
// This is device 'sensor-3' adjust these definitions to make each sensor unique
#define DHTPIN 16     // Device is connected to D4 on a WEMOS D1 Mini 16 on an \esp32 MHT-Live
#define SensorNumber 1
#define SensorType   "DHT11-1"             // You can make these sensor names unique e.g. SHT30-1 and SHT30-2, or your own choice
// *************************************************************************************************************
// **** NOTE: The ESP8266 only supports deep sleep mode by connecting GPIO 16 to Reset
// Wiring: ESP8266 16 -> ESP8266 RST (or D0 for GPIO16 on most ESP8266 Dev Boards
// 
// !!!!! REMEMBER TO REMOVE THE GPIO16 to RST link for (re) programming, otherwise it remains in a RESET state..
//
// *************************************************************************************************************
int SleepTime = 30 * 60 * 1000000; //SleepTime of 30mins
//This is the central servers address, all sensors need to know this
const char* ServerHost = "192.168.0.99"; // this is the IP address of the central sensor server

WiFiClient client;

#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti;
#else
  WiFiMulti wifiMulti;
#endif

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);
  Serial.println("...Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  // AP Wi-Fi credentials
  wifiMulti.addAP("ssid_for_AP_1", "your_password_for_AP_1");
  wifiMulti.addAP("ssid_for_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_for_AP_3", "your_password_for_AP_3");
  // Etc
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
    if (millis() > 40000) ESP.restart(); // If more than 40-secs since start-up, then Restart
  }
  Serial.print("\n\rWi-Fi connected\n\rIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\n\r...Wi-Fi connected");
  dht.begin();
  Serial.println("Found a sensor continuing");
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  sensor_temperature = dht.readTemperature();
  sensor_humidity    = dht.readHumidity();
  while (isnan(sensor_temperature) && isnan(sensor_humidity)) {
    Serial.println(sensor_temperature);
    sensor_temperature = dht.readTemperature(); 
    sensor_humidity    = dht.readHumidity();
  }
  CollectUploadData();
  SendHttpPOST();
  Serial.println(UploadData);
  #ifdef ESP8266
    ESP.deepSleep(SleepTime, WAKE_RF_DEFAULT); // Sleep for the time set by 'UpdateInterval'
  #else
    ESP.deepSleep(SleepTime);                  // Sleep for the time set by 'UpdateInterval'
  #endif
}

void loop() {
}

void SendHttpPOST() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting connection...");
    if (client.connect(ServerHost,80)) {
      Serial.println("connected");
    } else {
      Serial.println(" try again in 500 ms");
      delay(500);// Wait 0.5 seconds before retrying
    }
  }
  client.println("GET /"+UploadData+" HTTP/1.1\n\r");
  client.println("Host: "+String(ServerHost)); 
  client.println("Connection: close"); 
  client.stop();
}

void CollectUploadData() {
  Serial.println("...Submitting Upload request");
  UploadData  = "sensor?Sensor="+String(SensorNumber);
  UploadData += "&temperature="+String(sensor_temperature);
  UploadData += "&humidity="+String(sensor_humidity);
  UploadData += "&pressure="+String(sensor_pressure);
  UploadData += "&spare="+String(sensor_spare);
  UploadData += "&sensortype="+String(SensorType);
  Serial.println("...Information to be uploaded: "+UploadData);
}

