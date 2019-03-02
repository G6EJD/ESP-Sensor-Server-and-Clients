#define ServerVersion "1.0"
String  webpage = "";
bool    AUpdate = true; // Default value is On
const byte number_of_channels = 7;     // **** MAXIMUM Of 12 and ensure this is the required Number of Channels + 1 e.g. for 6 channels set this value to 7
// NOTE: ******************* FOR EACH increase or decrease in number_of_channels change lines 441 onwards accordingly, otherwise there will be compilation errors out-of-bounds
// This is until the compiler errors can be fixed. Currently 1.0.0 for ESP32

#define display_records     500        // 500 is the maximum of readings that can be displayed on the graph
#define graph_step          250        // The amount the graph is moved forwards/backwards
#define BaseTime            1514764800 // 00:00:00 01/01/2018
#ifdef ESP8266
#define SD_CS_pin           D8         // The pin on Wemos D1 Mini for SD Card Chip-Select
#else
#define SD_CS_pin           5          // The pin on MH-T Live ESP32 (version of Wemos D1 Mini) for SD Card Chip-Select
#endif
String  time_str;
bool    refresh_on       = true;
bool    refresh_off      = false;
bool    data_amended     = false;
String  graph_filename   = "";
bool    graph_on         = true;
bool    graph_off        = false;
bool    open_erase       = true; // Enables only Channel Data files to be deleted
bool    open_download    = true; // Enables only Channel Data files to be downloaded
enum    fieldselector    {one, two, three, four};
String  graphcolour      = "red";
int     readingCnt       = 0;
fieldselector graphfield = one;  

///////////////////////////////////////////////////////////////////////////////////////
typedef struct {
  int    ID           = 0;
  String Name         = "Name-TBA";
  String Description  = "Sensor Readings";
  String Type         = "SHT30";
  String Field1       = "Temperature";
  String Field1_Units = "°C";
  String Field2       = "Humidity";
  String Field2_Units = "%";
  String Field3       = "Pressure";
  String Field3_Units = "hPa";
  String Field4       = "Unused";
  String Field4_Units = "";
  String IconName     = "building.png";
  int    Created      = BaseTime;
  int    Updated      = BaseTime;
} sensor_details_type;

sensor_details_type ChannelData[number_of_channels];

typedef struct { 
  byte   sensornumber;        // Sensor number provided by e.g. Sensor=3
  float  value1;              // For example Temperature
  float  value2;              // For example Humidity
  float  value3;              // For example Pressure
  float  value4;              // Spare
  String sensortype  = "N/A"; // The sensor type e.g. an SHT30 or BMP180
  int    readingtime = BaseTime; 
} sensor_record_type; // total bytes per record = (1+4+4+4+4+String(6)+4) = 27Bytes ~ 4-years of records/MByte at 24 readings/day

sensor_record_type SensorData[number_of_channels]; // Define the data array 

int    channel_number = 0;
int    sensor_reading = 0;   // Default value
int    day_count      = 1;   // Default value
int    graph_sensor   = 1;
bool   SD_present;

int graph_start, graph_end, index_ptr=0;

typedef struct {
  int    ltime;   // Time reading arrived
  float  field1;  // usually Temperature values
  float  field2;  // usually Humidity values
  float  field3;  // usually Pressure values
  float  field4;  // spare
} record_type;

record_type DisplayData[display_records]; // Define the data array for display on a graph
