/*
 *  Version 1  ESP8266/32 provides a sensor client that uploads sensor data to a sensor server
 *  
 This software, the ideas and concepts is Copyright (c) David Bird 2018. All rights to this software are reserved.
 
 Any redistribution or reproduction of any part or all of the contents in any form is prohibited other than the following:
 1. You may print or download to a local hard disk extracts for your personal and non-commercial use only.
 2. You may copy the content to individual third parties for their personal use, but only if you acknowledge the author David Bird as the source of the material.
 3. You may not, except with my express written permission, distribute or commercially exploit the content.
 4. You may not transmit it or store it in any other website or other form of electronic retrieval system for commercial purposes.

 The above copyright ('as annotated') notice and this permission notice shall be included in all copies or substantial portions of the Software and where the
 software use is visible to an end-user.
 
 THE SOFTWARE IS PROVIDED "AS IS" FOR PRIVATE USE ONLY, IT IS NOT FOR COMMERCIAL USE IN WHOLE OR PART OR CONCEPT. FOR PERSONAL USE IT IS SUPPLIED WITHOUT WARRANTY 
 OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE AUTHOR OR COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 See more at http://www.dsbird.org.uk
 *
*/
#ifdef ESP8266
  #include <ESP8266WiFi.h>      // Built-in
  #include <ESP8266WiFiMulti.h> // Built-in
#else
  #include <WiFi.h>             // Built-in
  #include <WiFiMulti.h>        // Built-in
#endif
#include <WiFiClient.h>         // Built-in
#include <Adafruit_BME280.h>    // https://github.com/adafruit/Adafruit-BMP280-Library 
#include <Wire.h>               // Built-in

// Local ESP web-server address
String UploadData;
float  sensor_temperature, sensor_humidity, sensor_pressure, sensor_spare;

/*    Instructions:
 *    1. Check which pin your sensor is conencted to, if not a WEMOS D1 Mini, then change D4 to the pin number
 *    2. Change 'SensorNumber' to your requirements
 *    3. Change 'SensorType' to match your sensor e.g. BMP085 or BMP180 or BMP280
 *    4. Adjust the BMP sensor type on line 8-10 above
 *    5. Adjust sleep time as required. Maximum of 1-hour on an ESP8266
 *    6. Change the sensorserver host address to match your server's setting
*/

// This is device 'sensor-2' adjust these definitions to make each sensor unique
#define SensorNumber 2
#define SensorType   "BME280"             // You can make these sensor names unique e.g. SHT30-1 and SHT30-2, or your own choice
#define SleepTime    5*60*1000000         // 5-min = 5*60*1000000uS
//This is the central servers address, all sensors need to know this
const char*  ServerHost = "192.168.0.99"; // this is the IP address of the central sensor server

WiFiClient client;

#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti;
#else
  WiFiMulti wifiMulti;
#endif

#define pressure_offset 3.04 // Adjust for your altitude
Adafruit_BME280 bme;

void setup() {
  Serial.begin(115200);
  // Wire.begin(4,5); // For most ESP32 boards but check your board with a Serial.println(SDA) and Serial.println(SCL) first
  Wire.begin();  // D1 and D2 on a WEMOS D1 Mini(sda,scl) or use like this WEMOS D1 Mini example another pair of pins
  Serial.println("...Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  // AP Wi-Fi credentials
  wifiMulti.addAP("ssid_for_AP_1","your_password_for_AP_1");
  wifiMulti.addAP("ssid_for_AP_2","your_password_for_AP_2");
  wifiMulti.addAP("ssid_for_AP_3","your_password_for_AP_3");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
    if (millis() > 40000) ESP.restart(); // If more than 40-secs since start-up, then Restart
  }
  Serial.print("\n\rWi-Fi connected\n\rIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\n\r...Wi-Fi connected");
  if (!bme.begin()) {
    Serial.println("Could not find a sensor, check wiring!");
  }
  else
  {
    Serial.println("Found a sensor continuing");
    sensor_pressure = bme.readPressure(); 
    while (isnan(sensor_pressure)) {
      Serial.println(sensor_pressure);
      sensor_pressure = bme.readPressure(); 
    }
  }
}

void loop() {
  sensor_temperature = bme.readTemperature();
  sensor_humidity    = bme.readHumidity();
  sensor_pressure    = bme.readPressure()/100.0F + pressure_offset;
  CollectUploadData();
  SendHttpPOST();
  Serial.println("Going to sleep");
  #ifdef ESP8266 
    ESP.deepSleep(SleepTime, WAKE_RF_DEFAULT); // Sleep for the time set by 'UpdateInterval' 
  #else 
    ESP.deepSleep(SleepTime);                  // Sleep for the time set by 'UpdateInterval' 
  #endif 
}

void SendHttpPOST() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting connection...");
    if (client.connect(ServerHost,80)) {
      Serial.println("connected");
    } else {
      Serial.println(" try again in 500 ms");
      delay(500); // Wait 0.5 seconds before retrying
    }
  }
  client.println("GET /"+UploadData+" HTTP/1.1\n\r");
  client.println("Host: "+String(ServerHost));
  delay(500); // Essential delay for ESP32 
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

