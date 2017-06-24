

/*
   SA2HoTT
   Ziege-One
   v1.0
 
 Arduino pro Mini 5v/16mHz w/ Atmega 328

 
 /////Pin Belegung////
 D0: 
 D1: 
 D2: 
 D3: RX / TX Softserial HoTT V4
 D4: 
 D5: 
 D6: 
 D7: 
 D8: 
 D9: 
 D10: 
 D11: 
 D12: 
 D13: LED, um die Kommunikation zu visualisieren
 
 A0: Eingang Spannung 0-5V
 A1: Eingang Strom 0-5V
 A2: 
 A3: 
 A4: 
 A5: 
 
 
 */
 
// ======== SA2HoTT  =======================================

#include <EEPROM.h>
#include <SoftwareSerial.h>
#include "Message.h"
#include "SmartAudio.h"
#include <inttypes.h>

// LED an Pin 13
#define LEDPIN_PINMODE    pinMode (13, OUTPUT);
#define LEDPIN_SWITCH     PINB |= 1<<5;     //switch LEDPIN state (digital PIN 13)
#define LEDPIN_OFF        PORTB &= ~(1<<5);
#define LEDPIN_ON         PORTB |= (1<<5);

//#define Debug                             // Ein/Aus Debugging

// Time interval [ms] for display updates:
const unsigned long DISPLAY_INTERVAL = 5000;
static unsigned long lastTime=0;  // in ms
unsigned long timer=millis();      // in ms


// Einfügen Message.cpp Funktionen für Initialisierung und Hauptprogramm
GMessage message;
SmartAudio smarta;


// ======== Setup & Initialisierung =======================================
void setup()
{
  
  //Serial.begin (115200); // 115200 für Debugging
  
  LEDPIN_PINMODE
  LEDPIN_ON
  delay(200);
  LEDPIN_OFF
  delay(200);
  LEDPIN_ON
  delay(200);
  LEDPIN_OFF
  delay(200);
  LEDPIN_ON
  delay(200);
  LEDPIN_OFF
  
  // Initialisierung Graupner HoTT Protokoll
  message.init();
  smarta.init();
  
}

// ======== Haupt Schleife  =======================================
void loop()  {
  
    #ifdef Debug
      //Für Debugging an Serial 115200
      timer=millis();
      if (timer-lastTime>DISPLAY_INTERVAL)  // wenn DISPLAY_INTERVAL ms vergangen sind
      {
      //message.debug();
      smarta.sa_command(SA_GET_SETTINGS,0);
      lastTime=timer;
      }
      
    #endif
  // Keine Kommunikation
  LEDPIN_OFF 
  
  // Senden und Update Graupner HoTT Telemetry
  message.main_loop();
  
  //smarta.debug();
}
