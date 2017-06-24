/*
   SmartAudioTBS
   Ziege-One
   v1.0
 
 Arduino pro Mini 5v/16mHz w/ Atmega 328
 
 */

#include "SmartAudio.h"

static UNIFY unify;

uint8_t SmartAudio::get_vtx_version() {return unify.vtx_version; }
uint8_t SmartAudio::get_channel() {return unify.channel; }
uint8_t SmartAudio::get_powerLevel() {return unify.powerLevel; }
uint8_t SmartAudio::get_mode() {return unify.mode; }
uint16_t SmartAudio::get_frequency() {return unify.frequency; }

uint8_t crc8(const uint8_t *data, uint8_t len)
{
#define POLYGEN 0xd5
  uint8_t crc = 0;
  uint8_t currByte;

  for (int i = 0 ; i < len ; i++) {
    currByte = data[i];
    crc ^= currByte;
    for (int i = 0; i < 8; i++) {
      if ((crc & 0x80) != 0) {
        crc = (byte)((crc << 1) ^ POLYGEN);
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

SmartAudio::SmartAudio(){

}

void SmartAudio::sa_tx_packet(uint8_t cmd, uint32_t value){
  //here: length --> only payload, without CRC
  //here: CRC --> calculated for complete packet 0xAA ... payload
  uint8_t buff[10];
  uint8_t packetLength = 0;
  buff[0] = 0x00;
  buff[1] = 0xAA; //sync
  buff[2] = 0x55; //sync
  buff[3] = (cmd << 1) | 0x01; //cmd

  switch (cmd){
  case SA_GET_SETTINGS:
    buff[4] = 0x00; //length
    buff[5] = crc8(&buff[1],4);
    buff[6] = 0x00;
    packetLength = 7;
    break;
  case SA_SET_POWER:
    buff[4] = 0x01; //length
    buff[5] = (unify.vtx_version == SA_V1) ? V1_power_lookup[value] : value;
    buff[6] = crc8(&buff[1], 5);
    buff[7] = 0x00;
    packetLength = 8;
    break;
  case SA_SET_CHANNEL:
    buff[4] = 0x01; //length
    buff[5] = value;
    buff[6] = crc8(&buff[1], 5);
    buff[7] = 0x00;
    packetLength = 8;
    break;
  case SA_SET_FREQUENCY:
    buff[4] = 0x02;
    buff[5] = (value>>8); //high byte first
    buff[6] = value;
    buff[7] = crc8(&buff[1], 6);
    buff[8] = 0x00;
    packetLength = 9;
    break;
  case SA_SET_MODE: //supported for V2 only: UNIFY HV and newer
    if (unify.vtx_version == SA_V2){
      //TBD --> Pit mode
      /*
      buffer[4] = 0x01; //length
      buffer[5] = value;
      buffer[6] = crc8(&buffer[1], 5);
      buffer[7] = 0x00;
      packetLength = 8;s
      */
    }
    break;
  }
  for(int i=0;i<packetLength; i++){
    Serial.write(buff[i]);
  }
}

void SmartAudio::sa_rx_packet(uint8_t *buff, uint8_t len){
  //verify packet
  uint8_t packetStart=0;
  for(int i=0;i<len-3;i++){
    if(buff[i]==0xAA && buff[i+1]==0x55 && buff[i+3]<len){
      packetStart=i+2;
      uint8_t len=buff[i+3];
      uint8_t crcCalc=crc8(&buff[i+2],len+1);
      uint8_t crc=buff[i+3+len];

      if(crcCalc==crc){
        //Serial.println("CRC match");
        switch(buff[packetStart]){
          case SA_GET_SETTINGS: //fall-through
          case SA_GET_SETTINGS_V2:
            //Serial.println("SA_GET_SETTINGS");
            unify.vtx_version = (buff[packetStart]==SA_GET_SETTINGS) ? SA_V1 : SA_V2;
            packetStart+=2; //skip cmd and length
            unify.channel = buff[packetStart++];
            unify.powerLevel = buff[packetStart++];
            unify.mode = buff[packetStart++];
            unify.frequency = ((uint16_t)buff[packetStart++]<<8)|buff[packetStart++];
            break;
          case SA_SET_POWER:
            //Serial.println("SA_SET_POWER");
            packetStart+=2;
            unify.powerLevel = buff[packetStart++];      
            break;
          case SA_SET_CHANNEL:
            //Serial.println("SA_SET_CHANNEL");
            packetStart+=2;
            unify.channel = buff[packetStart++];
            break;
          case SA_SET_FREQUENCY:
            //Serial.println("SA_SET_FREQUENCY");
            //TBD: Pit mode Freq
            packetStart+=2;
            unify.frequency = ((uint16_t)buff[packetStart++]<<8)|buff[packetStart++];
            break;
          case SA_SET_MODE:
            //SA V2 only!
            break;
        }
        return;

      }else{
        //Serial.println("CRC mismatch");
        return;
      }
    }
  }
  
}

void SmartAudio::init()
{
  Serial.begin(4800);
  while (!Serial);             // Leonardo: wait for serial monitor
}

uint8_t buff[25];
uint8_t rx_len = 0;
uint8_t zeroes = 0;

void SmartAudio::sa_command(uint8_t cmd, uint32_t value){
    Serial.begin(4800);
    
    sa_tx_packet(cmd,value);
    
    Serial.end();//clear buffer, otherwise sa_tx_packet is received
    Serial.begin(4900);
    UCSR0B &= ~(1<<TXEN0);  //deactivate tx --> rx mode listening for respons
  
  delay(100);
  
  while(Serial.available()){
    buff[rx_len]=Serial.read();
    if(buff[rx_len]==0){
      zeroes++;
    }
    //Serial.print(buff[rx_len],HEX);
    //Serial.print(",");
    rx_len++;
  }
  //Serial.println();
  if(rx_len>6){

    //because rx is low in idle 0 is received
    //when calculating crc of 0 we have a crc match, so
    //when all bytes are 0 we should avoid parsing the input data
    if(rx_len==zeroes){
      while(Serial.available()){
        Serial.read();
      }
    }else{
      sa_rx_packet(buff,rx_len);
    }
    zeroes=0;
    rx_len=0;   
  }
  
}

void SmartAudio::debug(){
      Serial.print("Version:");
      Serial.print(unify.vtx_version);
      Serial.print(", Channel:");
      Serial.print(unify.channel);
      Serial.print(", Channel:");
      Serial.print(vtx58frequencyTable[unify.channel / 8][unify.channel % 8]);
      Serial.print(", PowerLevel:");
      Serial.print(unify.powerLevel);
      Serial.print(", Mode:");
      Serial.print(unify.mode);
      Serial.print(", Frequency:");
      Serial.println(unify.frequency);
}

