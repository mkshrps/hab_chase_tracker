#define _WIFI_
#include <WiFi.h>


//#include <HTTPClient.h>
#include "mjswifi.h"
// We start by connecting to a WiFi network


void connectToWifi( const char* ssid, const char* password){
    
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.print(ssid);
    Serial.print(" , ");
    Serial.println(password);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    u8_t progress = 10;
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
//        display.drawProgressBar(20, 45, 80, 10,progress);
//        display.display();
//        if(progress <= 100){
//            progress += 10;
//        }
//        else{
            //Serial.print("reset progress");
//            progress = 10;
//            display.setColor(BLACK);
//            display.display();
//            display.fillRect(20,45,100,15);
//            display.setColor(WHITE);
//            display.display();
//        }

    } 


    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

}

