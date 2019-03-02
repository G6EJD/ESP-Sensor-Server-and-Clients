/*  Version 1

    ESP32/ESP8266 Sensor Server

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

*/
#ifdef ESP8266
#include <ESP8266WiFi.h>       // Built-in
#include <ESP8266WiFiMulti.h>  // Built-in
#include <ESP8266WebServer.h>  // Built-in
#include <ESP8266mDNS.h>
#include <sys/time.h>          // struct timeval
#else
#include <WiFi.h>              // Built-in
#include <WiFiMulti.h>         // Built-in
#include <WebServer.h>         // https://github.com/Pedroalbuquerque/ESP32WebServer download and place in your Libraries folder
#include <ESPmDNS.h>
#include "FS.h"
#endif
#include "time.h"
#include "Network.h"
#include "Sys_Variables.h"
#include "CSS.h"
#include <SD.h>                  // Built-in
#include <SPI.h>                 // Built-in

#ifdef ESP8266
ESP8266WiFiMulti wifiMulti;
ESP8266WebServer server(80);
#else
WiFiMulti wifiMulti;
WebServer server(80);
#endif

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void setup(void) {
  Serial.begin(115200);
  //WiFi.config(ip, gateway, subnet, dns1, dns2);
  if (!WiFi.config(local_IP, gateway, subnet, dns)) {
    Serial.println("WiFi STATION Failed to configure Correctly");
  }
  wifiMulti.addAP(ssid_1, password_1);  // add Wi-Fi networks you want to connect to, it connects from strongest to weakest signal strength
  wifiMulti.addAP(ssid_2, password_2);
  wifiMulti.addAP(ssid_3, password_3);
  wifiMulti.addAP(ssid_4, password_4);

  Serial.println("Connecting ...");
  while (wifiMulti.run() != WL_CONNECTED) { // Wait for the Wi-Fi to connect: scan for Wi-Fi networks, and connect to the strongest of the networks above
    delay(250); Serial.print('.');
  }
  Serial.println("\nConnected to " + WiFi.SSID() + " Use IP address: " + WiFi.localIP().toString()); // Report which SSID and IP is in use
  // The logical name http://fileserver.local will also access the device if you have 'Bonjour' running or your system supports multicast dns
  if (!MDNS.begin(servername)) {          // Set your preferred server name, if you use "myserver" the address would be http://myserver.local/
    Serial.println(F("Error setting up MDNS responder!"));
    ESP.restart();
  }
#ifdef ESP32
  SPI.begin(18, 19, 23); // (SCK,MOSI,MISO) SPI pins used by most ESP32 boards.
  // Note: SD_Card readers on the ESP32 will NOT work unless there is a pull-up on MISO, either do this or wire a resistor (1K to 4K7) to Vcc
  pinMode(19, INPUT_PULLUP);
  pinMode(23, INPUT_PULLUP);
#endif
  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS_pin)) { // see if the card is present and can be initialised. Wemos D1 Mini SD-Card shields use D8 for CS
    Serial.println(F("Card failed or not present, no SD Card data logging possible..."));
    SD_present = false;
  }
  else
  {
    Serial.println(F("Card initialised... data logging enabled..."));
    SD_present = true;
  }
  // Note again: Using an ESP32 and SD Card readers requires a 1K to 4K7 pull-up to 3v3 on the MISO line, otherwise they do-not function.
  //----------------------------------------------------------------------
  ///////////////////////////// Server Commands that will be responded to
  server.on("/",            HomePage);
  server.on("/test", []()  {
    server.send(200, "text/plain", "Server status is OK");
  }); // Simple server test by providing a status response
  server.on("/sensor",      HandleSensors);   // Now associate the handler functions to the path of each function
  server.on("/Liveview",    DisplaySensors);
  server.on("/Iconview",    DisplayLocations);
  server.on("/Csetup",      ChannelSetup);
  server.on("/AUpdate",     Auto_Update);
  server.on("/Help",        Help);
  server.on("/Cstream",     Channel_File_Stream);
  server.on("/Cdownload",   Channel_File_Download);
  server.on("/Odownload",   File_Download);
  server.on("/Cupload",     Channel_File_Upload);
  server.on("/Cerase",      Channel_File_Erase);
  server.on("/Oerase",      File_Erase);
  server.on("/upload", HTTP_POST, []() {
    server.send(200);
  }, handleFileUpload);
  server.on("/SDdir",       SD_dir);
  server.on("/chart",       DrawChart);
  server.on("/forward",     MoveChartForward);
  server.on("/reverse",     MoveChartBack);
  server.onNotFound(handleNotFound);  // When a client requests an unknown URI for example something other than "/")
  ///////////////////////////// End of Request commands
  server.begin();
  Serial.println("HTTP server started");
  for (int i = 0; i < number_of_channels; i++) {
    ChannelData[i].ID = i;
  }
  SetupTime();
  ReadChannelData();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void loop(void) {
  server.handleClient(); // Listen for client connections
  if (data_amended) {
    SaveChannelData();
    data_amended = false;
  }
}
// Functions from here...
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void HomePage() {
  SendHTML_Header(refresh_off);
  webpage += F("<a href='/Liveview'><button>View</button></a>");
  webpage += F("<a href='/Iconview'><button>Locations</button></a>");
  webpage += F("<a href='/chart'><button>Graph</button></a>");
  webpage += F("<a href='/Csetup'><button>Setup</button></a>");
  webpage += F("<a href='/Help'><button>Help</button></a><br><br><br>");
  webpage += F("<a href='/Cstream'><button>Stream</button></a>");
  webpage += F("<a href='/Cdownload'><button>Download</button></a>");
  webpage += F("<a href='/Cupload'><button>Upload</button></a>");
  webpage += F("<a href='/Cerase'><button>Erase</button></a>");
  webpage += F("<a href='/SDdir'><button>Directory</button></a><br><br><br><p>");
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Help() {
  SendHTML_Header(refresh_off);
  webpage += F("<h3 class='rcorners_m'>Sensor Server Help</h3>");
  webpage += F("<p>Help section: The Server collects sensor readings and stores them in a Channel and then displayed according to the Channel settings.\
                Data fields are numeric and can be interpreted as the units of your choice. All data from sensors is saved sequentially to the SD-Card and there is a file \
                for each Channel.<br></p>");
  webpage += F("<p>For example, Channel-1 data is saved in a file called '1.txt' and Channel-2 in '2.txt' and so-on. The contents of a Channel file can be streamed \
                to the web-browser, graphed, downloaded, deleted or amended and then uploaded back again.</p><br>");
  webpage += F("<table style='text-align:left;font:0.725em;color:slateblue'><tr><td>Use the menu options to:</td></tr>");
  webpage += F("<tr><td>'Refresh'</td><td>Toggle on/off automatic screen refresh of 30-secs</td></tr>");
  webpage += F("<tr><td>'View Channels'</td><td>Shows sensor data for each channel as it is received</td></tr>");
  webpage += F("<tr><td>'View Locations'</td><td>Shows an Icon and data for each channel as it is received</td></tr>");
  webpage += F("<tr><td>'Graph Channels'</td><td>Graph Channel Readings</td></tr>");
  webpage += F("<tr><td>'Setup Channels'</td><td>Edit the Channel Name, Description, Sensor Type and Units</td></tr>");
  webpage += F("<tr><td>'Stream Channel Data'</td><td>Stream Channel Data to your browser</td></tr>");
  webpage += F("<tr><td>'Download Channel Data'</td><td>Download Channel Data to a file</td></tr>");
  webpage += F("<tr><td>'[Download] File'</td><td>Download any file</td></tr>");
  webpage += F("<tr><td></td><td>After download into Excel use the formula '=(((DataCell/60)/60)/24)+DATE(1970,1,1)' to convert UNIX time to a Date-Time</td></tr>");
  webpage += F("<tr><td></td><td>e.g If A1 = 1517059610 (unix time) and A2 is empty use A2 = (((A1/60)/60)/24)+DATE(1970,1,1)' now A2 = HH:MM;SS-DD/MM/YY</td></tr>");
  webpage += F("<tr><td></td><td>Set the format of cell A2 to Custom, then dd/mm/yyyy hh:mm</td></tr>");
  webpage += F("<tr><td>'Upload Channel Data'</td><td>Upload Channel Data file</td></tr>");
  webpage += F("<tr><td>'Erase Channel Data'</td><td>Erase Channel Data file</td></tr>");
  webpage += F("<tr><td>'[Erase] File'</td><td>Erase any file</td></tr>");
  webpage += F("<tr><td>'File Directory'</td><td>List files on the SD-Card</td></tr>");
  webpage += F("</table><br>");
  SendHTML_Content();
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~la~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ChannelSetup() {
  if (server.args() > 0 ) { // Arguments were received
    data_amended = true;
    for (byte ArgNum = 0; ArgNum <= server.args(); ArgNum++) {
      if (server.argName(ArgNum) == "chan_name")         {
        ChannelData[channel_number].Name         = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_desc")         {
        ChannelData[channel_number].Description  = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_type")         {
        ChannelData[channel_number].Type         = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field1")       {
        ChannelData[channel_number].Field1       = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field1_units") {
        ChannelData[channel_number].Field1_Units = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field2")       {
        ChannelData[channel_number].Field2       = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field2_units") {
        ChannelData[channel_number].Field2_Units = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field3")       {
        ChannelData[channel_number].Field3       = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field3_units") {
        ChannelData[channel_number].Field3_Units = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field4")       {
        ChannelData[channel_number].Field4       = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "chan_field4_units") {
        ChannelData[channel_number].Field4_Units = server.arg(ArgNum);
      }
      if (server.argName(ArgNum) == "iconname")          {
        ChannelData[channel_number].IconName     = server.arg(ArgNum);
      }
      ChannelData[channel_number].Created = TimeNow();
    }
  }
  if      (server.hasArg("edit_c1"))  {
    channel_number = 1;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c2"))  {
    channel_number = 2;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c3"))  {
    channel_number = 3;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c4"))  {
    channel_number = 4;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c5"))  {
    channel_number = 5;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c6"))  {
    channel_number = 6;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c7"))  {
    channel_number = 7;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c8"))  {
    channel_number = 8;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c9"))  {
    channel_number = 9;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c10")) {
    channel_number = 10;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c11")) {
    channel_number = 11;
    ChannelEditor(channel_number);
  }
  else if (server.hasArg("edit_c12")) {
    channel_number = 12;  // NOTE: *** Add more channels here to match the 'number_of_channels' value
    ChannelEditor(channel_number);
  }
  else
  {
    SendHTML_Header(refresh_off);
    webpage += F("<h3 class='rcorners_m'>Channel Setup/Editor</h3><br>");
    webpage += F("<table align='center'>");
    webpage += F("<tr><th style='width:7%;text-align:center'>Channel ID</th><th style='width:15%'>Name</th><th style='width:35%'>Description</th>");
    webpage += F("<th style='width:7%'>Created on:</th><th style='width:11%'>Last Updated at:</th><th style='width:6%'>File Size</th></tr>");
    for (byte cn = 1; cn < number_of_channels; cn++) {
      webpage += F("<tr><td style='text-align:center'>");
      webpage += String(ChannelData[cn].ID) + "</td><td>" + ChannelData[cn].Name + "</td><td>" + ChannelData[cn].Description + "</td>";
      webpage += "<td>" + Time(ChannelData[cn].Created).substring(9) + "</td>";
      if (ChannelData[cn].Updated == BaseTime) webpage += F("<td>-</td><td></td><tr>");
      else
      {
        webpage += "<td>" + Time(ChannelData[cn].Updated) + "</td><td>";
        webpage += String(file_size(String(cn))) + "</td><tr>";
      }
    }
    webpage += F("</table>");
    webpage += F("<h3>Select channel to View/Edit</h3>");
    webpage += F("<FORM action='/Csetup' method='post'>");
    for (byte in_select = 1; in_select < number_of_channels; in_select++) {
      webpage += F("<button class='buttons' type='submit' name='edit_c"); webpage += String(in_select) + "' value='" + String(in_select) + "'>Channel-" + String(in_select) + "</button>";
    }
    webpage += F("<br><br>");
    SendHTML_Content();
    append_page_footer(graph_off);
    SendHTML_Content();
    SendHTML_Stop();
    webpage = "";
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ChannelEditor(int channel_number) {
  SendHTML_Header(refresh_off);
  webpage += F("<h3 class='rcorners_m'>Channel Editor</h3>");
  webpage += F("<FORM action='/Csetup' method='post'>");
  webpage += F("<table align='center'><tr><td style='width:8%'>ID:</td><td>");
  webpage += String(ChannelData[channel_number].ID) + "</td><td>Edit Entries</td><td>Edit Units</td></tr>";
  //---
  webpage += F("<tr><td>Name:</td><td>"); webpage += ChannelData[channel_number].Name + "</td>";
  webpage += F("<td><input type='text' name='chan_name' pattern='[A-Za-z0-9.-_~# ]{1,12}' title='only A-Z,a-z,0-9,./_ -&*<>#:; allowed' value='");
  webpage += ChannelData[channel_number].Name + "'></td><td></td></tr>";
  //---
  webpage += F("<tr><td>Description:</td><td>"); webpage += ChannelData[channel_number].Description + "</td>";
  webpage += F("<td><input type='text' name='chan_desc' value='"); webpage += ChannelData[channel_number].Description + "'></td><td></td></tr>";
  //---
  webpage += F("<tr><td>Sensor Type:</td><td>"); webpage += ChannelData[channel_number].Type + "</td>";
  webpage += F("<td><input type='text' name='chan_type' value='"); webpage += ChannelData[channel_number].Type + "'></td><td></td></tr>";
  //---
  webpage += F("<tr><td>Field-1 Name:</td><td>"); webpage += ChannelData[channel_number].Field1 + "</td>";
  webpage += F("<td><input type='text' name='chan_field1' value='"); webpage += ChannelData[channel_number].Field1 + "'></td>";
  webpage += F("<td><input type='text' name='chan_field1_units' value='"); webpage += ChannelData[channel_number].Field1_Units + "'></td></tr>";
  SendHTML_Content();
  //---
  webpage += F("<tr><td>Field-2 Name:</td><td>"); webpage += ChannelData[channel_number].Field2 + "</td>";
  webpage += F("<td><input type='text' name='chan_field2' value='"); webpage += ChannelData[channel_number].Field2 + "'></td>";
  webpage += F("<td><input type='text' name='chan_field2_units' value='"); webpage += ChannelData[channel_number].Field2_Units + "'></td></tr>";
  //---
  webpage += F("<tr><td>Field-3 Name:</td><td>"); webpage += ChannelData[channel_number].Field3 + "</td>";
  webpage += F("<td><input type='text' name='chan_field3' value='"); webpage += ChannelData[channel_number].Field3 + "'></td>";
  webpage += F("<td><input type='text' name='chan_field3_units' value='"); webpage += ChannelData[channel_number].Field3_Units + "'></td></tr>";
  //---
  webpage += F("<tr><td>Field-4 Name:</td><td>"); webpage += ChannelData[channel_number].Field4 + "</td>";
  webpage += F("<td><input type='text' name='chan_field4' value='"); webpage += ChannelData[channel_number].Field4 + "'></td>";
  webpage += F("<td><input type='text' name='chan_field4_units' value='"); webpage += ChannelData[channel_number].Field4_Units + "'></td></tr>";
  //---
  webpage += F("<tr><td>Icon Name:</td><td>"); webpage += ChannelData[channel_number].IconName + "</td>";
  webpage += F("<td><input type='text' name='iconname' value='"); webpage += ChannelData[channel_number].IconName + "'></td>";
  //---
  webpage += F("</form></table>");
  webpage += F("<br><button class='buttons' type='submit'>Enter</button><br>");
  webpage += F("<a href='/Csetup'>[Back]</a><br><br>");
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
  if (data_amended) SaveChannelData();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
boolean isValidNumber(String str) {
  str.trim();
  if (!(str.charAt(0) == '+' || str.charAt(0) == '-' || isDigit(str.charAt(0)))) return false; // Failed if not starting with a unary +- sign or a number
  for (byte i = 1; i < str.length(); i++) {
    if (!(isDigit(str.charAt(i)) || str.charAt(i) == '.')) return false; // Anything other than a number or . is a failure
  }
  return true;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void handleNotFound() {
  if (SD_present && loadFromSdCard(server.uri())) return; // Process a file if requested
  ReportCommandNotFound(""); // go back to home page if erroneous command entered
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// This function handles many arguments e.g. http://sensorserver.local/sensor?Sensor=1&Temperature=11.1&Humidity=22.2&Pressure=1000.0mb&Value=5.0&sensortype=BME280
//                                      e.g. http://sensorserver.local/sensor?Sensor=2&Temperature=22.2&Humidity=44.4&Pressure=1002.2mb
//                                      e.g. http://sensorserver.local/sensor?Sensor=3&Temperature=33.3&Humidity=66.6&Pressure=1003.3mb
// Update the links above to your own name e.g. sensorserver becomes 'myserver.local' and click on the links to test the server!
void HandleSensors() { //Server request handler
  Serial.println("Handle sensor reception");
  if (server.argName(0) != "") Serial.print("Received from: ");
  for (int i = 0; i <= server.args(); i++) Serial.println(server.argName(i) + " " + server.arg(i)); // Display received arguments from sensor
  int sensor_num = server.arg(0).toInt();
  if (sensor_num > number_of_channels) sensor_num = 0; // Channel-0 is a null channel, assigned when incorrect data is received
  if ((sensor_num >= 1 && sensor_num < number_of_channels) && ((server.argName(0) == "Sensor") || (server.argName(0) == "sensor"))) {
    ChannelData[sensor_num].Updated = TimeNow(); // A valid sensor has been received, so update time-received record
    sensor_reading = sensor_num;                 // Each sensor has its own record, so Sensor-1 readings go in record-1, etc
    for (int i = 0; i <= server.args(); i++) {
      if (sensor_num != 0 && sensor_num <= number_of_channels) { // If a valid sensor number then process
        SensorData[sensor_reading].sensornumber = (byte)sensor_num; // Max. 255 sensors
        if (i == 1) SensorData[sensor_reading].value1     = server.arg(i).toFloat();
        if (i == 2) SensorData[sensor_reading].value2     = server.arg(i).toFloat();
        if (i == 3) SensorData[sensor_reading].value3     = server.arg(i).toFloat();
        if (i == 4) SensorData[sensor_reading].value4     = server.arg(i).toFloat();
        if (i == 5) SensorData[sensor_reading].sensortype = server.arg(i);
      }
      else Serial.println("Sensor number rejected");
    }
    SensorData[sensor_reading].readingtime = TimeNow();
    if (SD_present) { // If the SD-Card is present and board fitted then append the next reading to the log file called 'datalog.txt'
      File dataFile = SD.open("/" + String(sensor_num) + ".txt", FILE_READ); // Check to see if a file exists
      if (!dataFile) { // If not update its creation date
        ChannelData[sensor_num].Created =  TimeNow();
      }
      dataFile.close();
#ifdef ESP8266
      dataFile = SD.open("/" + String(sensor_num) + ".txt", FILE_WRITE); //On the ESP8266 FILE_WRITE opens file for writing and move to the end of the file
#else
      dataFile = SD.open("/" + String(sensor_num) + ".txt", FILE_APPEND); //On the ESP32 it needs FILE_APPEND to open file for writing and movs to the end of the file
#endif
      if (dataFile) { // if the file is available, write to it
        dataFile.print(SensorData[sensor_reading].readingtime); dataFile.print(char(9)); // TAB delimited data
        dataFile.print(SensorData[sensor_reading].value1);      dataFile.print(char(9));
        dataFile.print(SensorData[sensor_reading].value2);      dataFile.print(char(9));
        dataFile.print(SensorData[sensor_reading].value3);      dataFile.print(char(9));
        dataFile.print(SensorData[sensor_reading].value4);      dataFile.print(char(9));
        dataFile.println(SensorData[sensor_reading].sensortype);
      }
      dataFile.close();
    }
  }
  DisplaySensors();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DisplaySensors() {
  byte resolution = 1; // 1 = 20.2 or 2 = 20.23 or 3 = 20.234 displayed data if the sensor supports the resolution
  SendHTML_Header(refresh_on);
  webpage = F("<hr />");
  if (!ReceivedAnySensor()) {
    webpage += F("<h3>*** Waiting For Data Reception ***</h3>");
    Serial.println("No sensors received");
  }
  else
  {
    webpage += F("<h3>Current Sensor Readings</h3>");
    webpage += F("<div style='overflow-x:auto;'>");  // Add horizontal scrolling if number of fields exceeds page width
    webpage += F("<table style='width:100%'><tr><th style='width:4%'>Sensor Name</th>");
    // NOTE: ****** You might think there is a more efficient way to do the next section, there is, except using a FOR loop, but the ESP32 gives stack errors, it's a compiler error!!
    if (SensorData[0].sensornumber  != 0 && SensorData[0].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[0].sensornumber].Name)+"</th>";
    if (SensorData[1].sensornumber  != 0 && SensorData[1].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[1].sensornumber].Name)+"</th>";
    if (SensorData[2].sensornumber  != 0 && SensorData[2].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[2].sensornumber].Name)+"</th>";
    if (SensorData[3].sensornumber  != 0 && SensorData[3].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[3].sensornumber].Name)+"</th>";
    if (SensorData[4].sensornumber  != 0 && SensorData[4].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[4].sensornumber].Name)+"</th>";
    if (SensorData[5].sensornumber  != 0 && SensorData[5].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[5].sensornumber].Name)+"</th>";
    if (SensorData[6].sensornumber  != 0 && SensorData[6].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[6].sensornumber].Name)+"</th>";
//    if (SensorData[7].sensornumber  != 0 && SensorData[7].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[7].sensornumber].Name)+"</th>";
//    if (SensorData[8].sensornumber  != 0 && SensorData[8].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[8].sensornumber].Name)+"</th>";
//    if (SensorData[9].sensornumber  != 0 && SensorData[9].sensornumber  <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[9].sensornumber].Name)+"</th>";
//    if (SensorData[10].sensornumber != 0 && SensorData[10].sensornumber <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[10].sensornumber].Name)+"</th>";
//    if (SensorData[11].sensornumber != 0 && SensorData[11].sensornumber <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[11].sensornumber].Name)+"</th>";
//    if (SensorData[12].sensornumber != 0 && SensorData[12].sensornumber <= number_of_channels) webpage += "<th>"+String(ChannelData[SensorData[12].sensornumber].Name)+"</th>";
    webpage += F("</tr><tr><td style='width:4%'>Sensor Number</td>");
    if ((SensorData[0].sensornumber != 0)  && (SensorData[0].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[0].sensornumber) + "</td>";
    if ((SensorData[1].sensornumber != 0)  && (SensorData[1].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[1].sensornumber) + "</td>";
    if ((SensorData[2].sensornumber != 0)  && (SensorData[2].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[2].sensornumber) + "</td>";
    if ((SensorData[3].sensornumber != 0)  && (SensorData[3].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[3].sensornumber) + "</td>";
    if ((SensorData[4].sensornumber != 0)  && (SensorData[4].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[4].sensornumber) + "</td>";
    if ((SensorData[5].sensornumber != 0)  && (SensorData[5].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[5].sensornumber) + "</td>";
    if ((SensorData[6].sensornumber != 0)  && (SensorData[6].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[6].sensornumber) + "</td>";
//    if ((SensorData[7].sensornumber != 0)  && (SensorData[7].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[7].sensornumber) + "</td>";
//    if ((SensorData[8].sensornumber != 0)  && (SensorData[8].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[8].sensornumber) + "</td>";
//    if ((SensorData[9].sensornumber != 0)  && (SensorData[9].sensornumber  <= number_of_channels)) webpage += "<td>" + String(SensorData[9].sensornumber) + "</td>";
//    if ((SensorData[10].sensornumber != 0) && (SensorData[10].sensornumber <= number_of_channels)) webpage += "<td>" + String(SensorData[10].sensornumber) + "</td>";
//    if ((SensorData[11].sensornumber != 0) && (SensorData[11].sensornumber <= number_of_channels)) webpage += "<td>" + String(SensorData[11].sensornumber) + "</td>";
//    if ((SensorData[12].sensornumber != 0) && (SensorData[12].sensornumber <= number_of_channels)) webpage += "<td>" + String(SensorData[12].sensornumber) + "</td>";
    webpage += F("<tr><td style='width:10%'>Type</td>");
    if (SensorData[0].sensornumber != 0   && SensorData[0].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[0].Type + "</td>";
    if (SensorData[1].sensornumber != 0   && SensorData[1].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[1].Type + "</td>";
    if (SensorData[2].sensornumber != 0   && SensorData[2].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[2].Type + "</td>";
    if (SensorData[3].sensornumber != 0   && SensorData[3].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[3].Type + "</td>";
    if (SensorData[4].sensornumber != 0   && SensorData[4].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[4].Type + "</td>";
    if (SensorData[5].sensornumber != 0   && SensorData[5].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[5].Type + "</td>";
    if (SensorData[6].sensornumber != 0   && SensorData[6].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[6].Type + "</td>";
//    if (SensorData[7].sensornumber != 0   && SensorData[7].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[7].Type + "</td>";
//    if (SensorData[8].sensornumber != 0   && SensorData[8].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[8].Type + "</td>";
//    if (SensorData[9].sensornumber != 0   && SensorData[9].sensornumber  <= number_of_channels) webpage += "<td>" + ChannelData[9].Type + "</td>";
//    if (SensorData[10].sensornumber != 0  && SensorData[10].sensornumber <= number_of_channels) webpage += "<td>" + ChannelData[10].Type + "</td>";
//    if (SensorData[11].sensornumber != 0  && SensorData[11].sensornumber <= number_of_channels) webpage += "<td>" + ChannelData[11].Type + "</td>";
//    if (SensorData[12].sensornumber != 0  && SensorData[12].sensornumber <= number_of_channels) webpage += "<td>" + ChannelData[12].Type + "</td>";
    webpage += F("</tr><tr><td>Field-1</td>");
    if (SensorData[0].sensornumber != 0   && SensorData[0].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[0].value1, resolution) + ChannelData[0].Field1_Units + "</td>";
    if (SensorData[1].sensornumber != 0   && SensorData[1].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[1].value1, resolution) + ChannelData[1].Field1_Units + "</td>";
    if (SensorData[2].sensornumber != 0   && SensorData[2].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[2].value1, resolution) + ChannelData[2].Field1_Units + "</td>";
    if (SensorData[3].sensornumber != 0   && SensorData[3].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[3].value1, resolution) + ChannelData[3].Field1_Units + "</td>";
    if (SensorData[4].sensornumber != 0   && SensorData[4].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[4].value1, resolution) + ChannelData[4].Field1_Units + "</td>";
    if (SensorData[5].sensornumber != 0   && SensorData[5].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[5].value1, resolution) + ChannelData[5].Field1_Units + "</td>";
    if (SensorData[6].sensornumber != 0   && SensorData[6].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[6].value1, resolution) + ChannelData[6].Field1_Units + "</td>";
//    if (SensorData[7].sensornumber != 0   && SensorData[7].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[7].value1, resolution) + ChannelData[7].Field1_Units + "</td>";
//    if (SensorData[8].sensornumber != 0   && SensorData[8].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[8].value1, resolution) + ChannelData[8].Field1_Units + "</td>";
//    if (SensorData[9].sensornumber != 0   && SensorData[9].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[9].value1, resolution) + ChannelData[9].Field1_Units + "</td>";
//    if (SensorData[10].sensornumber != 0  && SensorData[10].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[10].value1, resolution) + ChannelData[10].Field1_Units + "</td>";
//    if (SensorData[11].sensornumber != 0  && SensorData[11].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[11].value1, resolution) + ChannelData[11].Field1_Units + "</td>";
//    if (SensorData[12].sensornumber != 0  && SensorData[12].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[12].value1, resolution) + ChannelData[12].Field1_Units + "</td>";
    SendHTML_Content();
    webpage += F("</tr><tr><td>Field-2</td>");
    if (SensorData[0].sensornumber != 0   && SensorData[0].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[0].value2, resolution) + ChannelData[0].Field2_Units + "</td>";
    if (SensorData[1].sensornumber != 0   && SensorData[1].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[1].value2, resolution) + ChannelData[1].Field2_Units + "</td>";
    if (SensorData[2].sensornumber != 0   && SensorData[2].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[2].value2, resolution) + ChannelData[2].Field2_Units + "</td>";
    if (SensorData[3].sensornumber != 0   && SensorData[3].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[3].value2, resolution) + ChannelData[3].Field2_Units + "</td>";
    if (SensorData[4].sensornumber != 0   && SensorData[4].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[4].value2, resolution) + ChannelData[4].Field2_Units + "</td>";
    if (SensorData[5].sensornumber != 0   && SensorData[5].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[5].value2, resolution) + ChannelData[5].Field2_Units + "</td>";
    if (SensorData[6].sensornumber != 0   && SensorData[6].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[6].value2, resolution) + ChannelData[6].Field2_Units + "</td>";
//    if (SensorData[7].sensornumber != 0   && SensorData[7].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[7].value2, resolution) + ChannelData[7].Field2_Units + "</td>";
//    if (SensorData[8].sensornumber != 0   && SensorData[8].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[8].value2, resolution) + ChannelData[8].Field2_Units + "</td>";
//    if (SensorData[9].sensornumber != 0   && SensorData[9].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[9].value2, resolution) + ChannelData[9].Field2_Units + "</td>";
//    if (SensorData[10].sensornumber != 0  && SensorData[10].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[10].value2, resolution) + ChannelData[10].Field2_Units + "</td>";
//    if (SensorData[11].sensornumber != 0  && SensorData[11].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[11].value2, resolution) + ChannelData[11].Field2_Units + "</td>";
//    if (SensorData[12].sensornumber != 0  && SensorData[12].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[12].value2, resolution) + ChannelData[12].Field2_Units + "</td>";
    webpage += F("</tr><tr><td>Field-3</td>");
    if (SensorData[0].sensornumber != 0   && SensorData[0].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[0].value3, resolution) + ChannelData[0].Field3_Units + "</td>";
    if (SensorData[1].sensornumber != 0   && SensorData[1].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[1].value3, resolution) + ChannelData[1].Field3_Units + "</td>";
    if (SensorData[2].sensornumber != 0   && SensorData[2].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[2].value3, resolution) + ChannelData[2].Field3_Units + "</td>";
    if (SensorData[3].sensornumber != 0   && SensorData[3].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[3].value3, resolution) + ChannelData[3].Field3_Units + "</td>";
    if (SensorData[4].sensornumber != 0   && SensorData[4].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[4].value3, resolution) + ChannelData[4].Field3_Units + "</td>";
    if (SensorData[5].sensornumber != 0   && SensorData[5].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[5].value3, resolution) + ChannelData[5].Field3_Units + "</td>";
    if (SensorData[6].sensornumber != 0   && SensorData[6].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[6].value3, resolution) + ChannelData[6].Field3_Units + "</td>";
//    if (SensorData[7].sensornumber != 0   && SensorData[7].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[7].value3, resolution) + ChannelData[7].Field3_Units + "</td>";
//    if (SensorData[8].sensornumber != 0   && SensorData[8].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[8].value3, resolution) + ChannelData[8].Field3_Units + "</td>";
//    if (SensorData[9].sensornumber != 0   && SensorData[9].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[9].value3, resolution) + ChannelData[9].Field3_Units + "</td>";
//    if (SensorData[10].sensornumber != 0   && SensorData[10].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[10].value3, resolution) + ChannelData[10].Field3_Units + "</td>";
//    if (SensorData[11].sensornumber != 0   && SensorData[11].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[11].value3, resolution) + ChannelData[11].Field3_Units + "</td>";
//    if (SensorData[12].sensornumber != 0   && SensorData[12].sensornumber <= number_of_channels) webpage += "<td>" + String(SensorData[12].value3, resolution) + ChannelData[12].Field3_Units + "</td>";
    webpage += F("</tr><tr><td>Field-4</td>");
    if ((SensorData[0].sensornumber != 0) && (SensorData[0].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[0].value4, resolution) + ChannelData[0].Field4_Units + "</td>";
    if ((SensorData[1].sensornumber != 0) && (SensorData[1].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[1].value4, resolution) + ChannelData[1].Field4_Units + "</td>";
    if ((SensorData[2].sensornumber != 0) && (SensorData[2].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[2].value4, resolution) + ChannelData[2].Field4_Units + "</td>";
    if ((SensorData[3].sensornumber != 0) && (SensorData[3].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[3].value4, resolution) + ChannelData[3].Field4_Units + "</td>";
    if ((SensorData[4].sensornumber != 0) && (SensorData[4].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[4].value4, resolution) + ChannelData[4].Field4_Units + "</td>";
    if ((SensorData[5].sensornumber != 0) && (SensorData[5].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[5].value4, resolution) + ChannelData[5].Field4_Units + "</td>";
    if ((SensorData[6].sensornumber != 0) && (SensorData[6].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[6].value4, resolution) + ChannelData[6].Field4_Units + "</td>";
//    if ((SensorData[7].sensornumber != 0) && (SensorData[7].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[7].value4, resolution) + ChannelData[7].Field4_Units + "</td>";
//    if ((SensorData[8].sensornumber != 0) && (SensorData[8].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[8].value4, resolution) + ChannelData[8].Field4_Units + "</td>";
//    if ((SensorData[9].sensornumber != 0) && (SensorData[9].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[9].value4, resolution) + ChannelData[9].Field4_Units + "</td>";
//    if ((SensorData[10].sensornumber != 0) && (SensorData[10].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[10].value4, resolution) + ChannelData[10].Field4_Units + "</td>";
//    if ((SensorData[11].sensornumber != 0) && (SensorData[11].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[11].value4, resolution) + ChannelData[11].Field4_Units + "</td>";
//    if ((SensorData[12].sensornumber != 0) && (SensorData[12].sensornumber <= number_of_channels))  webpage += "<td>" + String(SensorData[12].value4, resolution) + ChannelData[12].Field4_Units + "</td>";
    webpage += F("</tr><tr><td>Updated</td>");
    if ((SensorData[0].sensornumber != 0) && (SensorData[0].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[0].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[1].sensornumber != 0) && (SensorData[1].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[1].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[2].sensornumber != 0) && (SensorData[2].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[2].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[3].sensornumber != 0) && (SensorData[3].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[3].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[4].sensornumber != 0) && (SensorData[4].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[4].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[5].sensornumber != 0) && (SensorData[5].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[5].readingtime).substring(0, 8) + "</td>";
    if ((SensorData[6].sensornumber != 0) && (SensorData[6].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[6].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[7].sensornumber != 0) && (SensorData[7].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[7].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[8].sensornumber != 0) && (SensorData[8].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[8].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[9].sensornumber != 0) && (SensorData[9].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[9].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[10].sensornumber != 0) && (SensorData[10].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[10].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[11].sensornumber != 0) && (SensorData[11].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[11].readingtime).substring(0, 8) + "</td>";
//    if ((SensorData[12].sensornumber != 0) && (SensorData[12].sensornumber <= number_of_channels)) webpage += "<td>" + Time(SensorData[12].readingtime).substring(0, 8) + "</td>";
    webpage += F("</tr></table></div>");
  }
  webpage += F("<hr /><br>");
  SendHTML_Content();
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DisplayLocations() {
#define resolution 1 // 1 = 20.2 or 2 = 20.23 or 3 = 20.234 displayed data if the sensor supports the resolution
  SendHTML_Header(refresh_on);
  webpage += F("<hr />");
  if (!ReceivedAnySensor()) {
    webpage += F("<h3>*** Waiting For Data Reception ***</h3>");
  }
  else
  {
    webpage += F("<h3>Current Sensor Readings</h3>");
    webpage += F("<div style='overflow-x:auto;'>");  // Add horizontal scrolling if number of fields exceeds page width
    webpage += F("<table style='width:100%'><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0 && SensorData[s].sensornumber <= number_of_channels)
      {
        webpage += F("<th style='text-align:center'>");
        webpage += ChannelData[SensorData[s].sensornumber].Name;
        webpage += F("</th>");
      }
    }
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0 && SensorData[s].sensornumber <= number_of_channels)
      {
        webpage += F("<td><div class='container'><img src='/");
        webpage += ChannelData[s].IconName; webpage += F("' width='90' height='90'></td></div>");
      }
    }
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0   && SensorData[s].sensornumber <= number_of_channels)
      {
        if (ChannelData[s].Field1 != "") {
          webpage += "<td style='text-align:center'>" + String(SensorData[s].value1, resolution) + ChannelData[s].Field1_Units + "</td>";
        }
      }
      else webpage += F("<td></td>");
    }
    SendHTML_Content();
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0   && SensorData[s].sensornumber <= number_of_channels)
      {
        if (ChannelData[s].Field2 != "") {
          webpage += "<td style='text-align:center'>" + String(SensorData[s].value2, resolution) + ChannelData[s].Field2_Units + "</td>";
        }
      }
      else webpage += F("<td></td>");
    }
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0   && SensorData[s].sensornumber <= number_of_channels)
      {
        if (ChannelData[s].Field3 != "") {
          webpage += "<td style='text-align:center'>" + String(SensorData[s].value3, resolution) + ChannelData[s].Field3_Units + "</td>";
        }
      }
      else webpage += F("<td></td>");
    }
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if (SensorData[s].sensornumber != 0   && SensorData[s].sensornumber <= number_of_channels) {
        if (ChannelData[s].Field4 != "")
        {
          webpage += "<td style='text-align:center'>" + String(SensorData[s].value4, resolution) + ChannelData[s].Field4_Units + "</td>";
        }
      } else webpage += F("<td></td>");
    }
    webpage += F("</tr><tr>");
    for (int s = 1; s <= number_of_channels; s++) {
      if ((SensorData[s].sensornumber != 0) && (SensorData[s].sensornumber <= number_of_channels))
        webpage += "<td style='text-align:center'>" + Time(SensorData[s].readingtime).substring(0, 8) + "</td>";
    }
    webpage += F("</tr></table></div>");
  }
  webpage += F("<hr /><br>");
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Auto_Update () { // Auto-refresh of the screen, this turns it on/off
  AUpdate = !AUpdate;
  HomePage();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
bool ReceivedAnySensor() {
  bool sensor_received = false;
  for (int s = 1; s <= number_of_channels; s++) {
    if ((SensorData[s].sensornumber != 0) && (SensorData[s].sensornumber <= number_of_channels)) {
      sensor_received = true;
    }
  }
  return sensor_received;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReadChannelData() {
  if (SD_present) { // If the SD-Card is present and board fitted then append the next reading to the log file called 'datalog.txt'
    File dataFile = SD.open("/chandata.txt", FILE_READ);
    int cn = 1;
    if (dataFile) { // if the file is available, read it
      String in_record;
      while (dataFile.available() && cn < number_of_channels) {
        // Note the trim function is essential for graphing to work! Fields are padded out with spaces otherwise, I don't know why...
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].ID           = in_record.toInt();
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Name         = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Description  = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Type         = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field1       = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field1_Units = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field2       = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field2_Units = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field3       = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field3_Units = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field4       = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Field4_Units = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].IconName     = in_record;
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Created      = in_record.toInt();
        in_record = dataFile.readStringUntil(','); in_record.trim(); ChannelData[cn].Updated      = in_record.toInt();
        cn++;
      }
    } else ReportSDNotPresent();
    dataFile.close();
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SaveChannelData() {
  if (SD_present) { // If the SD-Card is present and board fitted then append the next reading to the log file called 'datalog.txt'
    if (!SD.remove("/chandata.txt")) Serial.println(F("Failed to delete Channel Setting files"));
    File dataFile = SD.open("/chandata.txt", FILE_WRITE);
    if (dataFile) { // Save all possible channel data
      for (int cn = 1; cn < number_of_channels; cn++) {
        dataFile.println(String(ChannelData[cn].ID) + ",");
        dataFile.println(ChannelData[cn].Name + ",");
        dataFile.println(ChannelData[cn].Description + ",");
        dataFile.println(ChannelData[cn].Type + ",");
        dataFile.println(ChannelData[cn].Field1 + ",");
        dataFile.println(ChannelData[cn].Field1_Units + ",");
        dataFile.println(ChannelData[cn].Field2 + ",");
        dataFile.println(ChannelData[cn].Field2_Units + ",");
        dataFile.println(ChannelData[cn].Field3 + ",");
        dataFile.println(ChannelData[cn].Field3_Units + ",");
        dataFile.println(ChannelData[cn].Field4 + ",");
        dataFile.println(ChannelData[cn].Field4_Units + ",");
        dataFile.println(ChannelData[cn].IconName + ",");
        dataFile.println(String(ChannelData[cn].Created) + ",");
        dataFile.println(String(ChannelData[cn].Updated) + ",");
      }
    } else ReportSDNotPresent();
    dataFile.close();
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Channel_File_Stream() {
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("stream")) {
      if (server.arg(0) == String(number_of_channels)) SD_file_stream("chandata");
    }  else SD_file_stream(server.arg(0));
  }
  else SelectInput(refresh_off, "Channel File Stream", "Select a Channel to Stream", "Cstream", "stream", graph_on);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_stream(String filename) {
  if (SD_present) {
    File dataFile = SD.open("/" + filename + ".txt", FILE_READ); // Now read data from SD Card
    if (dataFile) {
      if (dataFile.available()) { // If data is available and present
        String dataType = "application/octet-stream";
        if (server.streamFile(dataFile, dataType) != dataFile.size()) {
          Serial.print(F("Sent less data than expected!"));
        }
      }
      dataFile.close(); // close the file:
    } else ReportFileNotPresent("Cstream");
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Channel_File_Download() {
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("download")) {
      if (server.arg(0) == String(number_of_channels)) SD_file_download("chandata", !open_download);
    } else SD_file_download(server.arg(0), !open_download);
  }
  else SelectInput(refresh_off, "Channel File Download", "Select a Channel to Download", "Cdownload", "download", graph_on);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Download() {
  if (server.args() > 0 ) { // Arguments were received
    Serial.println(server.arg(0));
    if (server.hasArg("odownload")) SD_file_download(server.arg(0), open_download);
  }
  else OpenSelectInput("Enter a File Name to Download", "Odownload", "odownload");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_download(String filename, bool download_mode) {
  if (SD_present) {
    File download = SD.open("/" + filename + (download_mode ? "" : ".txt"), FILE_READ); // Now read data from SD Card
    if (download) {
      if (!download_mode) filename += ".txt";
      server.sendHeader("Content-Type", "text/text");
      server.sendHeader("Content-Disposition", "attachment; filename=" + filename);
      server.sendHeader("Connection", "close");
      server.streamFile(download, "application/octet-stream");
      download.close();
    } else ReportFileNotPresent("Cdownload");
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Channel_File_Upload() {
  append_page_header(refresh_off);
  webpage += F("<h3>Select File to Upload</h3>");
  webpage += F("<FORM action='/upload' method='post' enctype='multipart/form-data'>");
  webpage += F("<input class='buttons' style='width:40%' type='file' name='upload' id = 'upload' value=''><br>");
  webpage += F("<br><button class='buttons' style='width:10%' type='submit'>Upload File</button><br>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer(graph_off);
  server.send(200, "text/html", webpage);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
File UploadFile;
void handleFileUpload() { // upload a new file to the Filing system
  HTTPUpload& uploadfile = server.upload(); // See https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/srcv
  // For further information on 'status' structure, there are other reasons such as a failed transfer that could be used
  if (uploadfile.status == UPLOAD_FILE_START)
  {
    String filename = uploadfile.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("Upload File Name: "); Serial.println(filename);
    SD.remove(filename);                         // Remove a previous version, otherwise data is appended the file again
    UploadFile = SD.open(filename, FILE_WRITE);  // Open the file for writing in SPIFFS (create it, if doesn't exist)
    filename = String();
  }
  else if (uploadfile.status == UPLOAD_FILE_WRITE)
  {
    if (UploadFile) UploadFile.write(uploadfile.buf, uploadfile.currentSize); // Write the received bytes to the file
  }
  else if (uploadfile.status == UPLOAD_FILE_END)
  {
    if (UploadFile)         // If the file was successfully created
    {
      UploadFile.close();   // Close the file again
      Serial.print("Upload Size: "); Serial.println(uploadfile.totalSize);
      webpage = "";
      append_page_header(refresh_off);
      webpage += F("<h3>File was successfully uploaded</h3>");
      webpage += F("<h2>Uploaded File Name: "); webpage += uploadfile.filename + "</h2>";
      webpage += F("<h2>File Size: "); webpage += file_size(uploadfile.totalSize) + "</h2><br>";
      append_page_footer(graph_off);
      server.send(200, "text/html", webpage);
    }
    else
    {
      ReportCouldNotCreateFile("Cupload");
    }
  }
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void Channel_File_Erase() {
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("erase")) {
      if (server.arg(0) == String(number_of_channels)) SD_file_erase("chandata", !open_erase);
    } else SD_file_erase(server.arg(0), !open_erase);
    ChannelDataReset(server.arg(0).toInt());
    SaveChannelData();
  }
  else SelectInput(refresh_off, "Channel File Erase", "Select a Channel to Erase", "Cerase", "erase", graph_off);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void File_Erase() {
  if (server.args() > 0 ) { // Arguments were received
    Serial.println(server.arg(0));
    if (server.hasArg("oerase")) SD_file_erase(server.arg(0), open_erase);
  }
  else OpenSelectInput("Enter a File Name to Erase", "Oerase", "oerase");
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_file_erase(String filename, bool erase_mode) { // Erase the datalog file
  if (SD_present) {
    append_page_header(refresh_off);
    webpage += F("<h3 class='rcorners_n'>Channel File Erase</h3>");
    File dataFile = SD.open("/" + filename + (erase_mode ? "" : ".txt"), FILE_READ); // Now read data from SD Card
    if (dataFile)
    {
      if (SD.remove("/" + filename + (erase_mode ? "" : ".txt"))) {
        Serial.println(F("File deleted successfully"));
        webpage += "<h3>FILE: '" + filename + (erase_mode ? "" : ".txt") + "' has been erased</h3>";
        if (erase_mode) webpage += F("<a href='/Oerase'>[Back]</a><br><br>"); else webpage += F("<a href='/Cerase'>[Back]</a><br><br>");
      }
      else
      {
        webpage += F("<h3>Channel File was not deleted - error</h3>");
        webpage += F("<a href='/Cerase'>[Back]</a><br><br>");
      }
    } else ReportFileNotPresent("Cerase");
    append_page_footer(graph_off);
    server.send(200, "text/html", webpage);
    webpage = "";
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SD_dir() {
  if (SD_present) {
    File root = SD.open("/");
    if (root) {
      root.rewindDirectory();
      SendHTML_Header(refresh_off);
      webpage += F("<h3 class='rcorners_m'>SD Card Contents</h3>");
      webpage += F("<table width='85%' align='center'>");
      webpage += F("<tr><th>Name/Type</th><th width='9%'>File/Dir</th><th width='12%'>Size</th></tr>");
      printDirectory("/", 0);
      webpage += F("</table><br>");
      SendHTML_Content();
      root.close();
    }
    else
    {
      SendHTML_Header(refresh_off);
      webpage += F("<h3>No Channel Files Found</h3>");
    }
    append_page_footer(graph_off);
    SendHTML_Content();
    SendHTML_Stop();   // Stop is needed because no content length was sent
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void printDirectory(const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);
  File root = SD.open(dirname);
#ifdef ESP8266
  root.rewindDirectory(); //Only needed for ESP8266
#endif
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }
  File file = root.openNextFile();
  while (file) {
    if (webpage.length() > 1000) {
      SendHTML_Content();
    }
    if (file.isDirectory()) {
      Serial.println(file.name());
      webpage += "<tr><td>" + String(file.isDirectory() ? "Dir" : "File") + "</td><td>" + String(file.name()) + "</td><td></td></tr>";
      printDirectory(file.name(), levels - 1);
    }
    else
    {
      webpage += "<tr><td>" + String(file.name()) + "</td>";
      webpage += "<td>" + String(file.isDirectory() ? "Dir" : "File") + "</td>";
      webpage += "<td>" + file_size(file.size()) + "</td></tr>";
    }
    file = root.openNextFile();
  }
  file.close();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(int bytes) {
  String fsize = "";
  if (bytes < 1024)                     fsize = String(bytes) + " B";
  else if (bytes < (1024 * 1024))        fsize = String(bytes / 1024.0, 3) + " KB";
  else if (bytes < (1024 * 1024 * 1024)) fsize = String(bytes / 1024.0 / 1024.0, 3) + " MB";
  else                                  fsize = String(bytes / 1024.0 / 1024.0 / 1024.0, 3) + " GB";
  return fsize;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void DrawChart() {
  graph_start = 0;
  graph_end   = display_records;
  if (server.args() > 0 ) { // Arguments were received
    if (server.hasArg("draw"))  drawchart(server.arg(0));  // parameter is sensor number
  }
  else SelectInput(refresh_off, "Channel Graph", "Select a Channel to Graph", "chart", "draw", graph_off);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void drawchart(String filename) { // required to enable parameters with the call
  graph_filename = filename;     // global variable requirement
  channel_number = filename.toInt();
  readingCnt     = CountFileRecords(filename);
  GetandGraphData();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void GetDataForGraph(String filename, int start) {
  int recordCnt = 0, displayCnt = 0;
  String stype;
  graph_sensor = filename.toInt();
  if (SD_present) {
    File dataFile = SD.open("/" + filename + ".txt", FILE_READ); // Now read data from SD Card
    if (dataFile) {
      while (dataFile.available() && displayCnt < display_records) { // if the file is available, read from it
        DisplayData[displayCnt].ltime  = dataFile.parseInt();        // 1517052559 21.58 43.71 0.00  0.00  SHT30-1 typically
        DisplayData[displayCnt].field1 = dataFile.parseFloat();
        DisplayData[displayCnt].field2 = dataFile.parseFloat();
        DisplayData[displayCnt].field3 = dataFile.parseFloat();
        DisplayData[displayCnt].field4 = dataFile.parseFloat();
        stype                          = dataFile.readStringUntil('\n'); // Needed to complete a record read
        if (recordCnt >= start) displayCnt++;
        recordCnt++;
      }
    } else ReportFileNotPresent("chart");
    dataFile.close();
    if (recordCnt == 0)    displayCnt = 1; // In case the file is empty
    if (start > recordCnt) displayCnt = recordCnt; // In case the file is empty or there were not enough records.
    graph_start = start;
    graph_end   = displayCnt; // Number of records to display
  } else ReportSDNotPresent();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MoveChartForward() {
  graph_start += graph_step;
  graph_end   += graph_step;
  if (graph_end > readingCnt) {
    graph_end   = readingCnt;
    graph_start = readingCnt - display_records;
    if (graph_start < 0 ) graph_start = 0;
  }
  if ((graph_start > readingCnt - graph_step) && (readingCnt - graph_step > 0) ) graph_start = readingCnt - graph_step;
  GetandGraphData();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void MoveChartBack() {
  graph_start -= graph_step;
  graph_end   -= graph_step;
  if (graph_start < 0) graph_start = 0;
  GetandGraphData();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void GetandGraphData() {
  GetDataForGraph(graph_filename, graph_start);
  GraphSelectedData(DisplayData,
                    graph_start,
                    graph_end,
                    ChannelData[graph_sensor].Name);
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ConstructGraph(record_type displayed_SensorData[], int start, int number_of_records, String title, String ytitle, String yunits, String graphname) {
  webpage = "";
  // https://developers.google.com/loader/ // https://developers.google.com/chart/interactive/docs/basic_load_libs
  // https://developers.google.com/chart/interactive/docs/basic_preparing_data
  // https://developers.google.com/chart/interactive/docs/reference#google.visualization.arraytodatatable and See appendix-A
  // data format is: [field-name,field-name,field-name] then [data,data,data], e.g. [12, 20.5, 70.3]
  webpage += F("<script type=\"text/javascript\" src=\"https://www.google.com/jsapi?autoload={'modules':[{'name':'visualization','version':'1','packages':['corechart']}]}\"></script>");
  webpage += F("<script type=\"text/javascript\"> google.setOnLoadCallback(drawChart);");
  webpage += F("function drawChart() {");
  webpage += F("var data = google.visualization.arrayToDataTable(");
  webpage += F("[['R','"); webpage += ytitle + "'],";
  SendHTML_Content();
  for (int i = 0; i < number_of_records; i = i + 1) {
    if      (graphfield == one)   {
      webpage += F("[");
      webpage += i + start;
      webpage += "," + String(float(displayed_SensorData[i].field1), 1) + "],";
    }
    else if (graphfield == two)   {
      webpage += F("[");
      webpage += i + start;
      webpage += "," + String(float(displayed_SensorData[i].field2), 1) + "],";
    }
    else if (graphfield == three) {
      webpage += F("[");
      webpage += i + start;
      webpage += "," + String(float(displayed_SensorData[i].field3), 1) + "],";
    }
    else if (graphfield == four)  {
      webpage += F("[");
      webpage += i + start;
      webpage += "," + String(float(displayed_SensorData[i].field4), 1) + "],";
    }
    if (webpage.length() > 1500) {
      SendHTML_Content(); // Send interim results, on the ESP32 it can't hold much content unlike the ESP8266!
    }
  }
  webpage += F("]); var options={");
  SendHTML_Content(); // Send the residual
  webpage += "title:'" + title; webpage += F("',titleTextStyle:{fontName:'Arial',fontSize:14,color:'Maroon'},");
  webpage += F("legend:{position:'bottom'},width:'50%',height:375,colors:['"); webpage += graphcolour; webpage += F("'],backgroundColor:'#F7F2Fd',");
  webpage += F("hAxis:{viewWindow:{min:");
  webpage += graph_start;
  webpage += F(",max:");
  webpage += graph_start + graph_end;
  webpage += F("},titleTextStyle:{color:'Purple',bold:true,fontSize:14},showTextEvery:1},");
  webpage += F("vAxes:");
  webpage += F("{0:{viewWindowMode:'explicit',gridlines:{color:'black'},title:'"); webpage += ytitle + yunits + "',format:'##.##'},},";
  webpage += F("series:{0:{targetAxisIndex:0},1:{targetAxisIndex:1},},curveType:'none'};");
  webpage += F("var chart = new google.visualization.LineChart(document.getElementById('"); webpage += graphname; webpage += F("'));chart.draw(data,options);");
  webpage += F("}");
  webpage += F("</script>");
  SendHTML_Content();
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void GraphSelectedData(record_type displayed_SensorData[],
                       int start,
                       int number_of_records,
                       String title)
{
  // See google charts api for more details. To load the APIs, include the following script in the header of your web page.
  // <script type="text/javascript" src="https://www.google.com/jsapi"></script>
  // See https://developers.google.com/chart/interactive/docs/basic_load_libs
  bool graph1_on = false, graph2_on = false, graph3_on = false, graph4_on = false;
  String ytitle, yunits;
  SendHTML_Header(refresh_off);
  webpage += F("<div><h1 class='rcorners_m'>"); webpage += ChannelData[graph_sensor].Name + " (" + String(readingCnt) + "-readings)</h1>";
  SendHTML_Content();
  if (ChannelData[channel_number].Field1 != "") {
    graphfield  = one;
    graphcolour = "red";
    ytitle = ChannelData[graph_sensor].Field1;
    yunits = ChannelData[graph_sensor].Field1_Units;
    ConstructGraph(displayed_SensorData, start, number_of_records, title + " " + ytitle, ytitle, yunits, "graph1"); // 'graph1' is e.g. the tag to match the <div id=> statements below
    graph1_on = true;
  }
  //-----------------
  if (ChannelData[channel_number].Field2 != "") {
    graphfield  = two;
    graphcolour = "blue";
    ytitle = ChannelData[graph_sensor].Field2;
    yunits = ChannelData[graph_sensor].Field2_Units;
    ConstructGraph(displayed_SensorData, start, number_of_records, title + " " + ytitle, ytitle, yunits, "graph2");
    graph2_on = true;
  }
  //-----------------
  if (ChannelData[channel_number].Field3 != "") {
    graphfield  = three;
    graphcolour = "green";
    ytitle = ChannelData[graph_sensor].Field3;
    yunits = ChannelData[graph_sensor].Field3_Units;
    ConstructGraph(displayed_SensorData, start, number_of_records, title + " " + ytitle, ytitle, yunits, "graph3");
    graph3_on = true;
  }
  //-----------------
  if (ChannelData[channel_number].Field4 != "") {
    graphfield  = four;
    graphcolour = "orange";
    ytitle = ChannelData[graph_sensor].Field4;
    yunits = ChannelData[graph_sensor].Field4_Units;
    ConstructGraph(displayed_SensorData, start, number_of_records, title + " " + ytitle, ytitle, yunits, "graph4");
    graph4_on = true;
  }
  //-----------------
  webpage += F("<div>");
  webpage += F("<div class='row'>");
  if (graph1_on) webpage += F("<div id='graph1' class='column'></div>");
  if (graph2_on) webpage += F("<div id='graph2' class='column'></div>");
  if (graph3_on) webpage += F("<div id='graph3' class='column'></div>");
  if (graph4_on) webpage += F("<div id='graph4' class='column'></div>");
  webpage += F("</div>");
  webpage += F("</div><a href='/chart'>[Back]</a><br>");
  SendHTML_Content(); // Send Content
  append_page_footer(graph_on);
  SendHTML_Content(); // Send footer
  SendHTML_Stop();    // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Header(bool refresh_mode) {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Connection", "Keep-Alive");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header
  append_page_header(refresh_mode);
  server.sendContent(webpage);
  server.sendContent("\n\r"); // A blank line seperates the header
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Content() {
  server.sendContent(webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SendHTML_Stop() {
  server.sendContent("");
  server.client().stop(); // Stop is needed because no content length was sent
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SelectInput(bool refresh_mode, String heading1, String heading2, String command, String arg_calling_name, bool graph_mode_off) {
  SendHTML_Header(refresh_off);
  webpage += F("<h3>"); webpage += heading2 + "</h3>";
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments
  for (byte cn = 1; cn < number_of_channels; cn++) {
    webpage += F("<button class='buttons' style='width:15%' type='submit' name='"); webpage += arg_calling_name + "' value='" + String(cn) + "'>" + ChannelData[cn].Name + "</button>";
  }
  if (graph_mode_off) {
    webpage += F("<button class='buttons' style='width:15%' type='submit' name='");
    webpage += arg_calling_name + "' value='chandata'>Settings</button>";
  }
  webpage += F("<br><br>");
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void OpenSelectInput(String heading1, String command, String arg_calling_name) {
  SendHTML_Header(refresh_off);
  webpage += F("<h3>"); webpage += heading1 + "</h3>";
  webpage += F("<FORM action='/"); webpage += command + "' method='post'>"; // Must match the calling argument e.g. '/chart' calls '/chart' after selection but with arguments!
  webpage += F("<input type='text' name='"); webpage += arg_calling_name; webpage += F("' value=''><br>");
  webpage += F("<type='submit' name='"); webpage += arg_calling_name; webpage += F("' value=''><br><br>");
  append_page_footer(graph_off);
  SendHTML_Content();
  SendHTML_Stop();
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String file_size(String filename) { // Display file size of the datalog file
  String ftxtsize = "";
  if (SD_present) {
    File dataFile = SD.open("/" + filename + ".txt", FILE_READ); // Now read data from SD Card
    if (dataFile) {
      int bytes = dataFile.size();
      if (bytes < 1024)                     ftxtsize = String(bytes) + " B";
      else if (bytes < (1024 * 1024))        ftxtsize = String(bytes / 1024.0) + " KB";
      else if (bytes < (1024 * 1024 * 1024)) ftxtsize = String(bytes / 1024.0 / 1024.0) + " MB";
      else                                  ftxtsize = String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
    }
    else ftxtsize = "";
    dataFile.close();
    return ftxtsize;
  } else ReportSDNotPresent();
  return "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportSDNotPresent() {
  append_page_header(refresh_off);
  webpage += F("<h3>No SD Card present</h3>");
  webpage += F("<a href='/'>[Back]</a><br><br>");
  append_page_footer(graph_off);
  server.send(200, "text/html", webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportFileNotPresent(String target) {
  append_page_header(refresh_off);
  webpage += F("<h3>File does not exist</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer(graph_off);
  server.send(200, "text/html", webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCommandNotFound(String target) {
  append_page_header(refresh_off);
  webpage += F("<h3>Function does not exist</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer(graph_off);
  server.send(200, "text/html", webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ReportCouldNotCreateFile(String target) {
  append_page_header(refresh_off);
  webpage += F("<h3>Could Not Create Uploaded File (write-protected?)</h3>");
  webpage += F("<a href='/"); webpage += target + "'>[Back]</a><br><br>";
  append_page_footer(graph_off);
  server.send(200, "text/html", webpage);
  webpage = "";
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int CountFileRecords(String filename) {
  int recordCnt = 0, temp_read_int = 0;
  float  temp_read_flt = 0;
  String temp_read_txt = "";
  if (SD_present) {
    File dataFile = SD.open("/" + filename + ".txt", FILE_READ); // Now read data from SD Card
    if (dataFile) {
      while (dataFile.available()) { // if the file is available, read from it
        recordCnt++;
        temp_read_int = dataFile.parseInt();            // 1517052559 21.58 43.71 0.00  0.00  SHT30-1 typically
        temp_read_flt = dataFile.parseFloat();
        temp_read_flt = dataFile.parseFloat();
        temp_read_flt = dataFile.parseFloat();
        temp_read_flt = dataFile.parseFloat();
        temp_read_txt = dataFile.readStringUntil('\n'); // Needed to complete a record read
      }
    }
    dataFile.close();
    return recordCnt;
  } else ReportSDNotPresent();
  return 0;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void SetupTime() {
  timeval tv  = {BaseTime, 0 }; // 00:00:00 01/01/2018
  timezone tz = { 0 , 0 };
  settimeofday(&tv, &tz);
  configTime(0, 0, "pool.ntp.org");
  setenv("TZ", "GMT0BST,M3.5.0/2,M10.5.0/2", 1);
  tzset();
  time_t tnow = time(nullptr);
  delay(2000);
  Serial.print(F("\nWaiting for time..."));
  tnow = time(nullptr);
  Serial.println("Time set " + Time(tnow));
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
String Time(int unix_time) {
  struct tm *now_tm;
  int hour, min, second, day, month, year;
  // timeval tv = {unix_time,0};
  time_t tm = unix_time;
  now_tm = localtime(&tm);
  hour   = now_tm->tm_hour;
  min    = now_tm->tm_min;
  second = now_tm->tm_sec;
  day    = now_tm->tm_mday;
  month  = now_tm->tm_mon + 1;
  year   = 1900 + now_tm->tm_year; // To get just YY information
  //String days[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
  time_str =  (hour < 10 ? "0" + String(hour) : String(hour)) + ":" + (min < 10 ? "0" + String(min) : String(min)) + ":" + (second < 10 ? "0" + String(second) : String(second)) + "-";
  time_str += (day < 10 ? "0" + String(day) : String(day)) + "/" + (month < 10 ? "0" + String(month) : String(month)) + "/" + (year < 10 ? "0" + String(year) : String(year)); // HH:MM:SS  05/07/17
  //Serial.println(time_str);
  return time_str;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
int TimeNow() {
  time_t tnow = time(nullptr);
  return tnow;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
void ChannelDataReset(byte CN) {
  ChannelData[CN].ID           = CN;
  ChannelData[CN].Name         = "Name-TBA";
  ChannelData[CN].Description  = "Sensor Readings";
  ChannelData[CN].Type         = "e.g.SHT30";
  ChannelData[CN].Field1       = "e.g.Temperature";
  ChannelData[CN].Field1_Units = "°C";
  ChannelData[CN].Field2       = "e.g.Humidity";
  ChannelData[CN].Field2_Units = "%";
  ChannelData[CN].Field3       = "e.g.Pressure";
  ChannelData[CN].Field3_Units = "hPa";
  ChannelData[CN].Field4       = "Unused";
  ChannelData[CN].Field4_Units = "";
  ChannelData[CN].IconName     = "building.png";
  ChannelData[CN].Created      = BaseTime;
  ChannelData[CN].Updated      = BaseTime;
}
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Read file from SD Card and display it
bool loadFromSdCard(String filename) {
  String dataType = "text/plain";
  if (filename.endsWith(".htm"))  dataType = "text/html";
  else if (filename.endsWith(".html")) dataType = "text/html";
  else if (filename.endsWith(".css"))  dataType = "text/css";
  else if (filename.endsWith(".png"))  dataType = "image/png";
  else if (filename.endsWith(".gif"))  dataType = "image/gif";
  else if (filename.endsWith(".jpg"))  dataType = "image/jpeg";
  else if (filename.endsWith(".bmp"))  dataType = "image/bmp";
  else if (filename.endsWith(".ico"))  dataType = "image/x-icon";
  Serial.println(filename);
  File dataFile = SD.open(filename.c_str());
  if (!dataFile) return false;
  if (server.hasArg("download")) dataType = "application/octet-stream";
  if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    Serial.println("Sent less data than expected!");
  }
  dataFile.close();
  return true;
}
