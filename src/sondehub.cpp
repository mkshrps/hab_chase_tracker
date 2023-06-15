
#include <Arduino.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <main.h>
#include <gps_local.h>
#include <sondehub.h>
#include <utctime.h>
#include <remote.h>
#include <calc_crc.h>

char sondehubJson[500];
void test_json()
{
    char remotedateBuffer[30];
    char localdateBuffer[30];
    currentTimeUTC(localdateBuffer);
    remoteTimeUTCFromTime(remotedateBuffer,remote_data.time);
    Serial.print(" .  Frequency !!! ");
    Serial.println(remote_data.frequency);
    float frequency = (float)remote_data.frequency;
    Serial.println(frequency);
    frequency = frequency / 10E5;
    Serial.println(frequency);


     	sprintf(sondehubJson,	"[{\"software_name\": \"MJSChase\","		// Fixed software name
					"\"software_version\": \"%s\","					// Version
					"\"uploader_callsign\": \"%s\","				// User callsign
					"\"time_received\": \"%s\","					// UTC
					"\"payload_callsign\": \"%s\","
					"\"datetime\":\"%s\","							// UTC from payload
					"\"lat\": %.5f,"								// Latitude
					"\"lon\": %.5f,"								// Longitude
					"\"alt\": %.3f,"								// Altitude
					"\"frequency\": %3.6f,"							// Frequency
					"\"modulation\": \"LoRa\","						// Modulation
                    "\"uploader_position\": ["
                    " %.3f,"												// Listener Latitude
                    " %.3f,"												// Listener Longitude
                    " %d"												// Listener Altitude
	                    "],"
                    "\"uploader_antenna\": \"%s\""
                    "}]",
            thisReceiver.version,
            thisReceiver.callSign,
            localdateBuffer,
            remote_data.callSign,
            remotedateBuffer,
            remote_data.latitude,
            remote_data.longitude,
            remote_data.alt,
            //thisReceiver.set_frequency,
            frequency,
            localGPSData.Latitude, localGPSData.Longitude, localGPSData.Altitude,
            thisReceiver.antenna
         );
 
            Serial.println(sondehubJson);
            Serial.println(strlen(sondehubJson));

}
// #include "habhub.h"
// read the basic sentence values into the buffer
// not going to ude this tis just her for reference
void createJsonPayload()
{
    char remotedateBuffer[30];
    char localdateBuffer[30];
    currentTimeUTC(localdateBuffer);
    remoteTimeUTCFromTime(remotedateBuffer,remote_data.time);
    // convert the estimated tracker frequency to Mhz
    float frequency = (float)remote_data.frequency;
    frequency = frequency / 10E5;

    // Create json as required by sondehub-amateur
	sprintf(sondehubJson,	"[{\"software_name\": \"MJSChase\","		// Fixed software name
					"\"software_version\": \"%s\","					// Version
					"\"uploader_callsign\": \"%s\","				// User callsign
					"\"time_received\": \"%s\","					// UTC
					"\"payload_callsign\": \"%s\","					// Payload callsign
					"\"datetime\":\"%s\","							// UTC from payload
					"\"lat\": %.5f,"								// Latitude
					"\"lon\": %.5f,"								// Longitude
					"\"alt\": %.3f,"								// Altitude
					"\"frequency\": %3.6f,"							// Frequency
					"\"modulation\": \"LoRa\","						// Modulation
			"\"uploader_position\": ["
			" %.3f,"												// Listener Latitude
			" %.3f,"												// Listener Longitude
			" %d"												// Listener Altitude
			"],"
			"\"uploader_antenna\": \"%s\""
			"}]",
            thisReceiver.version,
            thisReceiver.callSign,
            localdateBuffer,
            remote_data.callSign,
            remotedateBuffer,
            remote_data.latitude,
            remote_data.longitude,
            remote_data.alt,
            frequency,
            localGPSData.Latitude, localGPSData.Longitude, localGPSData.Altitude,
            thisReceiver.antenna
           // remote_data.raw
    );
}

// used for test data
void getJson(){


	sprintf(sondehubJson,	"[{\"software_name\": \"auto_rx\","		// Fixed software name
					"\"software_version\": \"1.0.0\","					// Version
					"\"uploader_callsign\": \"MJS001\","				// User callsign
					"\"time_received\": \"2022-09-10T15:03:05.000000Z\","					// UTC
					"\"payload_callsign\": \"MJSPAY\","					// Payload callsign
					"\"datetime\": \"2022-09-10T15:03:05.000000Z\","							// UTC from payload
					"\"lat\": 53.227489,"								// Latitude
					"\"lon\": -2.521819,"								// Longitude
					"\"alt\": 20.0"								// Altitude

			"}]");


}

int sendJsonToSondehub(const char *url,char * json){
// upload the json payload to the amateur.sondehub
    int httpResponseCode=0;
    if(WiFi.status()== WL_CONNECTED){
        // load up the data to send from the various structures

        //WiFiClient wifiClient;
        HTTPClient http;   
       

        http.begin( url);
        http.addHeader("User-Agent", "autorx-1.0");
        http.addHeader("Content-Type","application/json");            
        http.addHeader("Accept","application/json");            
        http.addHeader("charsets","utf-8");    
        Serial.println(sondehubJson);
        httpResponseCode = http.PUT(sondehubJson);   
        
        if(httpResponseCode>0){
        
            String response = http.getString(); 
        
            Serial.println(httpResponseCode);
            Serial.println(response);          
        }
        else{
            Serial.print("Error on sending PUT Request: ");
            Serial.println(httpResponseCode);     
        }
        http.end();       
    }
    
    else{
        Serial.println("Error in WiFi connection");
    }
    return httpResponseCode;
}

int uploadHabPayloadToSondehub(){
       // Create json payload
       createJsonPayload();
       // getJson();      // test data
        const char * url = "https://api.v2.sondehub.org/amateur/telemetry"; 
 //       char url[100] ;
 //       strcpy(url, "https://api.v2.sondehub.org/amateur/telemetry");
        
       return sendJsonToSondehub(url,sondehubJson); 
}

int uploadListenerToSondehub()
{
    char url[100];
	// Create json as required by sondehub-amateur
	sprintf(sondehubJson,	"{\"software_name\": \"MJS -Chase-Gateway\","		// Fixed software name
            "\"software_version\": \"%s\","					// Version
            "\"uploader_callsign\": \"%s\","				// User callsign
			"\"uploader_position\": ["
			" %f,"												// Listener Latitude
			" %f,"												// Listener Longitude
			" %d"												// Listener Altitude
			"],"
			"\"uploader_radio\": \"RFM95 Lora\","
			"\"uploader_antenna\": \"%s\","
            "\"mobile\": true"
			"}",
			thisReceiver.version, 
            thisReceiver.callSign,
			localGPSData.Latitude,
            localGPSData.Longitude, 
            localGPSData.Altitude,
			thisReceiver.antenna);
			
	strcpy(url, "https://api.v2.sondehub.org/amateur/listeners");

	return sendJsonToSondehub(url, sondehubJson);
}

void testSentence(char * sentenceBuffer, int flightCount){

    char timeHMS[30];
    char checksum[10];
    float lat = 53.224509;
    float lng =  -2.552632;
    int alt = 30;
    const char *callSign = "FLIGHT1";
    
    getHMS(timeHMS);
    //char sentenceBuffer[150];
       
    sprintf(sentenceBuffer, "$$%s,%d,%s,%8.6f,%8.6f,%d",
        callSign,
        flightCount,
        timeHMS,
        lat,
        lng,
        alt);

    
    // crc all characters after the $$ at start of string
    int slen = 0;
    slen = calcCRC(&sentenceBuffer[2]);
    Serial.println(sentenceBuffer);
    
}

/*  typical json string 
{"data": {"_raw": "JCRNSlMwMSwtMi41MDY2MTgsNTMuMjI2ODg3LDQ0LDEyLDE3LDQzLDQ3KjIxMDAKCg=="},"receivers": {"MJS01": {"time_created": "1970-01-01T00:05:01Z","time_uploaded": "1970-01-01T00:05:01Z"}}}
*/