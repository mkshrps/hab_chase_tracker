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
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "main.h"
#include <gps_local.h>
#include <remote.h>
#include <display.h>
#include <LoRa.h>
#include "button.h"

#define BAND 434.45E6
// lora receive callback
void onReceive(int packetSize);
#define TTGO_LORA

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
#define OLED_I2C_ADDRESS 0x3C
#define PITS_ENABLED
#define LORA_DEFAULT_MODE 1
#define LORA_FRQ 434.450E6

#define GPS_RX_PIN 34
#define GPS_TX_PIN 12
#define GPS_BAND_RATE      9600
#define BUTTON_PIN 38


// #define BUTTON_PIN_MASK GPIO_SEL_39
String antenna("whip");
String radio("LoRa RFM95W");
const char *gatewayID = "MJS01";

//SSD1306 display(0x3c, 21, 22);

SSD1306Wire display(OLED_I2C_ADDRESS, SDA, SCL);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

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
  

  //display_init();
  // start the wifi connection
  connectToWifi(espClient,ssid,password);
  
  display.drawString(0,46,WiFi.localIP().toString().c_str());
  display.display();
  delay(5000);
  // upload listener location to HABHUB
  uploadListenerPacket(espClient,gatewayID, gps_time, gps_lat, gps_lon, antenna.c_str(),radio.c_str());
    
  Serial.println("Init Lora");
  SPI.begin(SPICLK,MISO,MOSI,SS);    // wrks for ESP32 DOIT board 
  LoRa.setPins(SS,RST,DIO);
  if(!LoRa.begin(BAND)){
  Serial.println("Lora not detected");

  }
  currentFrq = BAND;
  lastFrqOK = BAND;

  // set lora values to a pits compatible mode
  setLoraMode(1);
  LoRa.setTxPower(10,PA_OUTPUT_PA_BOOST_PIN);
  
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
   request->send(SPIFFS, "/index.html", "text/html", false, remoteProcessor);
    //request->send(SPIFFS, "/index.html","text/html");
  });
  
  // Route to load style.css file
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/style.css", "text/css");
  });

  // Route to set GPIO to HIGH
  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, remoteProcessor);
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, remoteProcessor);
  });

// Route to set GPIO to LOW
  server.on("/gps", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(SPIFFS, "/index.html", String(), false, localProcessor);
  });

  server.onNotFound(notFound);

  // Start server
  server.begin();
  habhubCounter = millis();
  Serial1.begin(GPS_BAND_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  // Acebutton setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  

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
  Serial.print(remote_data.rssi);
  Serial.print(" SNR ");
  Serial.println(remote_data.snr);
  remote_data.rssi = LoRa.packetRssi(); 
  remote_data.snr = LoRa.packetSnr();

  rxDone = true;
}



void loop()
{
    
    
    delay(10);

    

    if(gpsloopcnt++ == 100 ){
      // read local gps

      while (Serial1.available() > 0){
          //Serial.println("Reading GPS");
          gps.encode(Serial1.read());
            
      }
      //displayInfo(gps);
      updateLocalGPS(gps,gatewayID);
      gpsloopcnt = 0;
    }


    // Check and Process lora  message
    if(rxDone==true){

      // retune the frequency every so many packets if required
      if(msgCount++ > 5){
        msgCount = 0;
        tuneFrq();
      }
      // everything running so reset all the error conditions
      remote_data.active = true;
      remote_data.lastPacketAt = millis();
      resetFrq = false;
      Serial.println(rxBuffer);
      // send to habhub every so often
      if((millis() - habhubCounter) > habhubDelayMS){
        Serial.print("Sending to HabHub -- ");
        habhubCounter = millis();
        // just copy received data
        flightCount++;
        // sanity check

        if(rxBuffer[0] == '$'){ 
          uploadTelemetryPacket( espClient, rxBuffer , flightCount , (char *) gatewayID);
          Serial.println("Done");
        }
        else{
          Serial.println("invalid packet");
        }
      }
      // copy the received telem data to the remote tracker struct
      if(rxBuffer[0] == '$'){ 
        getTelemetryData(rxBuffer);
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
/*
             if(++currentPage > NUM_PAGES) currentPage = START_PAGE;
          displayPage(currentPage);
 */


  if (checkButton() == SHORT_PRESS){
      Serial.print("short press");

  }
  if (checkButton() == LONG_PRESS){
      Serial.print("short press");

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



void display_temp(){
   return;
}

