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

// LORA FREQUENCY SETTINGS *********************************
#define LORA_FRQ 434.448E6
#define IMPLICIT_PAYLOAD_LENGTH 255

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


void onReceive(int packetSize)
{
  if(packetSize <4){
    Serial.println("****Invalid packet****");
    return;
  }
  
  // received a packet
  Serial.println("");
  Serial.print("Received packet ");
  Serial.println("Packet Size ");
  Serial.println(packetSize);

  memset(rxBuffer,0,sizeof(rxBuffer));

  // read packet
  for (int i = 0; i < packetSize; i++)
  {
    rxBuffer[i] = (char)LoRa.read();
  }
  remote_data.rssi = LoRa.packetRssi(); 
  remote_data.snr = LoRa.packetSnr();

  // print RSSI of packet
  
  rxDone = true;
  

}


void setup()
{
// info strings
// 30 chars
  strcpy(thisConfig.antenna,"car-roof mag_mount");
  strcpy(thisConfig.callSign,"MJSChase01");
  strcpy(thisConfig.version,"1.0.0");
  strcpy(thisConfig.software,"MJSLoraRX");
// setup params  
  thisConfig.set_frequency = LORA_FRQ;
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
  thisReceiver.set_new_frq = false;
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
/*
  axp.setVWarningLevel1(3600);
  axp.setVWarningLevel2(3800);
  axp.setPowerDownVoltage(3300);

  axp.setTimeOutShutdown(false);
  axp.setTSmode(AXP_TS_PIN_MODE_DISABLE);
  axp.setShutdownTime(AXP_POWER_OFF_TIME_4S);
  axp.setStartupTime(AXP192_STARTUP_TIME_1S);

  // Turn on ADCs.
  axp.adc1Enable(AXP202_BATT_VOL_ADC1, true);
  axp.adc1Enable(AXP202_BATT_CUR_ADC1, true);
  axp.adc1Enable(AXP202_VBUS_VOL_ADC1, true);
  axp.adc1Enable(AXP202_VBUS_CUR_ADC1, true);

  // Handle power management events.
    pinMode(GPIO_NUM_35, INPUT_PULLUP);
    axp.enableIRQ(AXP202_VBUS_REMOVED_IRQ | AXP202_VBUS_CONNECT_IRQ | AXP202_VBUS_OVER_VOL_IRQ | AXP202_BATT_REMOVED_IRQ |
                      AXP202_BATT_CONNECT_IRQ | AXP202_CHARGING_FINISHED_IRQ | AXP202_PEK_SHORTPRESS_IRQ,
                  1);
    axp.clearIRQ();

 // Start charging the battery if it is installed.
  axp.setChargeControlCur(AXP1XX_CHARGE_CUR_450MA);
  axp.setChargingTargetVoltage(AXP202_TARGET_VOL_4_2V);
  axp.enableChargeing(true);
  



  axp.setChgLEDMode(AXP20X_LED_BLINK_1HZ);
*/
  axp.setPowerOutPut(AXP192_LDO2, AXP202_ON);
  axp.setPowerOutPut(AXP192_LDO3, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC2, AXP202_ON);
  axp.setPowerOutPut(AXP192_EXTEN, AXP202_ON);
  axp.setPowerOutPut(AXP192_DCDC1, AXP202_ON);
  axp.setlongPressTime(AXP_POWER_OFF_TIME_4S);

  if(axp.isBatteryConnect()) {
    Serial.println("Battery COnnected");
    Serial.print("Voltge ");
    Serial.print(axp.getBattVoltage());
    Serial.println("");
    Serial.print("Charge Current ");
    Serial.print(axp.getBattChargeCurrent());
    Serial.println("");
    Serial.print("Charge Current Setting ");
    Serial.println(axp.getSettingChargeCurrent());
    Serial.print(" is Charging Enabled? ");
    Serial.println(axp.isChargeingEnable());
    Serial.print(" is Charging ? ");
    Serial.println(axp.isChargeing());




  }
  else {
    Serial.println("!!!!!! No Battery Detected !!!!!!!!");

  }
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
  // expect the remote to be the same frequency
  remote_data.frequency = currentFrq;

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
    thisReceiver.set_new_frq = true;
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
    LoRa.enableCrc();
    payload=IMPLICIT_PAYLOAD_LENGTH;
    Serial.print("Mode 1: sf 6, BW 20k8, CR 5 ");
    Serial.print("Payload Len ");
    Serial.print(payload);
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
    if (thisReceiver.set_new_frq){
      currentFrq = thisReceiver.set_frequency;
      lastFrqOK = currentFrq;
      LoRa.setFrequency(currentFrq);
      thisReceiver.set_new_frq = false;
      Serial.print("New frequency set to : ");
      Serial.println(currentFrq);
       
    }

    if(thisReceiver.autoTune == false && (thisReceiver.set_frequency != currentFrq)){
      currentFrq = thisReceiver.set_frequency;
      lastFrqOK = currentFrq;
      LoRa.setFrequency(currentFrq);
      
      Serial.print("New frequency set to : ");
      Serial.println(currentFrq);
    } 

    // Check and Process lora  message
    if(rxDone==true){
      remote_data.isValid = false;

      Serial.print(" RSSI ");
      Serial.print(remote_data.rssi);
      Serial.print(" SNR ");
      Serial.println(remote_data.snr);
      Serial.print("Frequency ");
      Serial.println(thisReceiver.set_frequency);
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
        // get the telemetry from received packet and copy to the remote_data
        // isValid will be set if data ok
        getTelemetryData(rxBuffer);
      }

      remote_data.active = true;
      remote_data.lastPacketAt = millis();
      resetFrq = false;
      Serial.println(rxBuffer);
      
      // send to habhub every so often
      if(sondehubEnabled && remote_data.isValid){
        Serial.print("Sending to SondeHub -- ");
        int response = uploadHabPayloadToSondehub();
        //test_json();
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
    remote_data.frequency = currentFrq;
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

void print_upload_data(){

   //remote_data.frequency = 434.450E6;
    Serial.println(thisReceiver.version);
    Serial.println(thisReceiver.callSign);
    Serial.println(remote_data.callSign);
    Serial.println(remote_data.latitude);
    Serial.println(remote_data.longitude);
    Serial.println(remote_data.alt);
    Serial.println(thisReceiver.set_frequency);
    Serial.println(localGPSData.Latitude);
    Serial.println(localGPSData.Longitude);
    Serial.println(localGPSData.Altitude);
    Serial.println(thisReceiver.antenna);

}
