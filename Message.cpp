/*
   SA2HoTT
   Ziege-One
   v1.0
 
 Arduino pro Mini 5v/16mHz w/ Atmega 328
 
 */

#include "Message.h"
#include "SmartAudio.h"
#include "Sensor.h"
#include <EEPROM.h>
#include <SoftwareSerial.h>
 
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__) //Code in here will only be compiled if an Arduino Uno (or older) is used.
  #define HOTTV4_RXTX 3           // Pin für HoTT Telemetrie Ausgang 2-7
  /**
    * Enables RX and disables TX
    */
  static inline void hottV4EnableReceiverMode() {
    DDRD  &= ~(1 << HOTTV4_RXTX);
    PORTD |= (1 << HOTTV4_RXTX);
  }

  /**
    * Enabels TX and disables RX
    */
  static inline void hottV4EnableTransmitterMode() {
    DDRD |= (1 << HOTTV4_RXTX);
  }
#endif
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega16U4__) //Code in here will only be compiled if an Arduino Leonardo is used.
  #define HOTTV4_RXTX 9           // Pin für HoTT Telemetrie Ausgang 8-11
  /**
    * Enables RX and disables TX
    */
  static inline void hottV4EnableReceiverMode() {
    DDRB  &= ~(1 << (HOTTV4_RXTX - 4));
    PORTB |= (1 << (HOTTV4_RXTX - 4));
  }

  /**
    * Enabels TX and disables RX
    */
  static inline void hottV4EnableTransmitterMode() {
    DDRB |= (1 << (HOTTV4_RXTX - 4));
  }
#endif 

#define LEDPIN_OFF        digitalWrite(LED_BUILTIN, LOW);
#define LEDPIN_ON         digitalWrite(LED_BUILTIN, HIGH);

// Einfügen Sensor.cpp Funktionen
Sensor lipo;
SmartAudio smarta2;

SoftwareSerial SERIAL_HOTT(HOTTV4_RXTX , HOTTV4_RXTX); // RX, TX

static uint8_t _hott_serial_buffer[173];   //creating a buffer variable to store the struct

// pointer to the buffer structures "_hott_serial_buffer"
struct HOTT_GAM_MSG     *hott_gam_msg = (struct HOTT_GAM_MSG *)&_hott_serial_buffer[0];
struct HOTT_TEXTMODE_MSG	*hott_txt_msg =	(struct HOTT_TEXTMODE_MSG *)&_hott_serial_buffer[0];

struct HOTT_ESC_MSG     *hott_esc_msg = (struct HOTT_ESC_MSG *)&_hott_serial_buffer[0];


// Alarm
int alarm_on_off_batt1 = 1; // 0=FALSE/Disable 1=TRUE/Enable   // Enable alarm by default for safety
char* alarm_on_off[13];      // For Radio OSD

static uint16_t alarm_min_volt = 900; // Min volt for alarm in mV
static uint16_t alarm_max_used = 1800; // Max Current Use for alarm in mA

// Timer for alarm
// for refresh time
int alarm_interval = 15000; // in ms
static unsigned long lastTime=0;  // in ms
unsigned long time=millis();      // in ms

// Messbereiche im Eprom
int Volt_Offset;
int Volt_SCALE;
int Current_Offset;
int Current_SCALE;

int GMessage::getVoltOffset() {return Volt_Offset; }
int GMessage::getVoltCOEF() {return Volt_SCALE; }
int GMessage::getCurrentOffset() {return Current_Offset; }
int GMessage::getCurrentCOEF() {return Current_SCALE; }

// For communication
static uint8_t octet1 = 0;  // reception
static uint8_t octet2 = 0;  // reception

// For saving settings in EEPROM
/*
 !WARNING!
 Writing takes 3.3ms.
 Maximum life of the EEPROM is 100000 writings/readings.
 Be careful not to use it too much, it is not replacable!
 */
#define adr_eprom_test 0                 // For the test for 1st time init of the Arduino (First power on)
#define adr_eprom_alarm_min_volt 2       // Default alarm min is 9.00v
#define adr_eprom_alarm_max_used 4       // Default alarm max is 1800mA
#define adr_eprom_alarm_on_off_batt1 6   // 0=FALSE/Disable 1=TRUE/Enable
#define adr_eprom_alarm_interval 8       // Audio warning alarm interval 
// Messbereiche im Eprom
#define adr_eprom_volt_offset 10         // Volt Offset in digi 
#define adr_eprom_Volt_SCALE 12           // Volt COEF in x/10 mV
#define adr_eprom_current_offset 14      // Current Offset in digi 
#define adr_eprom_Current_SCALE 16        // Current COEF in x/10 mA 

GMessage::GMessage(){

}
void GMessage::init(){
  
  SERIAL_HOTT.begin(SERIAL_COM_SPEED); // 19400 FOR GRAUPNER HOTT using SoftSerial lib.
  hottV4EnableReceiverMode(); 
  
  // Test for 1st time init of the Arduino (First power on)
  int test = read_eprom(adr_eprom_test);
  if (test != 123)
  {
    write_eprom(adr_eprom_test,123);
    write_eprom(adr_eprom_alarm_min_volt,alarm_min_volt);
    write_eprom(adr_eprom_alarm_max_used,alarm_max_used);
    write_eprom(adr_eprom_alarm_interval,alarm_interval);
    write_eprom(adr_eprom_alarm_on_off_batt1,alarm_on_off_batt1);
    // Messbereiche im Eprom
    write_eprom(adr_eprom_volt_offset,OffsetVolt);
    write_eprom(adr_eprom_Volt_SCALE,SCALE_Volt);
    write_eprom(adr_eprom_current_offset,OffsetCurrent);
    write_eprom(adr_eprom_Current_SCALE,SCALE_Current);
  }
  // Read saved values from EEPROM
    // alarm min on battery
    alarm_min_volt = read_eprom(adr_eprom_alarm_min_volt); // default is 9.00v if not change
    // alarm max on used mA
    alarm_max_used = read_eprom(adr_eprom_alarm_max_used); // default is 1800mA if not change
    // Timer for alarm
    alarm_interval = read_eprom(adr_eprom_alarm_interval); // default is 15000 ms if not change
    // Enable / Disable alarm bip
    alarm_on_off_batt1 = read_eprom(adr_eprom_alarm_on_off_batt1); // 0=FALSE/Disable 1=TRUE/Enable
    // Messbereiche im Eprom
    Volt_Offset = read_eprom(adr_eprom_volt_offset);
    Volt_SCALE = read_eprom(adr_eprom_Volt_SCALE);
    Current_Offset = read_eprom(adr_eprom_current_offset);
    Current_SCALE = read_eprom(adr_eprom_Current_SCALE);
    

}

uint16_t GMessage::read_eprom(int address){
  return  (uint16_t) EEPROM.read(address) * 256 + EEPROM.read(address+1) ;
}

void GMessage::write_eprom(int address,uint16_t val){
  EEPROM.write(address, val  / 256);
  EEPROM.write(address+1,val % 256 );
}


void GMessage::init_gam_msg(){
  //puts to all Zero, then modifies the constants
  memset(hott_gam_msg, 0, sizeof(struct HOTT_GAM_MSG));   
  hott_gam_msg->start_byte = 0x7c;
  hott_gam_msg->gam_sensor_id = HOTT_TELEMETRY_GAM_SENSOR_ID;
  hott_gam_msg->sensor_id = 0xd0;
  hott_gam_msg->stop_byte = 0x7d;
}
void GMessage::init_esc_msg(){
  //puts to all Zero, then modifies the constants
  memset(hott_esc_msg, 0, sizeof(struct HOTT_ESC_MSG));   
  hott_esc_msg->start_byte = 0x7c;
  hott_esc_msg->esc_sensor_id = 0x8c;
  hott_esc_msg->sensor_id = 0xc0;
  hott_esc_msg->stop_byte = 0x7d;
}


// Sending the frame
void GMessage::send(int lenght){ 
  uint8_t sum = 0;
  hottV4EnableTransmitterMode(); 
  delay(5);
  for(int i = 0; i < lenght-1; i++){
    sum = sum + _hott_serial_buffer[i];
    SERIAL_HOTT.write (_hott_serial_buffer[i]);
    delayMicroseconds(HOTTV4_TX_DELAY);
  }  
  //Emision checksum
  SERIAL_HOTT.write (sum);
  delayMicroseconds(HOTTV4_TX_DELAY);

  hottV4EnableReceiverMode();
}


void GMessage::main_loop(){ 
  
  // STARTING MAIN PROGRAM
  static byte page_settings = 1; // page number to display settings
  
  lipo.ReadSensor(); //Spannung und Strom Werte einlesen

  if(SERIAL_HOTT.available() >= 2) {
    uint8_t octet1 = SERIAL_HOTT.read();
    switch (octet1) {
    case HOTT_BINARY_MODE_REQUEST_ID:
      { 
        uint8_t  octet2 = SERIAL_HOTT.read();
        
        // Demande RX Module =	$80 $XX
        switch (octet2) {
        
        case HOTT_TELEMETRY_GAM_SENSOR_ID: //0x8D
          {    
               LEDPIN_ON
           
            // init structure
               init_gam_msg();
          
               hott_gam_msg->cell[1] = 0;              
               hott_gam_msg->cell[2] = 0;
               hott_gam_msg->cell[3] = 0;
               hott_gam_msg->cell[4] = 0;
               hott_gam_msg->cell[5] = 0;
               hott_gam_msg->cell[6] = 0; 
               hott_gam_msg->Battery1 = (lipo.getVolt ()) * 10 ;        
               hott_gam_msg->Battery2 = (lipo.getVCC ()) * 10 ;       
               hott_gam_msg->temperature1 = (lipo.getTemp()) + 20 ;        
               hott_gam_msg->temperature2 = 20;        
               hott_gam_msg->fuel_procent = 0;        
               hott_gam_msg->fuel_ml = 0;         
               hott_gam_msg->rpm = 0;             
               hott_gam_msg->altitude = 500;        
               hott_gam_msg->climbrate_L = 30000;     
               hott_gam_msg->climbrate3s = 120;         
               hott_gam_msg->current = (lipo.getCurrent()) * 10 ;         
               hott_gam_msg->main_voltage = (lipo.getVolt ()) * 10 ;    
               hott_gam_msg->batt_cap = (lipo.getBattCap()) / 10 ;       
               hott_gam_msg->speed = 0;           
               hott_gam_msg->min_cell_volt = 0;       
               hott_gam_msg->min_cell_volt_num = 0;   
               hott_gam_msg->rpm2 = 0;                        
               hott_gam_msg->general_error_number = 0;
               hott_gam_msg->pressure = 0;           
              
            // Alarmmanagement
               time=millis();
               if (time-lastTime>alarm_interval)  // if at least alarm_interval in ms have passed
               {
                     // Check for alarm beep
                     if (((lipo.getVolt ()*100) < alarm_min_volt) && alarm_on_off_batt1 == 1)
                     {
                     hott_txt_msg->warning_beeps =  0x10    ; // alarm beep or voice
                     }
                     if (((lipo.getBattCap()) > alarm_max_used) && alarm_on_off_batt1 == 1)
                     {
                     hott_txt_msg->warning_beeps =  0x16    ; // alarm beep or voice
                     }                      
                 lastTime=time;  // reset timer
               }

                 
            // sending all data
            send(sizeof(struct HOTT_GAM_MSG));
            break;
          } //end case GAM*/
 
         case HOTT_TELEMETRY_ESC_SENSOR_ID: //0x8D
          {    
               LEDPIN_ON
           
            // init structure
               init_esc_msg();
         
               hott_esc_msg->voltage = (lipo.getVolt ()) * 10 ;               
               hott_esc_msg->voltage_min = (lipo.getVolt_min ()) * 10 ;                
               hott_esc_msg->current = (lipo.getCurrent()) * 10 ;        			
               hott_esc_msg->current_max = (lipo.getCurrent_max()) * 10 ;        			        
               hott_esc_msg->temp1 = (lipo.getTemp()) + 20;                   		        
               hott_esc_msg->temp1_max = 20;                    		        
               hott_esc_msg->RPM = 0;               
               hott_esc_msg->RPM_max= 0;               		        
               hott_esc_msg->batt_cap = (lipo.getBattCap()) / 10;            				
               hott_esc_msg->temp2 = 20;                   	        
               hott_esc_msg->temp2_max = 20;             
               
            // sending all data
            send(sizeof(struct HOTT_ESC_MSG));
            break;
          } //end case ESC*/
     
        } //end case octet 2
        break;
      }

    case HOTT_TEXT_MODE_REQUEST_ID:
      {
        //LEDPIN_ON
        uint8_t  octet3 = SERIAL_HOTT.read();
        byte id_sensor = (octet3 >> 4);
        byte id_key = octet3 & 0x0f;
        static byte ligne_select = 4 ;
        static int8_t ligne_edit = -1 ;
        hott_txt_msg->start_byte = 0x7b;
        hott_txt_msg->esc = 0x00;
        hott_txt_msg->warning_beeps = 0x00;
        
        memset((char *)&hott_txt_msg->text, 0x20, HOTT_TEXTMODE_MSG_TEXT_LEN);
        hott_txt_msg->stop_byte = 0x7d;

        if (id_key == HOTT_KEY_LEFT && page_settings == 1)
        {   
          hott_txt_msg->esc = 0x01;
        }
        else
        {
          if (id_sensor == (HOTT_TELEMETRY_GAM_SENSOR_ID & 0x0f)) 
          { 
            hott_txt_msg->esc = HOTT_TELEMETRY_GAM_SENSOR_TEXT;
            switch (page_settings) { //SETTINGS
              
              case 1://PAGE 1 SETTINGS
              
                    {
                    // test if alarm active for display ON or OFF the screen
                    if (alarm_on_off_batt1 == 0)
                      *alarm_on_off = " Alarm : OFF";
                      else
                      *alarm_on_off = " Alarm :  ON";
                                           
                    if (id_key == HOTT_KEY_UP && ligne_edit == -1)
                    ligne_select = min(6,ligne_select+1); // never gets above line 6 max
                    else if (id_key == HOTT_KEY_DOWN && ligne_edit == -1)
                    ligne_select = max(3,ligne_select-1); // never gets above line 3 min
                    else if (id_key == HOTT_KEY_SET && ligne_edit == -1)
                    ligne_edit =  ligne_select ;
                    
                    else if (id_key == HOTT_KEY_LEFT && ligne_edit == -1)
                        {
                        page_settings-=1;
                        if (page_settings <1)    // unter Seite 1 dann Seite 3
                          page_settings = 3;
                        }
                    else if (id_key == HOTT_KEY_RIGHT && ligne_edit == -1)
                      {
                        page_settings+=1;
                        if (page_settings >3)   // Über Seite 3 dann Seite 1
                          page_settings = 1;
                      }

                    //LINE 3 SELECTED = text[3]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 3 )
                      {
                        alarm_on_off_batt1 = 1;
                        *alarm_on_off = " Alarm :  ON";
                      }
                      
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 3 )
                      {
                        alarm_on_off_batt1 = 0;
                        *alarm_on_off = " Alarm : OFF";
                       }
                      
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 3)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_alarm_on_off_batt1,alarm_on_off_batt1);
                       }
                    
                    
                    //LINE 4 SELECTED = text[4]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 4 )
                      alarm_min_volt+=5;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 4 )
                      alarm_min_volt-=5;
                    else if (id_key == HOTT_KEY_RIGHT && ligne_select == 4 )
                      alarm_min_volt+=50;
                    else if (id_key == HOTT_KEY_LEFT && ligne_select == 4 )
                      alarm_min_volt-=50;                      
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 4)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_alarm_min_volt,alarm_min_volt);
                       }

                    else if (alarm_min_volt>1820) // not over 18.2v
                        {
                          alarm_min_volt=5;
                        } 
                    else if (alarm_min_volt<1)  // not behind 0v
                        {
                       alarm_min_volt=420;
                        }

                    //LINE 5 SELECTED = text[5]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 5 )
                      alarm_max_used+=100;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 5 )
                      alarm_max_used-=100;
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 5)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_alarm_max_used,alarm_max_used);
                       }
                    else if (alarm_max_used>60000)
                    {
                       alarm_max_used=3000;  
                    }
                    else if (alarm_max_used<0)
                       {alarm_max_used=0; }

                    //LINE 6 SELECTED = text[6]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 6 )
                      alarm_interval+=1000;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 6 )
                      alarm_interval-=1000;
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 6)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_alarm_interval,alarm_interval);
                       }
                    else if (alarm_interval>60000)
                    {
                       alarm_interval=1000;  
                    }
                    else if (alarm_interval<0)
                       {alarm_interval=0; }
                              
                    // Showing page 1
                    
                    //line 0:
                    snprintf((char *)&hott_txt_msg->text[0],21," Alarms           <>");
                    //line 1:
                    snprintf((char *)&hott_txt_msg->text[1],21,"Volt: %i.%iv",(int) lipo.getVolt (), ((int) (lipo.getVolt ()*100)) % 100);
                    //line 2:
                    snprintf((char *)&hott_txt_msg->text[2],21,"Change settings :");
                    //line 3:
                    snprintf((char *)&hott_txt_msg->text[3],21,*alarm_on_off);
                    //line 4:
                    if (((int) (alarm_min_volt % 100)) == 5 || ((int) (alarm_min_volt % 100)) == 0)// Display management to put a 0 after the comma and the X.05v X.00v values ​​because the modulo return 0 or 5 for typical values ​​100,105,200,205,300,305, etc ...
                    snprintf((char *)&hott_txt_msg->text[4],21," Alarm Volt : %i.0%iv",(int) (alarm_min_volt/100),(int) (alarm_min_volt % 100)); // adding a 0
                    else // normal display
                    snprintf((char *)&hott_txt_msg->text[4],21," Alarm Volt : %i.%iv",(int) (alarm_min_volt/100),(int) (alarm_min_volt % 100)); // no need of adding 0
                    //line 5:
                    snprintf((char *)&hott_txt_msg->text[5],21," Used mA: %imA",(alarm_max_used));
                    //line 6:
                    snprintf((char *)&hott_txt_msg->text[6],21," Alarm repeat: %is",(alarm_interval/1000));
                    //line 7:
                    snprintf((char *)&hott_txt_msg->text[7],21,"SamartAudio2HoTT %d/3",page_settings); //Showing page number running down the screen to the right
                    
                    hott_txt_msg->text[ligne_select][0] = '>';
                    _hott_invert_ligne(ligne_edit);
                    break;                    
                    }//END PAGE 1
                    
               case 2: // PAGE 2
                    {
                      // config test for the screen display has
                    if (id_key == HOTT_KEY_LEFT && ligne_edit == -1)
                        {
                        page_settings-=1;
                        if (page_settings <1)    // unter Seite 1 dann Seite 3
                          page_settings = 3;
                        }
                    else if (id_key == HOTT_KEY_RIGHT && ligne_edit == -1)
                      {
                        page_settings+=1;
                        smarta2.sa_command(SA_GET_SETTINGS,0); //////////////////////////////////////////////////////////////////////////////////////
                        if (page_settings >3)   // Über Seite 3 dann Seite 1
                          page_settings = 1;
                      }
                                                      
                    else if (id_key == HOTT_KEY_UP && ligne_edit == -1)
                    ligne_select = min(6,ligne_select+1); // never gets above line 6 max
                    else if (id_key == HOTT_KEY_DOWN && ligne_edit == -1)
                    ligne_select = max(3,ligne_select-1); // never gets above line 3 min
                    else if (id_key == HOTT_KEY_SET && ligne_edit == -1)
                    ligne_edit =  ligne_select ;

                    //LINE 3 SELECTED = text[3]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 3 )
                      Volt_Offset+=1;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 3 )
                      Volt_Offset-=1;
                    else if (id_key == HOTT_KEY_RIGHT && ligne_select == 3 )
                      Volt_Offset+=50;
                    else if (id_key == HOTT_KEY_LEFT && ligne_select == 3 )
                      Volt_Offset-=50;                      
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 3)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_volt_offset,Volt_Offset);
                       }

                    else if (Volt_Offset>5000) // not over 5000
                        {
                          Volt_Offset=5000;
                        } 
                    else if (Volt_Offset<0)  // not behind 0
                        {
                       Volt_Offset=2500;
                        }
                    
                    //LINE 4 SELECTED = text[4]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 4 )
                      Volt_SCALE+=1;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 4 )
                      Volt_SCALE-=1;
                    else if (id_key == HOTT_KEY_RIGHT && ligne_select == 4 )
                      Volt_SCALE+=50;
                    else if (id_key == HOTT_KEY_LEFT && ligne_select == 4 )
                      Volt_SCALE-=50;                         
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 4)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_Volt_SCALE,Volt_SCALE);
                       }

                    else if (Volt_SCALE>4000) // not over 2000
                        {
                          Volt_SCALE=4000;
                        } 
                    else if (Volt_SCALE<1)  // not behind 0
                        {
                       Volt_SCALE=2000;
                        }
                    
                    //LINE 5 SELECTED = text[5]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 5 )
                      Current_Offset+=1;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 5 )
                      Current_Offset-=1;
                    else if (id_key == HOTT_KEY_RIGHT && ligne_select == 5 )
                      Current_Offset+=50;
                    else if (id_key == HOTT_KEY_LEFT && ligne_select == 5 )
                      Current_Offset-=50;                         
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 5)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_current_offset,Current_Offset);
                       }

                    else if (Current_Offset>5000) // not over 2000
                        {
                          Current_Offset=5000;
                        } 
                    else if (Current_Offset<0)  // not behind 0
                        {
                       Current_Offset=2500;
                        }
                    
                    //LINE 6 SELECTED = text[6]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 6 )
                      Current_SCALE+=1;
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 6 )
                      Current_SCALE-=1;
                    else if (id_key == HOTT_KEY_RIGHT && ligne_select == 6 )
                      Current_SCALE+=50;
                    else if (id_key == HOTT_KEY_LEFT && ligne_select == 6 )
                      Current_SCALE-=50;                        
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 6)
                      {
                       ligne_edit = -1 ;
                       write_eprom(adr_eprom_Current_SCALE,Current_SCALE);
                       }

                    else if (Current_SCALE>4000) // not over 2000
                        {
                          Current_SCALE=4000;
                        } 
                    else if (Current_SCALE<1)  // not behind 0
                        {
                       Current_SCALE=2000;
                        }                    
                    
                    // Showing page 2 settings
                    //line 0:                                  
                    snprintf((char *)&hott_txt_msg->text[0],21," LIPO CAL         <>");
                    //line 1:
                    snprintf((char *)&hott_txt_msg->text[1],21,"Volt: %i.%iv",(int) lipo.getVolt (), ((int) (lipo.getVolt ()*100)) % 100);
                    //line 2:
                    snprintf((char *)&hott_txt_msg->text[2],21,"Current: %i.%iA",(int) lipo.getCurrent(), ((int) (lipo.getCurrent()*100)) % 100);
                    //line 3:
                    snprintf((char *)&hott_txt_msg->text[3],21," Volt Off: %imV",(Volt_Offset));
                    //line 4:
                    snprintf((char *)&hott_txt_msg->text[4],21,"    SCALE: %i.%imV/V",(int) (Volt_SCALE/10) , (int) (Volt_SCALE % 10)); 
                    //line 5:
                    snprintf((char *)&hott_txt_msg->text[5],21," Cur. Off: %imV",(Current_Offset));
                    //line 6:
                    snprintf((char *)&hott_txt_msg->text[6],21,"    SCALE: %i.%imV/A",(int) Current_SCALE/10 , (int) (Current_SCALE % 10));
                    //line 7:
                    snprintf((char *)&hott_txt_msg->text[7],21,"SamartAudio2HoTT %d/3",page_settings); //Showing page number running down the screen to the right
                    int Volt_Offset;

                    hott_txt_msg->text[ligne_select][0] = '>';
                    _hott_invert_ligne(ligne_edit);
                    break;
                    }//END PAGE 2

               case 3: // PAGE 3
                    {
                      // config test for the screen display has
                    if (id_key == HOTT_KEY_LEFT && ligne_edit == -1)
                        {
                        page_settings-=1;
                        if (page_settings <1)    // unter Seite 1 dann Seite 3
                          page_settings = 3;
                        }
                    else if (id_key == HOTT_KEY_RIGHT && ligne_edit == -1)
                      {
                        page_settings+=1;
                        if (page_settings >3)   // Über Seite 3 dann Seite 1
                          page_settings = 1;
                      }
                                                      
                    else if (id_key == HOTT_KEY_UP && ligne_edit == -1)
                    ligne_select = min(4,ligne_select+1); // never gets above line 4 max
                    else if (id_key == HOTT_KEY_DOWN && ligne_edit == -1)
                    ligne_select = max(2,ligne_select-1); // never gets above line 1 min
                    else if (id_key == HOTT_KEY_SET && ligne_edit == -1)
                    ligne_edit =  ligne_select ;

                    //LINE 2 SELECTED = text[2]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 2 )                    
                      {
                        if (smarta2.get_channel() > 31) // Nicht über 39 dann wieder bei 0 anfangen
                          smarta2.sa_command(SA_SET_CHANNEL,(smarta2.get_channel() + 8) - 40);
                        else
                          smarta2.sa_command(SA_SET_CHANNEL,smarta2.get_channel() + 8);
                      }
                      
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 2 )
                      {
                        if (smarta2.get_channel() < 8) // Nicht unter 0 dann wieder bei 39 anfangen
                          smarta2.sa_command(SA_SET_CHANNEL,(smarta2.get_channel() - 8) + 40); 
                        else
                          smarta2.sa_command(SA_SET_CHANNEL,smarta2.get_channel() - 8);
                      }   
                                       
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 2)
                      {
                       ligne_edit = -1 ;
                       }

                    //LINE 3 SELECTED = text[3]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 3 )                    
                      {
                        if (smarta2.get_channel() == 39) // Nicht über 39 dann 0
                          smarta2.sa_command(SA_SET_CHANNEL,0);
                        else
                          smarta2.sa_command(SA_SET_CHANNEL,smarta2.get_channel() + 1);
                      }
                      
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 3 )
                      {
                        if (smarta2.get_channel() == 0) // Nicht unter 0 dann 39
                          smarta2.sa_command(SA_SET_CHANNEL,39); 
                        else
                          smarta2.sa_command(SA_SET_CHANNEL,smarta2.get_channel() - 1);
                      }   
                                       
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 3)
                      {
                       ligne_edit = -1 ;
                       }


                    //LINE 4 SELECTED = text[4]
                    else if (id_key == HOTT_KEY_UP && ligne_select == 4 )                    
                      {
                        if (smarta2.get_powerLevel() == 3) // Nicht über 3 dann 0
                          smarta2.sa_command(SA_SET_POWER,0);
                        else
                          smarta2.sa_command(SA_SET_POWER,smarta2.get_powerLevel() + 1);
                      }
                      
                    else if (id_key == HOTT_KEY_DOWN && ligne_select == 4 )
                      {
                        if (smarta2.get_powerLevel() == 0) // Nicht unter 0 dann 3
                          smarta2.sa_command(SA_SET_POWER,3); 
                        else
                          smarta2.sa_command(SA_SET_POWER,smarta2.get_powerLevel() - 1);
                      }   
                                       
                    else if (id_key == HOTT_KEY_SET && ligne_edit == 4)
                      {
                       ligne_edit = -1 ;
                       }
                       
                    
                    // Showing page 3 settings
                    //line 0:                                 
                    snprintf((char *)&hott_txt_msg->text[0],21," Smart Audio TBS  <");
                    //line 1:
                    snprintf((char *)&hott_txt_msg->text[1],21," Freq: %i", (vtx58frequencyTable[smarta2.get_channel() / 8][smarta2.get_channel() % 8]));
                    //line 2:
                    snprintf((char *)&hott_txt_msg->text[2],21," Band: %s", (vtx58BandNames[smarta2.get_channel() / 8]));
                    //line 3:
                    snprintf((char *)&hott_txt_msg->text[3],21," Channel: %i",((smarta2.get_channel() % 8) + 1)); 
                    //line 4:
                    snprintf((char *)&hott_txt_msg->text[4],21," Power: %s",(saPowerNames[smarta2.get_powerLevel()]));
                    //line 5:
                    snprintf((char *)&hott_txt_msg->text[5],21," Pit: ");
                    //line 6:
                    snprintf((char *)&hott_txt_msg->text[6],21," Freq Pit: %i", (smarta2.get_frequency()));
                    //line 7:
                    snprintf((char *)&hott_txt_msg->text[7],21,"SamartAudio2HoTT %d/3",page_settings); //Showing page number running down the screen to the right
                    int Volt_Offset;

                    hott_txt_msg->text[ligne_select][0] = '>';
                    _hott_invert_ligne(ligne_edit);
                    break;
                    }//END PAGE 3
                    
                  default://PAGE 
                  {
                    break;
                  }
                  
                  
                  
            }//END SETTINGS

          } // END IF
          
             
          else {
            snprintf((char *)&hott_txt_msg->text[0],21,"Unknow sensor module <");
            snprintf((char *)&hott_txt_msg->text[1],21,"Nothing here");
          }
        }
        _hott_send_text_msg();
        //LEDPIN_OFF
        break;

      }
      break;
    }	
  }
}




void GMessage::_hott_send_text_msg() {
  for(byte *_hott_msg_ptr = hott_txt_msg->text[0]; _hott_msg_ptr < &hott_txt_msg->stop_byte ; _hott_msg_ptr++){
    if (*_hott_msg_ptr == 0)
      *_hott_msg_ptr = 0x20;
  }  
  send(sizeof(struct HOTT_TEXTMODE_MSG));
}


char * GMessage::_hott_invert_all_chars(char *str) {
  return _hott_invert_chars(str, 0);
}


char * GMessage::_hott_invert_chars(char *str, int cnt) {
  if(str == 0) return str;
  int len = strlen(str);
  if((len < cnt)  && cnt > 0) len = cnt;
  for(int i=0; i< len; i++) {
    str[i] = (byte)(0x80 + (byte)str[i]);
  }
  return str;
}


void GMessage::_hott_invert_ligne(int ligne) {
  if (ligne>= 0 && ligne<= 7)
    for(int i=0; i< 21; i++) {
      if (hott_txt_msg->text[ligne][i] == 0)   //reversing the null character (end of string)
        hott_txt_msg->text[ligne][i] = (byte)(0x80 + 0x20);
      else
        hott_txt_msg->text[ligne][i] = (byte)(0x80 + (byte)hott_txt_msg->text[ligne][i]);
    }
}

void GMessage::debug(){
   // FOR DEBUG
    Serial.println("-----------Strom2HoTT-V1.0------------");
    Serial.print("Volt:"); Serial.print(lipo.getVolt(), 2);Serial.print("V = digit:");Serial.println(lipo.getVoltDigi());
    Serial.print("Current:"); Serial.print(lipo.getCurrent(), 2);Serial.print("A = digit:");Serial.println(lipo.getCurrentDigi());
    Serial.print("BattCap:"); Serial.print(lipo.getBattCap(), 0);Serial.println("mA");
    Serial.print("VCC:"); Serial.print(lipo.getVCC(), 2);Serial.println("V");
    Serial.print("Temp:"); Serial.print(lipo.getTemp(), 2);Serial.println("*C");

   
}
