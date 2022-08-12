#include <Arduino.h>
#include <string>
#include <ctype.h>
#include <time.h>
#include "main.h"
#include <WiFi.h>
#include "mjswifi.h"
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include <gps_local.h>
#include <remote.h>



extern gpsT localGPSData;
//extern remoteT remote_data;

String remoteProcessor(const String& var){
  
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
     // return String(remote_data.courseTo,0);
    String tempStr = remote_data.cardinalCourseTo; 
    return String(remote_data.courseTo) + " -> " + tempStr;
    }
  if(var == "DIS"){
     // return String(remote_data.distancem,2);
    return String(remote_data.distancem);
    }
  if(var == "ID"){
    return String(remote_data.callSign);
    }
  return String("N/A");
}

String localProcessor(const String& var){
  
  if(var == "LAT"){
     
      return String( localGPSData.Latitude,6);
    }
  if(var == "LON"){
      return String(localGPSData.Longitude,6);
    }
  if(var == "ALT"){
      return String(localGPSData.Altitude,0);
    }
  if(var == "CRS"){
     // return String(remote_data.courseTo,0);
    return String(remote_data.courseTo);
    }
  if(var == "DIS"){
     // return String(remote_data.distancem,2);
    return String(remote_data.distancem);
    }
  if(var == "ID"){
    return String(localGPSData.callSign);
    }
  return String("N/A");
}

