#define _REMOTE_
#include <arduino.h>
#include <remote.h>


// load telemetry data from received lora message into the remote_data structure  
void getTelemetryData(char *rxptr){

  int valcount = 0;
  double val = 0.0;
  int ival = 0;
  // now loop round the substrings  
  //rxptr+=2;
  Serial.print("copy teltem data");

  char* strval = strtok(rxptr, "$,*");
  while(strval != 0)
  {

//    Serial.print("-");

    switch(valcount){
      case 0:
      if(strlen(strval)<=10){
        strcpy(remote_data.callSign,strval);
      }
      //Serial.print(strval);
      //Serial.print("~");    
      break;

      case 1:
      // flightCount
      ival = atoi(strval);
      remote_data.flightCount = ival;
      //Serial.print(ival,DEC);
      //Serial.print("~");    
      break;

      case 2:
      // time
      if(strlen(strval)<=10){
      strcpy(remote_data.time,strval);
      //Serial.print(strval);
      }
      else{
      strcpy(remote_data.time , "--:--:--");  
      //Serial.print(remote_data.time);
      //Serial.print("~");    
      }
      break;

      case 3:
      // latitude
      val = atof(strval);
      remote_data.latitude = val;
      if(val != 0){
        remote_data.latitude = val;
        remote_data.isValid = true;
      }
      else{
        remote_data.isValid = false;
      }
      //Serial.print(val,6);
      //Serial.print("~");    
      // Find the next command in input string
      break;

      case 4:
      // longitude
      val = atof(strval);
      remote_data.longitude = val;
      //Serial.print(val,6);
      //Serial.print("~");    
      break;
      
      case 5:
      // altitude
      val = atof(strval);
      remote_data.alt = val;
      //Serial.print(val,2);
      //Serial.print("~");    
      break;

      case 6:
      // Speed 
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;
      
      case 7:
      // Heading
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;

      case 8:
      // Sats
      ival = atoi(strval);
      remote_data.satellites = ival;
      //Serial.print(ival);
      //Serial.print("~");    
      break;

      case 9:
      // Internal Temp data
      val = atof(strval);
      remote_data.InternalTemperature = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 10:
      // External Temp data
      val = atof(strval);
      remote_data.temperature_c = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 11:
      // Pressure data
      val = atof(strval);
      remote_data.Pressure = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 12:
      // Humidity Temp data
      val = atof(strval);
      remote_data.humidity = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
      
      case 13:
      // Batt Voltage
      val = atof(strval);
      remote_data.BatteryVoltage = val;
      //Serial.print(val);
      //Serial.print("~");    
      break;
    
    }
    valcount ++;
    // next substring
    strval = strtok(0, ",*");
  
  }
  //Serial.println("-");
}


