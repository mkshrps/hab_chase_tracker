#ifndef HABHUB_H
#define HABHUB_H
#endif

void hash_to_hex( unsigned char *, char * );
int uploadTelemetryPacket( char * telemetry , int packetNumber , char * callSign );
int uploadListenerPacket(const char *callsign, time_t gps_time, float gps_lat, float gps_lon, const char *antenna, const char* radio);



