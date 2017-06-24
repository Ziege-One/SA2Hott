/*
   SA2HoTT
   Ziege-One
   v1.0
 
 Arduino pro Mini 5v/16mHz w/ Atmega 328
 
 */
 
#include "Sensor.h"
#include "Message.h"

// Einfügen Message.cpp Funktionen
GMessage message2;

//Time values for Used Capacity calculation 
unsigned long current_time = 0;
unsigned long last_time = 0;
float time_elapsed = 0.0f;

//Calculate this scalse factor here once to save cycles
float secound_scale = 1000.0f / 3600.0f;  

//Internal Values
float Volt = 0.0f;
float Volt_min = 50.0f;
float VoltDigi = 0.0f; 
float Current = 0.0f;
float Current_max = 0.0f;
float CurrentDigi = 0.0f;
float Capacity_Used = 0.0f;

float VCC = 0.0f;
float Temp = 0.0f;


float Sensor::getVolt() { return Volt; }
float Sensor::getVolt_min() { return Volt_min; }
float Sensor::getVoltDigi() {return VoltDigi; }
float Sensor::getCurrent() { return Current; }
float Sensor::getCurrent_max() { return Current_max; }
float Sensor::getCurrentDigi() {return CurrentDigi; }
float Sensor::getBattCap() { return Capacity_Used; }

float Sensor::getVCC() { return VCC; }
float Sensor::getTemp() { return Temp; }

void Sensor::ReadSensor(){
  // Spannung + Strom
      
for (uint8_t i = 0; i < 50; i++)
  {
    VoltDigi += analogRead(A0);     // Spannung
    CurrentDigi += analogRead(A1);  // Strom
  }
   
  VoltDigi = VoltDigi / 50;       // Durchschnitt der Meßungen
  CurrentDigi = CurrentDigi / 50;
     
  Volt = (VoltDigi - (0.2048 * message2.getVoltOffset()))* ((488281.25 / message2.getVoltCOEF()) * 0.0001);            // Skalieren
  Current = (CurrentDigi - (0.2048 * message2.getCurrentOffset()))* ((488281.25 / message2.getCurrentCOEF()) * 0.0001);

  //Volt = ((( VoltDigi / 1023.0) * 5000 ) - message2.getVoltOffset()) / (message2.getVoltCOEF() / 10);
  //Current = ((( CurrentDigi / 1023.0) * 5000 ) - message2.getCurrentOffset()) / (message2.getCurrentCOEF() / 10);

  // Nicht < 0
  if (Volt < 0) Volt = 0.0;
  if (Current < 0) Current = 0.0;
    
  // Kapazität
  current_time = millis();
  //Calculate the used Capacity in [mAh]
  //Calculate elapsed Time in sec first
  time_elapsed = (float)(current_time - last_time)/1000.0f;
  //Now the used capacity
  Capacity_Used += Current * secound_scale * time_elapsed;
  last_time = current_time;
  
  // Min/Max
  if (current_time > 5000)
  {
    if (Volt < Volt_min) Volt_min = Volt;
    if (Current > Current_max) Current_max = Current;
  }
  
  VCC = ReadVCC();
  Temp = ReadTemp(); 
}  



float Sensor::ReadVCC() {  //Die Spannung wird in Volt zurückgegeben.
  long result;
  float result2;
  // Read 1.1V reference against AVcc
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = 1126400L / result; // Back-calculate AVcc in mV
  result2 = result;
  return result2/1000;
}

float Sensor::ReadTemp() { // Temperatur wird in ° C zurückgegeben.  
  unsigned int wADC;
  double t;
  // Set the internal reference and mux.
  ADMUX = (_BV(REFS1) | _BV(REFS0) | _BV(MUX3));
  ADCSRA |= _BV(ADEN);  // enable the ADC
  delay(20);            // wait for voltages to become stable.
  ADCSRA |= _BV(ADSC);  // Start the ADC
  // Detect end-of-conversion
  while (bit_is_set(ADCSRA,ADSC));
  // Reading register "ADCW" takes care of how to read ADCL and ADCH.
  wADC = ADCW;
  // The offset of 324.31 could be wrong. It is just an indication.
  t = (wADC - 324.31 ) / 1.22;
  // The returned temperature is in degrees Celcius.
  return (t);
}



