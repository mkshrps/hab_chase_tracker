#include <TinyGPSPlus.h>

struct gpsT
{
  char  callSign[20];
  time_t  lastPacketAt;
  uint32_t time; 
  uint8_t Hours, Minutes, Seconds;
  unsigned long SecondsInDay;         // Time in seconds since midnight
  double Longitude, Latitude;
  long Altitude;
  unsigned int Satellites;
  unsigned int failedCS;
  int Speed;
  int Direction;
  byte FixType;
  bool isValid;
} ;

#ifdef _GPS_
gpsT localGPSData;
// The TinyGPS++ object
TinyGPSPlus gps;

  

#else
extern gpsT localGPSData;
extern TinyGPSPlus gps;
void updateLocalGPS(TinyGPSPlus , const char * ) ;
void displayInfo(TinyGPSPlus );

#endif
