/*
   SmartAudioTBS
   Ziege-One
   v1.0
 
 Arduino pro Mini 5v/16mHz w/ Atmega 328
 
 */


#include "Arduino.h"

const uint16_t vtx58frequencyTable[5][8] =
{
    { 5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725 }, // Boscam A
    { 5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866 }, // Boscam B
    { 5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945 }, // Boscam E
    { 5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880 }, // FatShark
    { 5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917 }, // RaceBand
};

const char * const vtx58BandNames[] = {
    "BOSCAM A",
    "BOSCAM B",
    "BOSCAM E",
    "FATSHARK",
    "RACEBAND",
};

static const char * const saPowerNames[] = {
//    "---",
    "25mW ",
    "200mW",
    "500mW",
    "800mW",
};

#define SA_GET_SETTINGS 0x01
#define SA_GET_SETTINGS_V2 0x09
#define SA_SET_POWER 0x02
#define SA_SET_CHANNEL 0x03
#define SA_SET_FREQUENCY 0x04
#define SA_SET_MODE 0x05

#define SA_POWER_25MW 0
#define SA_POWER_200MW 1
#define SA_POWER_500MW 2
#define SA_POWER_800MW 3

static const uint8_t V1_power_lookup[] = {
  7,
  16,
  25,
  40
};

enum SMARTAUDIO_VERSION {
  NONE,
  SA_V1,
  SA_V2
};

typedef struct {
  SMARTAUDIO_VERSION vtx_version;
  uint8_t channel;
  uint8_t powerLevel;
  uint8_t mode;
  uint16_t frequency;
  
}UNIFY;

class SmartAudio {

public:
  SmartAudio();
  void sa_command(uint8_t cmd, uint32_t value);
  void sa_tx_packet(uint8_t cmd, uint32_t value);
  void sa_rx_packet(uint8_t *buff, uint8_t len);
  void init();
  void debug();

  uint8_t get_vtx_version();
  uint8_t get_channel();
  uint8_t get_powerLevel();
  uint8_t get_mode();
  uint16_t get_frequency();

};  
