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
#include <WiFiClient.h>   // Built-in

#include <OneWire.h>
#include <DallasTemperature.h>

#define sensor_pin 21

#define ONE_WIRE_BUS sensor_pin  // DS18B20 connected to pin D2 on a Wemos D1 or 21 on MHT-LIVE
#define TEMPERATURE_PRECISION 12 // Lower resolution

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

int numberOfDevices; // Number of temperature devices found

DeviceAddress tempDeviceAddress, deviceAddress; // We'll use this variable to store a found device address

String UploadData;
float  sensor_temperature = 0, sensor_humidity = 0, sensor_pressure =0, sensor_spare =0;

// This is device 'sensor-1' adjust this definition to make each sensor unique
#define SensorNumber 4
#define SensorType   "DS18B20"    

// *************************************************************************************************************
// **** NOTE: The ESP8266 only supports deep sleep mode by connecting GPIO 16 to Reset
// Wiring: ESP8266 16 -> ESP8266 RST (or D0 for GPIO16 on most ESP8266 Dev Boards
// 
// !!!!! REMEMBER TO REMOVE THE GPIO16 to RST link for (re) programming, otherwise it remains in a RESET state..
//
// *************************************************************************************************************
int SleepTime = 30 * 60 * 1000000; //SleepTime of 30mins
// Local ESP web-server address
const char* ServerHost = "192.168.0.99"; // this is the IP address of the sensor server

WiFiClient client;

#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti;
#else
  WiFiMulti wifiMulti;
#endif

OneWire  ds(21);  // pin D2 on Wemos D1 Mini or 21 on MHT-LIVE

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  sensor_temperature = tempC;
  Serial.print("Temp C: ");
  Serial.println(tempC);
  Serial.print("Temp F: ");
  Serial.println(DallasTemperature::toFahrenheit(tempC)); // Converts tempC to Fahrenheit
}

void setup() {
  Serial.begin(115200);
  Serial.println("...Connecting to Wi-Fi");
  WiFi.mode(WIFI_STA);
  // AP Wi-Fi credentials
  wifiMulti.addAP("ssid_from_AP_1"  "your_password_for_AP_1");
  wifiMulti.addAP("ssid_from_AP_2"  "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");
  // Etc
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250);
    Serial.print('.');
    if (millis() > 40000) ESP.restart(); // If more than 40-secs since start-up, then Restart
  }
  Serial.print("\n\rWi-Fi connected\n\rIP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("\n\r...Wi-Fi connected");
  //////
  sensors.begin(); // Start the DS18B20 reading process
  // Get a count of devices on the wire
  numberOfDevices = sensors.getDeviceCount();
    // locate device on the bus
  Serial.print("Locating devices... Found ");
  Serial.print(numberOfDevices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
  
  // Loop through each device, print out address
  for(int i=0; i < numberOfDevices; i++)
  {
    // Search the wire for address
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Found device ");
      Serial.print(i, DEC);
      Serial.print(" with address: ");
      printAddress(tempDeviceAddress);
      Serial.println();
      Serial.print("Setting resolution to ");
      Serial.println(TEMPERATURE_PRECISION, DEC);
      // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
      Serial.print("Resolution set to: ");
      Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
      Serial.println();
    }
    else
    {
      Serial.print("Found ghost device at ");
      Serial.print(i, DEC);
      Serial.print(" but could not detect address. Check power and cabling");
    }
  }
  sensors.requestTemperatures(); // Send the command to get temperatures
  Serial.println("DONE");
  for(int i=0;i<numberOfDevices; i++)
  {
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      Serial.print("Temperature for device: ");
      Serial.println(i,DEC);
      printTemperature(tempDeviceAddress); // Use a simple function to print out the data
    }
  } 
  CollectUploadData();
  SendHttpPOST();
  Serial.println();
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
      delay(500); // Wait 0.5 seconds before retrying
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




