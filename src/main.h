/*
 *  Created on: 4 Apr 2019
 *      Author: mikes
 */


#ifdef _MAIN_
#include "creds.h"
// local gateway GPS data
// Wifi Creds
// const char* ssid     = "VodafoneConnect96376469";
//const char* password = "58xdlm9ddipa8dh";

WiFiClient espClient;

//static const int RXPin = 4, TXPin = 5;
static const uint32_t GPSBaud = 9600;
char rxBuffer[256];
char txBuffer[256];

// general loop control
int timer  = millis(); // get current timer
int onesec = 0;
int elapsed = 0;
int newtimer = timer+10;
int gpsloopcnt=0;

// display 
char disp_str[20];

// fixed gps data for home initialisation
float gps_lat = 53.226849;
float gps_lon =-2.506535;
time_t gps_time =1562694930;

// Lora values
int payload = 0;
void onReceive(int packetSize);
int flightCount=0;
bool rxDone = false;
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

long habhubCounter = 0;
long habhubDelayMS = 10000;  // minimum time between habhub updates

#else


extern int SentenceCounter;
extern unsigned char packet[];                 // used to send arbitrary LoRa Messages for diagnostics and gen info
extern long currentFrq ;   
#endif

void display_init(void);
void display_gps(void);
void getTelemetryData(char *rxptr);
void setLoraMode(int mode);
void tuneFrq(void);
String remoteProcessor(const String& var);
String localProcessor(const String& var);
char checkButton();



