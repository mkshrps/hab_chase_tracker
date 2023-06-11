#include <time.h>
#include <ESP32Time.h>
#include <NTPClient.h>
#include <WiFi.h> // for WiFi shield
#include <WiFiUdp.h>
//ESP32Time rtc;
ESP32Time rtc(3600);  // offset in seconds GMT+1


void initRTC() {

    WiFiUDP ntpUDP;
    NTPClient timeClient(ntpUDP);
    timeClient.begin();
    timeClient.update();
    timeClient.getEpochTime();
    rtc.setTime(timeClient.getEpochTime());
    
    Serial.println(rtc.getTime("%Y %m %dT%H:%M:%S.0000Z")); 
    
}

const char *getTimeNow(){
   return rtc.getTime("%Y-%m-%dT%H:%M:%S.000000Z").c_str(); 
}

void currentTimeUTC(char *dateBuffer){
   // YYYY-MM-DDTHH:MM:SS.SSSSSSZ
    strcpy(dateBuffer,rtc.getTime("%Y-%m-%dT%H:%M:%S.000000Z").c_str()); 

    }

void remoteTimeUTCFromHMS(char *datebuff,int hours, int minutes, int seconds){
   
    //char * tptr;
   sprintf( datebuff,"%sT%02d-%02d-%02d.000000Z",rtc.getTime("%Y-%m-%d").c_str(),hours,minutes,seconds);
    //Serial.print(datebuff);
} 

void remoteTimeUTCFromTime(char *datebuff,char *timebuff){
   
    //char * tptr;
   sprintf( datebuff,"%sT%s.000000Z",rtc.getTime("%Y-%m-%d").c_str(),timebuff);
    //Serial.print(datebuff);
} 

void getHMS(char *timeStr){
    
    strcpy(timeStr,rtc.getTime().c_str());

}

void testTime(){
    char datebuff[30];

    Serial.print("current time --");
    currentTimeUTC(datebuff);
    Serial.println(datebuff);
    Serial.print("remote time test");
    remoteTimeUTCFromHMS(datebuff,01,02,15);
    Serial.println(datebuff);
    delay(2000);
    Serial.print(" get time now() -- ");
    Serial.println(getTimeNow());
}
