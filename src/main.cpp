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
#include "SSD1306Wire.h"  
//#include <SoftwareSerial.h>      // legacy: #include "SSD1306.h"
#include <TinyGPSPlus.h>
#include <WiFi.h>
#include <habhub.h>
#include "calc_crc.h"
#include "mjswifi.h"
#include <AsyncTCP.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <main.h>
#include <gps_local.h>
#include <remote.h>
#include <display.h>
#include <LoRa.h>
#include "button.h"
#include <axp20x.h>
#include <sondehub.h>
#include <utctime.h>



// #define BAND 434.450E6
// #define LORA_DEFAULT_MODE 0


// power management library required for some versions of T Beam
AXP20X_Class axp;
//#include <SparkFun_u-blox_GNSS_Arduino_Library.h> //http://librarymanager/All#SparkFun_u-blox_GNSS
//SFE_UBLOX_GNSS myGNSS;

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
//#define LORA_FRQ 434.450E6
//#define LORA_FRQ 434.712E6
#define IMPLICIT_PAYLOAD_LENGTH 128
#define GPS_RX_PIN 34
#define GPS_TX_PIN 12
#define GPS_BAND_RATE      9600
#define BUTTON_PIN 38
#define I2C_SDA         21
#define I2C_SCL         22

void update_display();

bool sondehubEnabled = true; // stops updating live habhub wqhen running receiver tests
unsigned long loopTimer;
int listenerUploadTimer = 0;
TinyGPSPlus gps;
//SSD1306 display(0x3c, 21, 22);

SSD1306Wire display(OLED_I2C_ADDRESS, SDA, SCL);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void copy_config(){

  strcpy(thisReceiver.antenna,thisConfig.antenna);
  strcpy(thisReceiver.callSign, thisConfig.callSign);
  strcpy(thisReceiver.software, thisConfig.software);
  strcpy(thisReceiver.version, thisConfig.version);

  thisReceiver.set_frequency = thisConfig.set_frequency;
  thisReceiver.set_bandwidth = thisConfig.set_bandwidth;
  thisReceiver.lora_mode = thisConfig.lora_mode;
  thisReceiver.gps_lon = thisConfig.gps_lon;
  thisReceiver.gps_lat = thisConfig.gps_lat;
  thisReceiver.gps_alt = thisConfig.gps_alt;
  thisReceiver.chaseCar = thisConfig.chaseCar;
  thisReceiver.listenerUploadTimer = thisConfig.listenerUploadTimer;
  thisReceiver.autoTune = thisConfig.autoTune;
}
void display_logo(){
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_16);
  display.drawString(1, 10, "We are the..");
  display.drawString(2, 15, "Weirdie Beardies");
  // ss.begin(GPSBaud);
  display.display();
}

void testSondeHubForever(){
  int flightCount = 1;
  char resultBuffer[30];  
  char sentenceBuffer[150];

  while(1){
    testSentence(sentenceBuffer,flightCount++);
    getTelemetryData(sentenceBuffer);
    Serial.print("call sign:");
    Serial.println(remote_data.callSign);
    Serial.print("Flight Count:");
    Serial.println(remote_data.flightCount);
    Serial.print("Time:");
    Serial.println(remote_data.time);
    Serial.print("latitude:");
    Serial.println(remote_data.latitude);
    Serial.print("longitude:");
    Serial.println(remote_data.longitude);
    Serial.print("altitude:");
    Serial.println(remote_data.alt);

    Serial.print("Tests UTC from Time");
    remoteTimeUTCFromTime(resultBuffer,&remote_data.time[0]);

    Serial.println(resultBuffer);
    
    Serial.print("Local call sign:");
    Serial.println(localGPSData.callSign);
    Serial.print("Local Time:");
    Serial.println(localGPSData.time);
    Serial.print("Local latitude:");
    Serial.println(localGPSData.Latitude);
    Serial.print("Local longitude:");
    Serial.println(localGPSData.Longitude);
    Serial.print("Local altitude:");
    Serial.println(localGPSData.Altitude); 
    
    uploadHabPayloadToSondehub(); 
    delay(5000); 
    uploadListenerToSondehub();
     
    delay(10000);
  
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
    Serial.print(rxBuffer[i]); 
  }
  remote_data.rssi = LoRa.packetRssi(); 
  remote_data.snr = LoRa.packetSnr();

  // print RSSI of packet
  Serial.print(" RSSI ");
  Serial.print(remote_data.rssi);
  Serial.print(" SNR ");
  Serial.println(remote_data.snr);
  
  rxDone = true;
}


void setup()
{
// info strings
  strcpy(thisConfig.antenna,"Diamond YAGI");
  strcpy(thisConfig.callSign,"MJSChase01");
  strcpy(thisConfig.version,"1.0.0");
  strcpy(thisConfig.software,"MJSLoraRX");
// setup params  
  thisConfig.set_frequency = 434.226E6;
  thisConfig.lora_mode = 1;
  thisConfig.set_bandwidth = 20.8E3;
  // receiver location defaults
  thisConfig.gps_lat = 53.226849;
  thisConfig.gps_lon = -2.506535;
  thisConfig.gps_alt = 30;
  thisConfig.chaseCar = true;
  thisConfig.listenerUploadTimer = 10; 
  thisConfig.autoTune = true;


  // **********IMPORTANT initialisation of  default config values *************
  copy_config();

  Wire.begin(I2C_SDA, I2C_SCL);
//  pinMode(ledPin, OUTPUT);
  pinMode(16,OUTPUT);
  pinMode(BLUE_LED,OUTPUT);
  
  digitalWrite(16, LOW);    // set GPIO16 low to reset OLED
  delay(50); 
  digitalWrite(16, HIGH); // while OLED is running, must set GPIO16 in high
  
  // start serial comms
  Serial.begin(115200);
  delay(1000);  
  
  // Initialize SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }


  // setup power management chip if fitted to enable GPS comms
  if (!axp.begin(Wire, AXP192_SLAVE_ADDRESS)) {
    Serial.println("AXP192 Begin PASS");
  } else {
    Serial.println("AXP192 Begin FAIL");
  }
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  
  Serial.println("All comms started");

  delay(100);

  Serial.println("init display");
  // Initialising the UI will init the display too.
  display.init();

  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  display.drawString(0,0,"Lora Tracker");
  display.drawString(0,10,"MJS Technology");

  display.drawString(0,20,String(thisReceiver.set_frequency));
  display.drawString(0,30,"connecting to internet");

  display.display() ;
/**************************** Init Internet **************************/  
  //display_init();
  // start the wifi connection
  connectToWifi(ssid,password);
  // set up the onboard real time clock and sync with the internet    
  initRTC();
  initLocalGPSData(thisReceiver.callSign);

  // just run to test sondehub with fake datai
  //int flightCOunt = 1;
  //char xsentenceBuffer[150];
  //display.clear(); 
  display.drawString(0,46,WiFi.localIP().toString().c_str());
  display.display();
  delay(2000);
  // upload listener location to HABHUB

/**************************** Init LORA **************************/  
  
  Serial.println("Init Lora");
  SPI.begin(SPICLK,MISO,MOSI,SS);    // wrks for ESP32 DOIT board 
  LoRa.setPins(SS,RST,DIO);
  if(!LoRa.begin(thisReceiver.set_frequency)){
  Serial.println("Lora not detected");

  }
  currentFrq = thisReceiver.set_frequency;
  lastFrqOK = currentFrq;

  // set lora values to a pits compatible mode
  setLoraMode(thisReceiver.lora_mode);
  LoRa.setTxPower(10,PA_OUTPUT_PA_BOOST_PIN);
  
  //LoRa.dumpRegisters(Serial);

  LoRa.onReceive(onReceive);
  // put the radio into receive mode
  // set the packet size if using SF6 (implicit mode)
  LoRa.receive(payload);
  
  
/**************************** setup Listener on Sondehub map **************************/  
  uploadListenerToSondehub();
  listenerUploadTimer = 0;


/**************************** Init WEBSERVER **************************/  

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
    thisReceiver.autoTune = true;
  });
  
  // Route to set GPIO to LOW
  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/index.html", String(), false, remoteProcessor);
    thisReceiver.autoTune = false;
  });

// Route to set GPIO to LOW
  server.on("/gps", HTTP_GET, [](AsyncWebServerRequest *request){
     request->send(SPIFFS, "/index.html", String(), false, localProcessor);
  });

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    int paramsNr = request->params(); // number of params (e.g., 1)
    Serial.println(paramsNr);
    Serial.println();
    
    AsyncWebParameter * j = request->getParam(0); // 1st parameter
    Serial.print("Frequency: ");
    Serial.print(j->value());                     // value ^
    Serial.println();
    
    double temp = j->value().toDouble();
    thisReceiver.set_frequency = long(temp);

    request->send(SPIFFS, "/index.html", String(), false, localProcessor);
    //request->send(200);
  });
  server.onNotFound(notFound);

  // Start server
  server.begin();
  habhubCounter = millis();
  Serial1.begin(GPS_BAND_RATE, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);


  // button setup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  // reset the oneSec loop timer 
  loopTimer = millis();
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
    payload=IMPLICIT_PAYLOAD_LENGTH;
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
      remote_data.lastPacketAt = millis();
    }
  }
}

/* ******************************* MAIN LOOP MAIN LOOP *************************************/

void loop()
{
    delay(10);

    if((millis() - loopTimer) >= 1000){
      loopTimer = millis();
      onesec_events();
    }
    // PROCESS GPS
    if(gpsloopcnt++ >= 100 ){
      // read local gps
      //Serial.println("try gps");
      int gpsflag = 0;
      while (Serial1.available() > 0){
          gpsflag = 1;
          //Serial.println("Reading GPS");
          gps.encode(Serial1.read());
            
      }
      //displayInfo(gps);
      updateLocalGPS(gps,thisReceiver.callSign);
      gpsloopcnt = 0;
      
      if(thisReceiver.chaseCar == true && ++listenerUploadTimer >= thisReceiver.listenerUploadTimer){
       // uploadListenerToSondehub();
        listenerUploadTimer = 0;
      }
    }
    // ********************** Set frequency manually if required ***********************
    if(thisReceiver.autoTune == false && (thisReceiver.set_frequency != currentFrq)){
      currentFrq = thisReceiver.set_frequency;
      lastFrqOK = currentFrq;
      LoRa.setFrequency(currentFrq);
      Serial.print("New frequency set to : ");
      Serial.println(currentFrq);
    } 

    // Check and Process lora  message
    if(rxDone==true){

      // retune the frequency every so many packets if required
      if(msgCount++ > 2 && thisReceiver.autoTune == true){
        msgCount = 0;
        tuneFrq();
      }
      // everything running so reset all the error conditions
      // copy the received telem data to the remote tracker struct

      if(rxBuffer[0] == '$'){ 
//        for(int i = 0; i <= 40; i++){
//          Serial.print(rxBuffer[i]);
//        }
        getTelemetryData(rxBuffer);
      }

      remote_data.active = true;
      remote_data.lastPacketAt = millis();
      resetFrq = false;
      Serial.println(rxBuffer);
      
      // send to habhub every so often
        if(sondehubEnabled){
          Serial.print("Sending to SondeHub -- ");
          int response = uploadHabPayloadToSondehub();
          
          Serial.print(response);
      }
      rxDone = false;
    }

    // reset valid data if timed out on message so we know we havn't received for a while
    else{
      if(((millis() - remote_data.lastPacketAt) >= 10000) && !resetFrq && thisReceiver.autoTune == true){
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
      next_page();

  }
  if (checkButton() == LONG_PRESS){
      Serial.print("long press");

  }

}

void tuneFrq(){    
  lastFrqOK = currentFrq;
  long freqErr = LoRa.packetFrequencyError();
  Serial.print("Frequency Error = ");
  Serial.println(freqErr);
  
  if(abs(freqErr) > 200){
    Serial.print("FRQ was: ");
    Serial.print(currentFrq);
    
    Serial.print(" FRQ Error: ");
    Serial.print(freqErr);
    int adj = abs(freqErr);
    //(adj < 1000) ? adj=adj : adj = 1000; 
    if(freqErr > 0){
      //currentFrq -= 1000; // abs(freqErr);
      currentFrq -= adj; // abs(freqErr);
    }
    else{
      //currentFrq += 1000; //abs(freqErr);
      currentFrq += adj; //abs(freqErr);
    }

    LoRa.setFrequency(currentFrq);
    Serial.println(" Correcting FRQ to: ");
    Serial.println(currentFrq);
  }
}


void onesec_events(){
  //Serial.println("one second");
  display_page();
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

