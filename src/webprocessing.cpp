#include <Arduino.h>
#include <string>
#include <ctype.h>
#include <time.h>
#include "main.h"
#include <WiFi.h>
#include "mjswifi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

// Replaces placeholder with LED state value
/*
String processor(const String& var){
  Serial.println(var);
  if(var == "STATE"){
    if(digitalRead(ledPin)){
      ledState = "ON";
    }
    else{
      ledState = "OFF";
    }
    Serial.print(ledState);
    return ledState;
  }
  return String();
}
*/

String processor(const String& var){
  Serial.println(var);
  if(var == "LAT"){
      
      return String(remote_data.latitude,6);
    }
  if(var == "LON"){
      return String(remote_data.longitude,6);
    }
  if(var == "ALT"){
      return String(remote_data.alt,0);
    }
  if(var == "CRS"){
      return String(remote_data.courseTo,0);
    }
  if(var == "DIS"){
      return String(remote_data.distancem,2);
    }
  if(var == "ID"){
      return remote_data.callSign;
    }
  return "N/A";
}


