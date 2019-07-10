/*---------------------------------------------------*\
|                                                     |
| Connections:                                        |
|                                                     |
|               Arduino  X - RFM98W DIO5              |
|               Arduino  X - RFM98W DIO0              |
|                                                     |
|               Arduino 10  - RFM98W NSS              |
|               Arduino 11 - RFM98W MOSI              |
|               Arduino 12 - RFM98W MISO              |
|               Arduino 13 - RFM98W CLK               |
|                                                     |
\*---------------------------------------------------*/

#include <Arduino.h>
#include <ctype.h> 
#include <SPI.h> 
#include <string.h>
#include <lora_esp32_oled.h>
#define REG_VERSION              0x42

//#define LORA_NSS  18
//#define LORA_RESET        14                // Comment out if not connected
//#define LORA_DIO0           26
//#define LORA_DIO5           2

typedef enum {lmIdle, lmListening, lmSending} tLoRaMode;

tLoRaMode LoRaMode;

byte currentMode = 0x81;
int ImplicitOrExplicit;
int BadCRCCount;
unsigned long LastLoRaTX=0;

// lora message buffer
char packet[100];                 

int lora_nss;
int lora_dio0;
// int lora_dio5;

void SetupLoRa(int dio0, int reset, int nss)
{
  int ErrorCoding;
  int Bandwidth;
  int SpreadingFactor;
  int LowDataRateOptimize;
  int PayloadLength;
  
  lora_dio0 = dio0;
  //lora_dio5 = dio5;
  lora_nss = nss;
  pinMode(nss,OUTPUT);
  pinMode(dio0,INPUT_PULLUP);
  pinMode(reset, OUTPUT);

  digitalWrite(reset, LOW);
  delay(10);          // Module needs this before it's ready
  digitalWrite(reset, HIGH);
  delay(10);          // Module needs this before it's ready
  digitalWrite(nss,HIGH);

  SPI.begin(5, 19, 27, 18);
  //SPI.begin();
  
  // LoRa mode 
  enableLoRaMode();

  // Frequency
  setFrequency(LORA_FREQUENCY);

  // LoRa settings for various modes.  We support modes 2 (repeater mode), 1 (normally used for SSDV) and 0 (normal slow telemetry mode).

  #if LORA_MODE == 3
    Serial.println("Lora Mode = 3");
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_7;
    LowDataRateOptimize = 0;    
  #endif
  
  #if LORA_MODE == 2
    Serial.println("Lora Mode = 2");
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_62K5;
    SpreadingFactor = SPREADING_8;
    LowDataRateOptimize = 0;		
  #endif

  #if LORA_MODE == 1
    Serial.println("Lora Mode = 1");
    ImplicitOrExplicit = IMPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_5;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_6;
    LowDataRateOptimize = 0;    
  #endif

  #if LORA_MODE == 0  
    Serial.println("Lora Mode = 0");
    ImplicitOrExplicit = EXPLICIT_MODE;
    ErrorCoding = ERROR_CODING_4_8;
    Bandwidth = BANDWIDTH_20K8;
    SpreadingFactor = SPREADING_11;
    LowDataRateOptimize = 0x08;		
  #endif

  PayloadLength = ImplicitOrExplicit == IMPLICIT_MODE ? 255 : 0;

  writeRegister(REG_MODEM_CONFIG, ImplicitOrExplicit | ErrorCoding | Bandwidth);
  writeRegister(REG_MODEM_CONFIG2, SpreadingFactor | CRC_ON);
  writeRegister(REG_MODEM_CONFIG3, 0x04 | LowDataRateOptimize);									// 0x04: AGC sets LNA gain
  
  // writeRegister(REG_DETECT_OPT, (SpreadingFactor == SPREADING_6) ? 0x05 : 0x03);					// 0x05 For SF6; 0x03 otherwise
  writeRegister(REG_DETECT_OPT, (readRegister(REG_DETECT_OPT) & 0xF8) | ((SpreadingFactor == SPREADING_6) ? 0x05 : 0x03));  // 0x05 For SF6; 0x03 otherwise
  
  writeRegister(REG_DETECTION_THRESHOLD, (SpreadingFactor == SPREADING_6) ? 0x0C : 0x0A);		// 0x0C for SF6, 0x0A otherwise  
  
  writeRegister(REG_PAYLOAD_LENGTH, PayloadLength);
  writeRegister(REG_RX_NB_BYTES, PayloadLength);
  
  // Change the DIO mapping to 01 so we can listen for TxDone on the interrupt
  writeRegister(REG_DIO_MAPPING_1,0x40);
  writeRegister(REG_DIO_MAPPING_2,0x00);
  int val = readRegister(REG_PAYLOAD_LENGTH);
  Serial.print("payload len reg, nss, dio0 ");
  Serial.print(val);
  Serial.print(lora_nss);
  Serial.println(lora_dio0);
  uint8_t version = readRegister(REG_VERSION);
  Serial.print(" Version ");
  Serial.println(version, DEC );
  
  // Go to standby mode
  setOperatingMode(RF98_MODE_STANDBY);
  
  Serial.println("Setup Complete");
}

void setFrequency(double Frequency)
{
  unsigned long FrequencyValue;
    
  Serial.print("Frequency is ");
  Serial.println(Frequency);

  Frequency = Frequency * 7110656 / 434;
  FrequencyValue = (unsigned long)(Frequency);

  Serial.print("FrequencyValue is ");
  Serial.println(FrequencyValue);

  writeRegister(0x06, (FrequencyValue >> 16) & 0xFF);    // Set frequency
  writeRegister(0x07, (FrequencyValue >> 8) & 0xFF);
  writeRegister(0x08, FrequencyValue & 0xFF);
}

// set rfmxx chip to lora mode
void enableLoRaMode()
{
  Serial.println("Enabling LoRa Mode");
  setOperatingMode(RF98_MODE_SLEEP);
  writeRegister(REG_OPMODE,0x80);
}

/////////////////////////////////////
//    Method:   Change the mode
//////////////////////////////////////
void setOperatingMode(byte newMode)
{
  if(newMode == currentMode)
    return;  
  
  switch (newMode) 
  {
    case RF98_MODE_TX:
      writeRegister(REG_LNA, LNA_OFF_GAIN);  // TURN LNA OFF FOR TRANSMITT
      writeRegister(REG_PA_CONFIG, PA_MAX_UK);
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    case RF98_MODE_RX_CONTINUOUS:
      writeRegister(REG_PA_CONFIG, PA_OFF_BOOST);  // TURN PA OFF FOR RECIEVE??
      writeRegister(REG_LNA, LNA_MAX_GAIN);  // MAX GAIN FOR RECIEVE
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    case RF98_MODE_SLEEP:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    case RF98_MODE_STANDBY:
      writeRegister(REG_OPMODE, newMode);
      currentMode = newMode; 
      break;
    default: return;
  } 
    
  if(newMode != RF98_MODE_SLEEP){
    delay(10);
    Serial.print("Mode Changed");
    Serial.println(newMode,HEX);
  /*
    while(digitalRead(lora_dio5) == 0)
    {
    }
  */ 
  }
   
  return;
}



/////////////////////////////////////
//    Method:   Read Register
//////////////////////////////////////

uint8_t readRegister(byte addr)
{
  select();
  SPI.transfer(addr & 0x7F);
  uint8_t regval = SPI.transfer(0);
  unselect();
  return regval;
}

/////////////////////////////////////
//    Method:   Write Register
//////////////////////////////////////

void writeRegister(byte addr, byte value)
{
  select();
  SPI.transfer(addr | 0x80); // OR address with 10000000 to indicate write enable;
  SPI.transfer(value);
  unselect();
}

/////////////////////////////////////
//    Method:   Select Transceiver
//////////////////////////////////////
void select() 
{
  digitalWrite(lora_nss, LOW);
}

/////////////////////////////////////
//    Method:   UNSelect Transceiver
//////////////////////////////////////
void unselect() 
{
  digitalWrite(lora_nss, HIGH);
}


int LoRaIsTransmitting(void){

  if(LoRaMode == lmSending){                // transmission in progress 
    if(digitalRead(lora_dio0)){             // transmission finished
      writeRegister( REG_IRQ_FLAGS, 0x08);  // clear flags
      LoRaMode = lmIdle;
      return 0;                             // not transmitting now
    }
    else{
      return 1;                             // not finished yet
    }

  }
  return 0;                                 // Not transmitting
}


int LoRaMsgReady(void){
  if(LoRaMode == lmListening){              // receive mode so expecting a message 
    if(digitalRead(lora_dio0)){             // payload ready
      return 1;                             
    }
    else{
      return 0;                             // not payload yet
    }

  }
  return 0;                                 // Not receiving
}




// make sure we are not sending
int LoRaIsFree(void)
{
  if ((LoRaMode != lmSending) || digitalRead(lora_dio0))
  {
    // Either not sending, or was but now it's sent.  Clear the flag if we need to
    if (LoRaMode == lmSending)
    {
      // Clear that IRQ flag
      writeRegister( REG_IRQ_FLAGS, 0x08); 
      LoRaMode = lmIdle;
    }

  }
  
  return 0;
}


void startReceiving(void)
{
  writeRegister(REG_DIO_MAPPING_1, 0x00);   // 00 00 00 00 maps DIO0 to RxDone
  
  writeRegister(REG_FIFO_RX_BASE_AD, 0);
  writeRegister(REG_FIFO_ADDR_PTR, 0);
  Serial.println("receiving mode");  
  // Setup Receive Continuous Mode
  setOperatingMode(RF98_MODE_RX_CONTINUOUS);
    
  LoRaMode = lmListening;
}


//Lora receive routine need to set lora mode to rx first
int receiveMessage(unsigned char *message, int MaxLength)
{
  int i, Bytes, currentAddr, x;

  Bytes = 0;
	
  x = readRegister(REG_IRQ_FLAGS);
  
  // clear the rxDone flag
  writeRegister(REG_IRQ_FLAGS, 0x40); 
    
  // check for payload crc issues (0x20 is the bit we are looking for
  if((x & 0x20) == 0x20)
  {
    // CRC Error
    writeRegister(REG_IRQ_FLAGS, 0x20);		// reset the crc flags
    BadCRCCount++;
  }
  else
  {
    currentAddr = readRegister(REG_FIFO_RX_CURRENT_ADDR);
    Bytes = readRegister(REG_RX_NB_BYTES);
    Bytes = min(Bytes, MaxLength-1);

    writeRegister(REG_FIFO_ADDR_PTR, currentAddr);   
		
    for(i = 0; i < Bytes; i++)
    {
      message[i] = (unsigned char)readRegister(REG_FIFO);
    }
    message[Bytes] = '\0';

    // Clear all flags
    writeRegister(REG_IRQ_FLAGS, 0xFF); 
  }
  
  return Bytes;
}


void SendLoRaPacket(unsigned char *buffer, int Length)
{
  int i;
  
  LastLoRaTX = millis();
  
  Serial.print("Sending "); 
  Serial.print(Length);
  Serial.println(" bytes");
  
  setOperatingMode(RF98_MODE_STANDBY);

  writeRegister(REG_DIO_MAPPING_1, 0x40);   // 01 00 00 00 maps DIO0 to TxDone
  writeRegister(REG_FIFO_TX_BASE_AD, 0x00);  // Update the address ptr to the current tx base address
  writeRegister(REG_FIFO_ADDR_PTR, 0x00); 
  if (ImplicitOrExplicit == EXPLICIT_MODE)
  {
    writeRegister(REG_PAYLOAD_LENGTH, Length);
  }
  select();
  // tell SPI which address you want to write to
  SPI.transfer(REG_FIFO | 0x80);

  // loop over the payload and put it on the buffer 
  for (i = 0; i < Length; i++)
  {
    SPI.transfer(buffer[i]);
  }
  unselect();

  // go into transmit mode
  setOperatingMode(RF98_MODE_TX);
  
  LoRaMode = lmSending;
}

bool sendLoRaMessage(char *Msg,uint8_t from,uint8_t to,uint8_t ID,uint8_t flags){

  packet[0] = to;
  packet[1] = from;
  packet[2] = ID;
  packet[3] = flags;
  if(strlen(Msg) <=50){
    strcpy(&packet[4],Msg);
    SendLoRaPacket((unsigned char *) &packet[0], strlen(Msg)+5);
    Serial.println(packet);
    return true;
  }
  return false;
} 


uint8_t getRSSI(){
  return readRegister(REG_RSSI_CURRENT);

}

uint8_t getRSSI_Packet(){
  return readRegister(REG_RSSI_PACKET);
}

