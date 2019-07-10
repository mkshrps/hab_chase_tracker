/*
 * lora.h
 *
 *  Created on: 2 Apr 2019
 *      Author: mikes
 */

// 
#ifndef LORA_H_
#define LORA_H_

// RFM98 registers
#define REG_FIFO                    0x00
#define REG_OPMODE                  0x01
#define REG_FIFO_ADDR_PTR           0x0D 
#define REG_FIFO_TX_BASE_AD         0x0E
#define REG_FIFO_RX_BASE_AD         0x0F
#define REG_FIFO_RX_CURRENT_ADDR    0x10
#define REG_IRQ_FLAGS_MASK          0x11
#define REG_IRQ_FLAGS               0x12
#define REG_RX_NB_BYTES             0x13
#define REG_MODEM_CONFIG            0x1D
#define REG_MODEM_CONFIG2           0x1E
#define REG_MODEM_CONFIG3           0x26
#define REG_PREAMBLE_MSB            0x20
#define REG_PREAMBLE_LSB            0x21
#define REG_PAYLOAD_LENGTH          0x22
#define REG_HOP_PERIOD              0x24
#define REG_FREQ_ERROR              0x28
#define REG_DETECT_OPT              0x31
#define	REG_DETECTION_THRESHOLD     0x37
#define REG_DIO_MAPPING_1           0x40
#define REG_DIO_MAPPING_2           0x41

#define REG_RSSI_PACKET             0x1A
#define REG_RSSI_CURRENT            0x1B

// MODES
#define RF98_MODE_RX_CONTINUOUS     0x85
#define RF98_MODE_TX                0x83
#define RF98_MODE_SLEEP             0x80
#define RF98_MODE_STANDBY           0x81

#define PAYLOAD_LENGTH              255

// Modem Config 1
#define EXPLICIT_MODE               0x00
#define IMPLICIT_MODE               0x01

#define ERROR_CODING_4_5            0x02
#define ERROR_CODING_4_6            0x04
#define ERROR_CODING_4_7            0x06
#define ERROR_CODING_4_8            0x08

#define BANDWIDTH_7K8               0x00
#define BANDWIDTH_10K4              0x10
#define BANDWIDTH_15K6              0x20
#define BANDWIDTH_20K8              0x30
#define BANDWIDTH_31K25             0x40
#define BANDWIDTH_41K7              0x50
#define BANDWIDTH_62K5              0x60
#define BANDWIDTH_125K              0x70
#define BANDWIDTH_250K              0x80
#define BANDWIDTH_500K              0x90

// Modem Config 2

#define SPREADING_6                 0x60
#define SPREADING_7                 0x70
#define SPREADING_8                 0x80
#define SPREADING_9                 0x90
#define SPREADING_10                0xA0
#define SPREADING_11                0xB0
#define SPREADING_12                0xC0

#define CRC_OFF                     0x00
#define CRC_ON                      0x04


// POWER AMPLIFIER CONFIG
#define REG_PA_CONFIG               0x09
#define PA_MAX_BOOST                0x8F    // 100mW (max 869.4 - 869.65)
#define PA_LOW_BOOST                0x81
#define PA_MED_BOOST                0x8A
#define PA_MAX_UK                   0x88    // 10mW (max 434)
#define PA_OFF_BOOST                0x00
#define RFO_MIN                     0x00

// 20DBm
#define REG_PA_DAC                  0x4D
#define PA_DAC_20                   0x87

// LOW NOISE AMPLIFIER
#define REG_LNA                     0x0C
#define LNA_MAX_GAIN                0x23  // 0010 0011
#define LNA_OFF_GAIN                0x00

#define LED_WARN            5
#define LED_OK              6
#define A0_MULTIPLIER      4.9

#define LORA_FREQUENCY      434.5
#define LORA_MODE            3                // setup to test with radiohead 20K8, ERR4-8,xplicit, SF7 CRC ON
#define LORA_RH_FORMAT       1               // enable radiohead header formatting so a radiohead client can receive the messages

// use hardware interrupt or software
//set true if using DIO pins
// false if using software registers
// to determine rxdone, txdone, modechanged
#define HARDWARE_IRQ        true

#endif

void SetupLoRa(int dio0,int dio5, int nss);

void setFrequency(double Frequency);
void enableLoRaMode();
void setOperatingMode(byte newMode);
uint8_t readRegister(byte addr);
void writeRegister(byte addr, byte value);
void select();
void unselect();
int LoRaIsTransmitting();
int LoRaMsgReady();
int LoRaIsFree();
void startReceiving();
int receiveMessage(unsigned char *message, int MaxLength);
void SendLoRaPacket(unsigned char *buffer, int Length);
int BuildLoRaPositionPacket(unsigned char *TxLine);
void CheckLoRaRx();
bool sendLoRaMessage(char *Msg,uint8_t from,uint8_t to,uint8_t ID,uint8_t flags);
uint8_t getRSSI(void);
uint8_t getRSSI_Packet(void);
