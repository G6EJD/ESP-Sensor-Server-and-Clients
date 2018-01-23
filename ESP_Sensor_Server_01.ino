/* ESP32 providing a sensor server collating sensor data uploads and siaplsy the readigns on a Web page
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
  #include <ESP8266WebServer.h>  // Built-in
  #include <ESP8266mDNS.h>
#else
  #include <WiFi.h>              // Built-in
  #include <WiFiMulti.h>         // Built-in
  #include <ESP32WebServer.h>    // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
  #include <ESPmDNS.h>
#endif
#include <WiFiClient.h>   // Built-in

#ifdef ESP8266
  ESP8266WiFiMulti wifiMulti; 
  ESP8266WebServer server(80);
#else
  WiFiMulti wifiMulti;
  ESP32WebServer server(80);
#endif

// Adjust the following values to match your needs
///////////////////////////////////////////////////////////////////////////////////////
#define ServerVersion       1.0
#define number_of_sensors   16    // 20 is a practical limit
#define readings_per_sensor 1     // NOTE: there must be 1 record per sensor, making this (e.g.6x2) will allow 2 readings per sensor per day, etc. But not supported in this version
#define number_of_days      2     // NOTE: must be > 1, but more than 2 is not supported in this version
#define PageWidth           1023  // Screen width in pixels
char SensorName[number_of_sensors][16] = {"Sensor-1","Sensor-2","Sensor-3","Sensor-4","Sensor-5","Sensor-6",
                                          "Sensor-7","Sensor-8","Sensor-9","Sensor-10","Sensor-11","Sensor-12"};//NOTE: No more than 16-chars per sensor name, or adjust array size
char RowName[5][16]                    = {"Sensor-type","Temperature","Humidity","Pressure","Spare"}; //NOTE: No more than 16-chars per row name, or adjust array size
char RowUnits[5][20]                   = {"","&deg;<sup>C</sup>","%","hPa",""}; //NOTE: No more than 20-chars per unit name, or adjust array size
///////////////////////////////////////////////////////////////////////////////////////

char ServerLogicalName[16] = {0}; // used here http://ServerLogicalName.local 
// If you make ServerLogicalName = "myserver" then the address would be http://myserver.local/

typedef struct { 
  byte   sensornumber;       // Sensor number provided by e.g. Sensor=3
  float  value1;             // For example Temperature
  float  value2;             // For example Humidity
  float  value3;             // For example Pressure
  float  value4;             // Spare
  String sensortype = "N/A"; // The sensor type e.g. an SHT30 or BMP180
} sensor_record_type; 

sensor_record_type sensor_data[number_of_days][number_of_sensors * readings_per_sensor]; // Define the data array 
int    sensor_reading = 0;   // Default value
int    day_record_cnt = 1;   // Default value
int    day_count      = 1;   // Default value
String webpage = "";
bool   AUpdate = true;       // Default value

IPAddress local_IP(192, 168, 0, 99); // Set your server's fixed IP address here
IPAddress gateway(192, 168, 0, 1); 
IPAddress subnet(255, 255, 0, 0); 

void setup(void){
  Serial.begin(115200);
  sprintf(ServerLogicalName,"sensorserver"); // Don't add '.local' !
  Serial.println("Hostname: "+String(ServerLogicalName)); 
  if (!WiFi.config(local_IP, gateway, subnet)) { 
    Serial.println("STA Failed to configure"); 
  } 
  wifiMulti.addAP("ssid_of_AP_1", "password_for_AP_1");   // add Wi-Fi networks you want to connect to, so your SSID and your Password
  wifiMulti.addAP("ssid_of_AP_2", "password_for_AP_2");
  wifiMulti.addAP("ssid_of_AP_3", "password_for_AP_3");
  // etc
  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250); Serial.print('.');
  }
  Serial.print("\nConnected to "+WiFi.SSID()); // Report which SSID has been used
  Serial.print("IP address:\t");             
  Serial.println(WiFi.localIP());            // And the IP address being used, although http://ServerLogicalName.local will access the device
  Serial.println("");
  while (WiFi.status() != WL_CONNECTED) { // Wait for connection
    delay(500);
    Serial.print(".");
  }
  if (!MDNS.begin("sensorserver")) { 
    Serial.println("Error setting up MDNS responder!"); 
    while(1){ 
     delay(1000); 
    } 
  } 
  ///////////////////////////// Request commands
  server.on("/", handleRoot);
  server.on("/test", [](){ server.send(200, "text/plain", "The server status is OK"); }); // Test the server by giving a status response
  server.on("/sensor", handleSensors);   // Associate the handler function to the path, note /sensor only refreshes the data
  server.onNotFound(handleNotFound);     // Handle when a client requests an unknown URI for example something other than "/")
  server.on("/AUpdate", auto_update);     // Turn auto-update ON/OFF       
  ///////////////////////////// End of Request commands
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void){
  server.handleClient(); // Listen for client connections
}

// Functions from here...
void handleRoot() {
  server.send(200, "text/plain", "hello from the ESP8266 Sensor Server");
}

void handleNotFound(){
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

// This function handles many arguments e.g. http://sensorserver.local/sensor?Sensor=1&Temperature=11.1&Humidity=22.2&Pressure=1000.0mb&Value=5.0&sensortype=BME280
//                                      e.g. http://sensorserver.local/sensor?Sensor=2&Temperature=22.2&Humidity=44.4&Pressure=1002.2mb
//                                      e.g. http://sensorserver.local/sensor?Sensor=3&Temperature=33.3&Humidity=66.6&Pressure=1003.3mb
// Update the sensorserver to your own and click on the links to test the server!
void handleSensors() { //Server request handler
  webpage = "";
  Serial.print("Received from: ");
  for (int i = 0; i <= server.args(); i++) Serial.println(server.argName(i)+" "+server.arg(i)); // Display received arguments from sensor
  
  day_record_cnt = 1; // day_record_cnt += 1; //scope for more than 1-day of records, but forced to 1 for this version!
  int sensor_num = server.arg(0).toInt();
  if (sensor_num > number_of_sensors) sensor_num = 0;
  if (sensor_num !=0 ) sensor_reading = sensor_num; else sensor_reading = 1; // Each sensor has its own record, so Sensor-1 readings go in record-1, etc
  for (int i = 0; i <= server.args(); i++) {
    if (sensor_num != 0 && sensor_num <= number_of_sensors) { // If a valid sensor number then process
      sensor_data[day_record_cnt][sensor_reading].sensornumber = (byte)sensor_num; // Max. 255 sensors
      if (i == 1) sensor_data[day_record_cnt][sensor_reading].value1     = server.arg(i).toFloat();
      if (i == 2) sensor_data[day_record_cnt][sensor_reading].value2     = server.arg(i).toFloat();
      if (i == 3) sensor_data[day_record_cnt][sensor_reading].value3     = server.arg(i).toFloat();
      if (i == 4) sensor_data[day_record_cnt][sensor_reading].value4     = server.arg(i).toFloat();
      if (i == 5) sensor_data[day_record_cnt][sensor_reading].sensortype = server.arg(i);
   } else Serial.println("Sensor number rejected");
  }
  #define resolution 1 // 1 = 20.2 or 2 = 20.23 or 3 = 20.234 displayed data if the sensor supports it
  append_page_header();  
  webpage += "<hr />";
  if (!ReceivedAnySensor()) {
    webpage += "<h3>*** Waiting For Sensors ***</h3>";
    Serial.println("No sensors received");
  }
  else
  {
    webpage += "<div style='overflow-x:auto;'>"; // Add horizontal scrolling if number of fields exceeds page width
    webpage += "<table><tr><th></th>";  // Top-left corner is blank
    for (int s = 0; s < number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s+1].sensornumber != 0 && sensor_data[1][s+1].sensornumber <= number_of_sensors) 
        webpage += "<th>"+String(SensorName[s])+"</th>";
    }
    webpage += "</tr><tr><td>Sensor Number</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s].sensornumber != 0   && sensor_data[1][s].sensornumber <= number_of_sensors) 
        webpage += "<td text-align='center'>"+String(sensor_data[1][s].sensornumber)+"</td>";
    }
    webpage += "<tr><td>"+String(RowName[0])+"</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s].sensornumber != 0   && sensor_data[1][s].sensornumber <= number_of_sensors) 
        webpage += "<td>"+String(sensor_data[1][s].sensortype)+RowUnits[0]+"</td>";
    }
    webpage += "</tr>";
    webpage += "<tr><td>"+String(RowName[1])+"</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s].sensornumber != 0   && sensor_data[1][s].sensornumber <= number_of_sensors) 
        webpage += "<td>"+String(sensor_data[1][s].value1,resolution)+RowUnits[1]+"</td>";
    }
    webpage += "</tr>";
    webpage += "<tr><td>"+String(RowName[2])+"</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s].sensornumber != 0   && sensor_data[1][s].sensornumber <= number_of_sensors) 
        webpage += "<td>"+String(sensor_data[1][s].value2,resolution)+RowUnits[2]+"</td>";
    }
    webpage += "</tr>";
    webpage += "<tr><td>"+String(RowName[3])+"</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if (sensor_data[day_record_cnt][s].sensornumber != 0   && sensor_data[1][s].sensornumber <= number_of_sensors) 
        webpage += "<td>"+String(sensor_data[1][s].value3,resolution)+RowUnits[3]+"</td>";
    }
    webpage += "</tr>";
    webpage += "<tr><td>"+String(RowName[4])+"</td>";
    for (int s = 1; s <= number_of_sensors; s++) {
      if ((sensor_data[day_record_cnt][s].sensornumber != 0) && (sensor_data[1][s].sensornumber <= number_of_sensors))
        { webpage += "<td>"+String(sensor_data[1][s].value4,resolution)+RowUnits[4]+"</td>"; }
    }
    webpage += "</tr>";
    webpage += "</table></div>";
  }
  webpage += "<hr /><br>";
  append_page_footer();
  server.send(200, "text/html", webpage);  //Send Response to the HTTP request
}

void auto_update () { // Auto-refresh of the screen, this turns it on/off
  AUpdate = !AUpdate;
  handleSensors();
}

void append_page_header() {
  webpage  = "<!DOCTYPE html><html><head>";
  if (AUpdate) webpage += "<meta http-equiv='refresh' content='30'>"; // 30-sec refresh time, test needed to prevent auto updates repeating some commands
  webpage += "<title>Sensor Server</title><style>";
  webpage += "body {width:"+String(PageWidth)+"px; margin:0 auto; font-family:arial; font-size:14px; text-align:center; color:blue; background-color:#F7F2Fd;}";
  webpage += "</style></head><body><h1>Sensor Server "+String(ServerVersion)+"</h1>";
}

void append_page_footer(){ // Saves repeating many lines of code for HTML page footers
  webpage += "<head><style>ul{list-style-type:none; margin:0; padding:0; overflow:hidden; background-color:#ADD8E6; font-size:14px;}";
  webpage += "li{float:left;border-right:1px solid #bbb;}last-child {border-right: none;}";
  webpage += "li a{display: block;padding:3px 15px; text-decoration:none;}";
  webpage += "li a:hover{background-color:#FFFFFF;}";
  webpage += "section {font-size:14px;}";
  webpage += "p {background-color:#E3D1E2;}";
  webpage += "h1{background-color:#558ED5; color:Yellow;}";
  webpage += "h3{color:orange; font-size:24px;}";
  webpage += "table{font-family:arial,sans-serif; font-size:16px; border-collapse:collapse; width:100%;}"; 
  webpage += "th,td {border: 0px solid black;text-align: left;padding: 5px; border-bottom: 1px solid #ddd;}"; 
  webpage += "</style>";
  webpage += "<ul>";
  webpage += "<li><a href='/'>Home Page</a></li>";
  webpage += "<li><a href='/AUpdate'>AutoUpdate("+String((AUpdate?"ON":"OFF"))+")</a></li>";
  webpage += "</ul>";
  webpage += "<p>&copy;"+String(char(byte(0x40>>1)))+String(char(byte(0x88>>1)))+String(char(byte(0x5c>>1)))+String(char(byte(0x98>>1)))+String(char(byte(0x5c>>1)));
  webpage += String(char((0x84>>1)))+String(char(byte(0xd2>>1)))+String(char(0xe4>>1))+String(char(0xc8>>1))+String(char(byte(0x40>>1)));
  webpage += String(char(byte(0x64/2)))+String(char(byte(0x60>>1)))+String(char(byte(0x62>>1)))+String(char(0x70>>1))+"</p>";
  webpage += "</body></html>";
}

bool ReceivedAnySensor(){
  bool sensor_received = false;
  for (int s = 1; s <= number_of_sensors; s++) {
    if ((sensor_data[day_record_cnt][s].sensornumber != 0) && (sensor_data[day_record_cnt][s].sensornumber <= number_of_sensors)) { sensor_received = true; }
  }
  return sensor_received;
}

