# ESP-Sensor-Server-Client
A sensor server that receives and displays on a webpage client data, usually from an ESP configured as a sensor

1. Download the files to your IDE location.
2. Locate the files referenced in the Server and Clients, download those and place in your Libraries folder
3. Choose an IP address for your Server e.g. 192.168.0.99
4. Edit the Server IP address accordingly.
5. Test the Server by complining and uploading to either an ESP8266 or ESP32, the code adapts accordingly. Make sure you choose the correct board type!
6. Test the server by typeing the a browser address bar:
http://sensorserver.local/sensor?Sensor=1&temperature=21.2&humidity=50.1&pressure=1001&spare=0&sensortype="Mine"

Or if your PC does not have 'Bonjour' installed, this is needed to resolve mult-cast DNS packets and resolves the address sesnorserver.local to the IP address, in which case enter in the browser address bar:

http://192.168.0.99/sensor?Sensor=1&temperature=21.2&humidity=50.1&pressure=1001&spare=0&sensortype="Mine"

Adjust the names and types and units as desired.
