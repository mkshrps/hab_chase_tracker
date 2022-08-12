#define _GPS_
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <main.h>
//#include <TinyGPSPlus.h>
#include <gps_local.h>
#include <remote.h>


//extern remoteT remote_data;

void displayInfo(TinyGPSPlus gps)
{ 
    
    Serial.print(F("Location: "));
    if (gps.location.isValid()) {
        Serial.print(gps.location.lat(), 6);
        Serial.print(F(","));
        Serial.print(gps.location.lng(), 6);
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.print(F("  Date/Time: "));
    if (gps.date.isValid()) {
        Serial.print(gps.date.month());
        Serial.print(F("/"));
        Serial.print(gps.date.day());
        Serial.print(F("/"));
        Serial.print(gps.date.year());
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.print(F(" "));
    if (gps.time.isValid()) {
        if (gps.time.hour() < 10) Serial.print(F("0"));
        Serial.print(gps.time.hour());
        Serial.print(F(":"));
        if (gps.time.minute() < 10) Serial.print(F("0"));
        Serial.print(gps.time.minute());
        Serial.print(F(":"));
        if (gps.time.second() < 10) Serial.print(F("0"));
        Serial.print(gps.time.second());
        Serial.print(F("."));
        if (gps.time.centisecond() < 10) Serial.print(F("0"));
        Serial.print(gps.time.centisecond());
    } else {
        Serial.print(F("INVALID"));
    }

    Serial.println();
}

void updateLocalGPS(TinyGPSPlus gps, const char * gatewayID) {

      strcpy(localGPSData.callSign,gatewayID);
      if(gps.location.isUpdated()){
        if (gps.location.isValid()){
          localGPSData.Longitude = (float) gps.location.lng();
          localGPSData.Latitude = (float) gps.location.lat();
        }

        if (gps.altitude.isValid()){
          localGPSData.Altitude = (long) gps.altitude.meters();
        }

        // if gps data is ready
        if (gps.location.isValid() && gps.altitude.isValid()){
          localGPSData.isValid = true;
          //Serial.println("values valid");
        }
        localGPSData.Satellites = (unsigned int) gps.satellites.value();
        localGPSData.failedCS = (unsigned int) gps.failedChecksum();
        if(gps.time.isValid() && gps.time.isUpdated() ){
        localGPSData.Hours = (uint8_t) gps.time.hour();
        localGPSData.Minutes = (uint8_t) gps.time.minute();
        localGPSData.Seconds = (uint8_t) gps.time.second();
        }

        if(localGPSData.isValid && remote_data.isValid){
          // get course and distance if we have a remote tracker
          remote_data.courseTo =gps.courseTo(localGPSData.Latitude,localGPSData.Longitude, remote_data.latitude,remote_data.longitude);
          remote_data.distancem = gps.distanceBetween(localGPSData.Latitude,localGPSData.Longitude,remote_data.latitude,remote_data.longitude);
          strcpy(remote_data.cardinalCourseTo , gps.cardinal(remote_data.courseTo));
        }
      }
}