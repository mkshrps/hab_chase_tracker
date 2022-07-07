/*
 * miniLoraTrackerGPS.h
 *
 *  Created on: 4 Apr 2019
 *      Author: mikes
 */

#ifdef _MAIN_

// local gateway GPS data
struct TGPS
{
  time_t  lastPacketAt;
  uint32_t time; 
  uint8_t Hours, Minutes, Seconds;
  unsigned long SecondsInDay;         // Time in seconds since midnight
  double Longitude, Latitude;
  long Altitude;
  unsigned int Satellites;
  unsigned int failedCS;
  int Speed;
  int Direction;
  byte FixType;
  bool isValid;
} GPS;

struct remoteT
{
  char callSign[12];
  long  lastPacketAt;
  double longitude;
  double latitude;
  float alt;
  double  courseTo;
  double  distancem;
  char  cardinalCourseTo[20];
  float temperature_c;
  float humidity;
  bool  active;
  int satellites;
  int hours;
  int minutes;
  int seconds;
  int flightCount;
  float InternalTemperature;
  float BatteryVoltage;
  float ExternalTemperature;
  float Pressure;
  unsigned int BoardCurrent;
  unsigned int errorstatus;
  byte FlightMode;
  byte PowerMode;
  bool isValid;
  int rssi;
  char time[12];
  int speed;
} remote_data;

// Wifi Creds
// const char* ssid     = "VodafoneConnect96376469";
//const char* password = "58xdlm9ddipa8dh";
const char* ssid     = "Mike's iPhone SE";
const char* password = "MjKmJe6360";
WiFiClient espClient;

//static const int RXPin = 4, TXPin = 5;

static const uint32_t GPSBaud = 9600;
char rxBuffer[256];
char txBuffer[250];
int rssi = 0;
int snr = 0;
byte to_node = 1;
byte from_node = 10;
byte ID = 10;
byte flags = 0;
int timer  = millis(); // get current timer
int onesec = 0;
int elapsed = 0;
int newtimer = timer+10;
char disp_str[20];

String antenna("whip");
String radio("LoRa RFM95W");

float gps_lat = 53.226849;
float gps_lon =-2.506535;
time_t gps_time =1562694930;

// Lora values
int payload = 0;
void onReceive(int packetSize);
const char *gatewayID = "MJS01";
int flightCount=0;
bool rxDone = false;
int loopcnt=0;
int msgCount = 0;
bool resetFrq = false;
// current lora frequency
long currentFrq ;   
long lastFrqOK ; 

// Set LED GPIO
const int ledPin = 2;
// Stores LED state
String ledState;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// The TinyGPS++ object
//TinyGPSPlus gps;
// The serial connection to the GPS device
// SoftwareSerial ss(RXPin, TXPin);

long habhubCounter = 0;
long habhubDelayMS = 10000;  // minimum time between habhub updates

#else

extern struct TGPS
{
  time_t  lastPacketAt;
  uint32_t  time;
  uint8_t Hours, Minutes, Seconds;
  unsigned long SecondsInDay;         // Time in seconds since midnight
  double Longitude, Latitude;
  long Altitude;
  unsigned int Satellites;
  unsigned int failedCS;
  int Speed;
  int Direction;
  byte FixType;
  } GPS;

extern int SentenceCounter;
extern unsigned char packet[];                 // used to send arbitrary LoRa Messages for diagnostics and gen info
extern struct remoteT
{
  char callSign[12];
  long  lastPacketAt;
  double longitude;
  double latitude;
  float alt;
  double  courseTo;
  double  distancem;
  char  cardinalCourseTo[20];
  float temperature_c;
  float humidity;
  bool  active;
  int satellites;
  int hours;
  int minutes;
  int seconds;
  int flightCount;
  float InternalTemperature;
  float BatteryVoltage;
  float ExternalTemperature;
  float Pressure;
  unsigned int BoardCurrent;
  unsigned int errorstatus;
  byte FlightMode;
  byte PowerMode;
  bool isValid;
  int rssi;
  char time[12];
} remote_data;

#endif

void display_init(void);
void display_gps(void);
void getTelemetryData(char *rxptr);
void setLoraMode(int mode);
void tuneFrq(void);
String processor(const String& var);



