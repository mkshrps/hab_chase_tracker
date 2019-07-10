/*******************************************
 * LoRa Receiver with OLED support
 * */

#define _MAIN_
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <TinyGPSPlus.h>
//#include <SoftwareSerial.h>
#include <U8x8lib.h>
#include "main.h"
#include <WiFi.h>
#include <habhub.h>
#include "calc_crc.h"
#include "mjswifi.h"

#include <LoRa.h>
#define BAND    4345E5  //you can set band here directly,e.g. 868E6,915E6
// define this if running on heltec board comment out if not

void onReceive(int packetSize);
#define DOIT_ESP32
//#define HELTEC_LORA

#ifdef HELTEC_LORA
#define SS      18
#define RST     14
#define DIO     26
#define MISO    19
#define MOSI    27
#define SPICLK  5  
#endif
#ifdef DOIT_ESP32
#define SS 5
#define RST 14
#define DIO 4
#define MISO    19
#define MOSI    23
#define SPICLK  18
#endif

#define _EP32_

// Define proper RST_PIN if required.
#define RST_PIN -1

#define LED_WARN            5
#define LED_OK              6
#define A0_MULTIPLIER      4.9

// define for u8x8 ascii library
#define I2C_ADDRESS 0x3C

void onReceive(int packetSize);

// Wifi Creds
const char* ssid     = "VodafoneConnect96376469";
const char* password = "58xdlm9ddipa8dh";
//const char* ssid     = "Mike's iPhone SE";
//const char* password = "MjKmJe6360";
WiFiClient espClient;

//static const int RXPin = 4, TXPin = 5;

static const uint32_t GPSBaud = 9600;

const char *habID = "MWH01";

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
int flightCount=0;
bool rxDone = false;

// OLED display device 
#ifdef HELTEC_LORA
  U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 04,/*reset*/ 16);
#else
  U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 21, /* data=*/ 22);
#endif


// current lora frequency
long currentFrq = 4345E5;    


// The TinyGPS++ object
//TinyGPSPlus gps;
// The serial connection to the GPS device
// SoftwareSerial ss(RXPin, TXPin);

  String antenna("whip");
  String radio("LoRa RFM95W");

  float gps_lat = 53.226849;
  float gps_lon =-2.506535;
  time_t gps_time =1562694930;


void setup()
{
  u8x8.begin();
  //u8x8.setFont(utf8font10x16);
  //Wire.begin();
  // Wire.setClock(400000L);
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  Serial.begin(115200);
  delay(1000);  
  // ss.begin(GPSBaud);
  Serial.println("Init Lora");

  u8x8.clear();
  u8x8.println("Pico Track 1");
  u8x8.println("MJS Technology");
  u8x8.println("\nlora 434.5Mhz: ");
  
  delay(2000); // Pause for 2 seconds

  Serial.print("Display Started 1");
  delay(2000);
  
  display_init();
  // start the wifi connection
  connectToWifi(espClient,ssid,password);

  // upload listener location to HABHUB
  uploadListenerPacket("MWH01", gps_time, gps_lat, gps_lon, antenna.c_str(),radio.c_str());



  SPI.begin(SPICLK,MISO,MOSI,SS);    // wrks for ESP32 DOIT board 
  LoRa.setPins(SS,RST,DIO);
  if(!LoRa.begin(434503000)){
  Serial.println("Lora not detected");

}

  LoRa.setSpreadingFactor(7);
  LoRa.setSignalBandwidth(20.8E3);
  LoRa.setCodingRate4(8);
  //LoRa.setPreambleLength(preambleLength);
  LoRa.setSyncWord(0x12);
  LoRa.crc();
  LoRa.setTxPower(15,PA_OUTPUT_PA_BOOST_PIN);
  
  LoRa.dumpRegisters(Serial);


  // put your setup code here, to run once:
  //LoRa.onReceive(onReceive);

  // put the radio into receive mode
  //LoRa.receive();


  time_t rawtime;
  struct tm * timeinfo;
  int year, month ,day;
  year = 2019;
  month = 4;
  day = 3;

  /* get current timeinfo and modify it to the user's choice */
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  timeinfo->tm_year = 2019;
  timeinfo->tm_mon = month;
  timeinfo->tm_mday = day;

  /* call mktime: timeinfo->tm_wday will be set */
  mktime ( timeinfo );

}

void  readPacketsLoop(){
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    onReceive(packetSize);
    long freqErr = LoRa.packetFrequencyError();
    if(abs(freqErr) > 500){
      Serial.println("FRQ was: ");
      Serial.println(currentFrq);
      Serial.println("FRQ Error: ");
      Serial.println(freqErr);

      if(freqErr > 0){
        currentFrq -= abs(freqErr);
      }
      else{
        currentFrq += abs(freqErr);
      }

      LoRa.setFrequency(currentFrq);
      Serial.println("Correcting FRQ to: ");
      Serial.println(currentFrq);
      remote_data.active = true;
      remote_data.lastPacketAt = millis();
    }
  }
  else{
    if((millis() - remote_data.lastPacketAt) >= 20000){
      remote_data.active=false;
      remote_data.rssi = 0;

    }
  }
}

void onReceive(int packetSize)
{
  if(packetSize <4){
    //Serial.println("****Invalid packet****");
    return;
  }

  // received a packet
  Serial.println("");
  Serial.print("Received packet ");
  memset(rxBuffer,0,sizeof(rxBuffer));

  // read packet
  for (int i = 0; i < packetSize; i++)
  {
    rxBuffer[i] = (char)LoRa.read(); 
    //Serial.print(rxBuffer[i]);

  }
  
  // print RSSI of packet
  Serial.print(" RSSI ");
  remote_data.rssi = LoRa.packetRssi(); 
  Serial.println(remote_data.rssi);

  rxDone = true;
}



void loop()
{

    struct tm currentTime;
    currentTime.tm_hour = 12;
    currentTime.tm_min = 0;
    currentTime.tm_sec = 0;
    currentTime.tm_year = 2019;
    currentTime.tm_mon = 3;
    currentTime.tm_mday = 5;
    String antenna("whip");

    delay(10);
    readPacketsLoop();

    float gps_lat = 53.226849;
    float gps_lon =-2.506535;
    time_t gps_time =mktime(&currentTime);


      // Process message
      if(rxDone==true){
        Serial.println("Message Received");
        Serial.print("This Node=> ");
        Serial.print(rxBuffer[0],DEC);
        Serial.print(" Gateway Node=> ");
        Serial.print(rxBuffer[1],DEC);
        Serial.print(" Gateway ID=> ");
        Serial.print(rxBuffer[2],DEC);
        Serial.print(" Gateway Flags=> ");
        Serial.print(rxBuffer[3],HEX);
        Serial.println("");
        Serial.print(" Bytes Received=> ");
        
        // if no gps data at remote station just display a waiting message
        if(rxBuffer[4]=='$' && rxBuffer[5]=='?'){

          int remoteID = (int) rxBuffer[1];
          
          Serial.println("Remote Station " + String(remoteID) + " Active");
         }

        // data ok
        else {

          //Data Received
          char * rxptr;
          // extract the values
          rxptr = &rxBuffer[4];
          
          // just test CRC function 'habitat' should checksub 0x3EFB
          //char testBuffer[100];
          //strcpy(testBuffer,"habitat");
          //calcCRC(testBuffer);
          //Serial.println(testBuffer);

          // prep for habhub

          BuildSentence(txBuffer,rxptr,sizeof(txBuffer),habID);
          Serial.println(txBuffer);
          flightCount++;
          // send to habhub
          uploadTelemetryPacket( txBuffer , flightCount , (char *) habID );

          Serial.println(rxptr);
          // process received data and store as variables
          rxptr++;
          // initialise strtok()
          char* strval = strtok(rxptr, ",");
          int valcount = 0;
          double val = 0.0;
          int ival = 0;
          // now loop round the substrings  
          while(strval != 0)
          {
            switch(valcount){
              case 0:
              val = atof(strval);
              remote_data.longitude = val;
              Serial.print(val,6);
              Serial.print(":");    
              break;

              case 1:
              val = atof(strval);
              remote_data.latitude = val;
              Serial.print(val,6);
              Serial.print(":");    
              // Find the next command in input string
              break;
              
              case 2:
              val = atof(strval);
              remote_data.alt = val;
              Serial.print(val);
              Serial.print(":");    
              break;

              case 3:
              // Temp data
              ival = atoi(strval);
              remote_data.satellites = ival;
              Serial.print(ival,2);
              Serial.print(":");    
              break;
              
              case 4:
              // hours
              ival = atoi(strval);
              remote_data.hours = ival;
              Serial.print(ival,2);
              break;
              
              case 5:
              // mins
              ival = atoi(strval);
              remote_data.minutes = ival;
              Serial.print(ival,2);
              break;
              
              case 6:
              // seconds
              ival = atoi(strval);
              remote_data.seconds = ival;
              Serial.print(ival,2);
              break;

              case  7:
              // flightCount
              val = atoi(strval);
              remote_data.flightCount = val;
              Serial.print(val,DEC);
              break;
            
            }
            valcount ++;
            // next substring
            strval = strtok(0, ",");
          
          }
          Serial.println("-");
          rssi = LoRa.packetRssi();
          //snr = getRSSI_Packet();
          //rssi = 0;
          //snr = 0;
          display_gps();
        }
    rxDone = false;
    delay(500);
    }
  
}



void onesec_events(){
  Serial.println("one second");
  return;
}

void fivesec_events(){
  Serial.println("five second");
  return;
}

void tensec_events(){
  Serial.println("ten second");
  return;
}


/* *******************
 *  DISPLAY ROUTINES
 *  
 */

void display_init(){
   return;
}


void display_gps(){
      
      double val;
      char dstr[20];
      Serial.println("Display results on OLED");
      u8x8.clear();
      u8x8.print("HAB SNR");
      dtostrf(snr,2,2,dstr);
      u8x8.println(dstr);

      val = remote_data.longitude;
      dtostrf(val,4,6,dstr);
      Serial.println(dstr);
      u8x8.print("Lng: ");
      u8x8.println(dstr);
      
      
      val = remote_data.latitude;
      dtostrf(val,4,6,dstr);
      u8x8.print("Lat: ");
      u8x8.println(dstr);

    
      val = remote_data.alt;
      dtostrf(val,4,2,dstr);
      u8x8.print(" Alt:");
      u8x8.println(dstr);

      val = remote_data.hours;
      dtostrf(val,2,0,dstr);
      u8x8.print("Time: ");
      u8x8.print(dstr);
      u8x8.print(":");
      
      val = remote_data.minutes;
      dtostrf(val,2,0,dstr);
      u8x8.print(dstr);
      u8x8.print(":");
      
      val = remote_data.seconds;
      dtostrf(val,2,0,dstr);
      u8x8.print(dstr);
      
      //itoa(remote_data.flightCount,dstr,10);
      //u8x8.println(dstr);
   
   return;
}

void display_temp(){
   return;
}


