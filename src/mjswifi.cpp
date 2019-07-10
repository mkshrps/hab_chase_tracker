#define _WIFI_
#include <WiFi.h>
#include <HTTPClient.h>
#include "mjswifi.h"
// We start by connecting to a WiFi network


void connectToWifi(WiFiClient , const char* ssid, const char* password){
    
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

}

