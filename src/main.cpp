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
#include "SSD1306Wire.h"        // legacy: #include "SSD1306.h"
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <habhub.h>
#include "calc_crc.h"
#include "mjswifi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "main.h"


#include <LoRa.h>
//#define BAND    4345E5  //you can set band here directly,e.g. 868E6,915E6
#define BAND 434.45E6
// define this if running on heltec board comment out if not

void onReceive(int packetSize);
#define TTGO_LORA
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

#ifdef TTGO_LORA
#define SPICLK     5    // GPIO5  -- SX1278's SCK
#define MISO    19   // GPIO19 -- SX1278's MISnO
#define MOSI    27   // GPIO27 -- SX1278's MOSI
#define SS      18   // GPIO18 -- SX1278's CS
#define RST     14   // GPIO14 -- SX1278's RESET
#define DIO     26   // GPIO26 -- SX1278's IRQ(Interrupt Request)
#define BLUE_LED 14
#endif

#define _EP32_

// Define proper RST_PIN if required.
#define RST_PIN -1

#define LED_WARN            5
#define LED_OK              6
#define A0_MULTIPLIER      4.9

// define for display ascii library
#define I2C_ADDRESS 0x3C
#define PITS_ENABLED
//SSD1306 display(0x3c, 21, 22);
SSD1306Wire display(0x3c, SDA, SCL);   // ADDRESS, SDA, SCL  -  SDA and SCL usually populate automatically based on your board's pins_arduino.h e.g. https://github.com/esp8266/Arduino/blob/master/variants/nodemcu/pins_arduino.h

void setup()
{
//  pinMode(ledPin, OUTPUT);
  pinMode(16,OUTPUT);
  pinMode(BLUE_LED,OUTPUT);
  
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  Serial.begin(115200);
  delay(1000);  
 
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  Serial.println("init display");
// Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  //display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Hello world");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 26, "Hello world");
  // ss.begin(GPSBaud);
  display.display();
  delay(2000);
  //display.clear();
  //display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0,0,"MJS Lora Tracker");
  display.drawString(0,10,"MJS Technology");
  display.drawString(0,26,"lora 434.45Mhz: ");
  display.display();
  Serial.println("Init Lora");

  delay(2000);
  
  //display_init();
  // start the wifi connection
  connectToWifi(espClient,ssid,password);

  // upload listener location to HABHUB
  uploadListenerPacket(gatewayID, gps_time, gps_lat, gps_lon, antenna.c_str(),radio.c_str());
    
    

  SPI.begin(SPICLK,MISO,MOSI,SS);    // wrks for ESP32 DOIT board 
  LoRa.setPins(SS,RST,DIO);
  if(!LoRa.begin(BAND)){
  Serial.println("Lora not detected");

  }
  currentFrq = BAND;
  lastFrqOK = BAND;

  // set lora values to a pits compatible mode
  setLoraMode(0);
  LoRa.setTxPower(15,PA_OUTPUT_PA_BOOST_PIN);
  
  //LoRa.dumpRegisters(Serial);

  LoRa.onReceive(onReceive);
  // put the radio into receive mode
  // set the packet size if using SF6 (implicit mode)
  LoRa.receive(payload);

  time_t rawtime;
  struct tm * timeinfo;
  int  month ,day;
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

// Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, processor);
  });

// Route to set GPIO to LOW
  server.on("/gps", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(SPIFFS, "/index.html", String(), false, processor);
  });

  // Start server
  server.begin();
  habhubCounter = millis();

}

void setLoraMode(int mode){
  payload=0;
  switch (mode){
    case 0:
    LoRa.setSpreadingFactor(11);
    LoRa.setSignalBandwidth(20800);
    LoRa.setCodingRate4(8);
    Serial.print("Mode 0: sf 7, BW 20k8, CR 8 ");
    break;

    case 1:
    LoRa.setSpreadingFactor(6);
    LoRa.setSignalBandwidth(20800);
    LoRa.setCodingRate4(5);
    payload=255;
    Serial.print("Mode 1: sf 6, BW 20k8, CR 5 ");
    break;

    case 2:
    LoRa.setSpreadingFactor(8);
    LoRa.setSignalBandwidth(62.5E3);
    LoRa.setCodingRate4(8);
    Serial.print("Mode 1: sf 8, BW 62K5, CR 8 ");
    break;

    case 3:
    LoRa.setSpreadingFactor(7);
    LoRa.setSignalBandwidth(20.8E3);
    LoRa.setCodingRate4(8);
    Serial.print("Mode 3: sf 7, BW 20k8, CR 8 ");
    break;
  }
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
    Serial.println("****Invalid packet****");
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
    
    delay(10);
    if(loopcnt++ == 10 ){
//      Serial.println("read Lora");
      loopcnt = 0;
    }
    
   // readPacketsLoop();


      // Process message
    if(rxDone==true){
      

      if(msgCount++ > 5){
        msgCount = 0;
        tuneFrq();
      }
      // everything running so rese tall the error conditions
      remote_data.active = true;
      remote_data.lastPacketAt = millis();
      resetFrq = false;

      #ifndef PITS_ENABLED   
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
      #endif

      // if no gps data at remote station just display a waiting message
      //Data Received
      char * rxptr;

      rxptr = rxBuffer;

      // extract the values
      #ifndef PITS_ENABLED
      // if radiohead format skip header
      rxptr = &rxBuffer[4];`
      #endif

      // prep for habhub
      if(BuildSentence(txBuffer,rxptr,sizeof(txBuffer))>0){
        Serial.print("message received: " );
        Serial.println(rxptr);
        // its telemetry so process it 
        flightCount++;
        // pull the telem data into global remote data structure
        // send to habhub
        if((millis() - habhubCounter) > habhubDelayMS){
          Serial.print("String to send to HabHub -- ");
          Serial.println(txBuffer);
          Serial.println("end");
          habhubCounter = millis();
          // just copy received data
          
          uploadTelemetryPacket( rxptr , flightCount , (char *) gatewayID);

        }
        getTelemetryData(rxptr);
        rssi = LoRa.packetRssi();


      }
      display_gps();
      rxDone = false;
    }
    // reset valid data if timed out on message so we know we havn't received for a while
    else{
      if(((millis() - remote_data.lastPacketAt) >= 10000) && !resetFrq){
        resetFrq = true;
        remote_data.active=false;
        remote_data.rssi = 0;
        LoRa.setFrequency(lastFrqOK);
        Serial.println("No Signal so I'm resetting Frequency to last known good");
        Serial.println(lastFrqOK);
      }
    }
}

void tuneFrq(){    
  lastFrqOK = currentFrq;
  long freqErr = LoRa.packetFrequencyError();
  Serial.print("Frequency Error = ");
  Serial.println(freqErr);
  
  if(abs(freqErr) > 500){
    Serial.print("FRQ was: ");
    Serial.print(currentFrq);
    Serial.print(" FRQ Error: ");
    Serial.print(freqErr);
    int adj = abs(freqErr);
    (adj < 1000) ? adj=adj : adj = 1000; 
    if(freqErr > 0){
      currentFrq -= 1000; // abs(freqErr);
    }
    else{
      currentFrq += 1000; //abs(freqErr);
    }

    LoRa.setFrequency(currentFrq);
    Serial.println(" Correcting FRQ to: ");
    Serial.println(currentFrq);
  }
}

void getTelemetryData(char *rxptr){

  int valcount = 0;
  double val = 0.0;
  int ival = 0;
  // now loop round the substrings  
  //rxptr+=2;
  char* strval = strtok(rxptr, "$,*");
  while(strval != 0)
  {
//    Serial.print(strval);
//    Serial.print("-");

    switch(valcount){
      case 0:
      if(strlen(strval)<=10){
        strcpy(remote_data.callSign,strval);
      }
      //Serial.print(strval);
      //Serial.print("~");    
      break;

      case 1:
      // flightCount
      ival = atoi(strval);
      remote_data.flightCount = ival;
      //Serial.print(ival,DEC);
      //Serial.print("~");    
      break;

      case 2:
      // time
      if(strlen(strval)<=10){
      strcpy(remote_data.time,strval);
      //Serial.print(strval);
      }
      else{
      strcpy(remote_data.time , "--:--:--");  
      //Serial.print(remote_data.time);
      //Serial.print("~");    
      }
      break;

      case 3:
      // latitude
      val = atof(strval);
      remote_data.latitude = val;
      //Serial.print(val,6);
      //Serial.print("~");    
      // Find the next command in input string
      break;

      case 4:
      // longitude
      val = atof(strval);
      remote_data.longitude = val;
      //Serial.print(val,6);
      //Serial.print("~");    
      break;
      
      case 5:
      // altitude
      val = atof(strval);
      remote_data.alt = val;
      //Serial.print(val,2);
      //Serial.print("~");    
      break;

      case 6:
      // Speed 
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;
      
      case 7:
      // Heading
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;

      case 8:
      // Sats
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;

      case 9:
      // Internal Temp data
      val = atof(strval);
      remote_data.InternalTemperature = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 10:
      // External Temp data
      val = atof(strval);
      remote_data.temperature_c = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 11:
      // Pressure data
      val = atof(strval);
      remote_data.Pressure = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 12:
      // Humidity Temp data
      val = atof(strval);
      remote_data.humidity = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 13:
      // Batt Voltage
      val = atof(strval);
      remote_data.BatteryVoltage = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
    
    }
    valcount ++;
    // next substring
    strval = strtok(0, ",*");
  
  }
  //Serial.println("-");
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
      //Serial.println("Display results on OLED");
      display.clear();
      display.drawString(0,0,"HAB SNR");
      dtostrf(snr,2,2,dstr);
      display.drawString(60,0,dstr);

      display.drawString(0,10,"RSSI ");
      dtostrf(rssi,2,2,dstr);
      display.drawString(60,10,dstr);

      val = remote_data.longitude;
      dtostrf(val,4,6,dstr);
      display.drawString(0,20,"Lng: ");
      display.drawString(20,20,dstr);
      
      
      val = remote_data.latitude;
      dtostrf(val,4,6,dstr);
      display.drawString(0,30,"Lat: ");
      display.drawString(20,30,dstr);

    
      val = remote_data.alt;
      dtostrf(val,4,2,dstr);
      display.drawString(0,40," Alt:");
      display.drawString(20,40,dstr);

      
      display.drawString(0,50,"Time: ");
      display.drawString(20,50,remote_data.time);
      display.display();            
      //itoa(remote_data.flightCount,dstr,10);
      //display.drawStringln(dstr);
   
   return;
}

void display_temp(){
   return;
}


