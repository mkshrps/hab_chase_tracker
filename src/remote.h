struct remoteT
{
  char callSign[12];
  unsigned long  lastPacketAt;
  double longitude = 0.0;
  double latitude = 0.0;
  float alt = 0.0;
  double  courseTo = 0.0;
  double  distancem = 0.0;
  char  cardinalCourseTo[20];
  float temperature_c = 0.0;
  float humidity = 0.0;
  bool  active;
  int satellites;
  int hours;
  int minutes;
  int seconds;
  int flightCount = 0;
  float InternalTemperature;
  float BatteryVoltage;
  float ExternalTemperature;
  float Pressure;
  unsigned int BoardCurrent;
  unsigned int errorstatus;
  unsigned char FlightMode;
  unsigned char PowerMode;
  bool isValid;
  long frequency;
  int rssi;
  float snr;
  char time[12];
  int speed;
  //char raw[150];
  int crc_count = 0;
  bool crc = false;
} ;


#ifdef _REMOTE_
remoteT remote_data;

#else
extern remoteT remote_data;
void getTelemetryData(char *);

#endif

