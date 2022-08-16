#define _DISPLAY_
//#include <Arduino.h>
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <SPI.h>
#include <Wire.h>
#include <remote.h>
#include <SSD1306Wire.h>
#include <lora.h>
#include <main.h>

//#include <display.h>
extern SSD1306Wire display;


void display_gps(){
      
      double val;
      char dstr[20];
      //Serial.println("Display results on OLED");
      display.clear();
      display.drawString(0,0,"HAB SNR");
      dtostrf(remote_data.snr,2,2,dstr);
      display.drawString(60,0,dstr);

      if(remote_data.active){
        snprintf( dstr, 4, "%d", remote_data.flightCount % 100 );
        display.drawString(100,0,dstr);
      }
      else{
        display.drawString(100,0,"NS");
      }

      display.drawString(0,10,"RSSI ");
      dtostrf(remote_data.rssi,2,2,dstr);
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
      display.drawString(90,30," Alt:");
      display.drawString(130,30,dstr);

      display.drawString(0,40,"CrsTo:");

      val = remote_data.courseTo;
      dtostrf(val,4,2,dstr);
      display.drawString(80,40,dstr);
      
      display.drawString(0,50,"Dist: ");
      val = remote_data.distancem;
      dtostrf(val,4,2,dstr);
      display.drawString(80,50,dstr);
       
      display.display();            
      //itoa(remote_data.flightCount,dstr,10);
      //display.drawStringln(dstr);
   
   return;
}

void  display_frequency_page(void){
      display.clear();
      display.print("Curr Frq ");
      display.print(currentFrq);

      display.print("RSSI ");
      display.print(remote_data.rssi);
      display.print("Frq Error ");
      long freqErr = LoRa.packetFrequencyError();
      display.print(freqErr);
      display.print("Lora Mode ");
      
    //  display.print(loraMode);
      

}
