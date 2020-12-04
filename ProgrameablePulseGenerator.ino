/*
 * Programable puls generator.
 * 
 * It will generate pulses on a number of output pins. 
 * 
 * Programmable parameters:
 * Only the structure for programmable parameters are in place. The parameters hardcoded as "default parameters" in function setConfigurationDefaults().
 * Parameters are:
 * - Number of channels:                    1 
 * - Pulse length:                          50
 * - Pulse interval:                        1000
 * - Pulse (the short part / Active part):  LOW
 * - Pulse order:                           Randum
 * 
 * 
 */
#define SKETCH_VERSION "Programable puls generator - Version 0.0.0"

#define DEBUG  //If defined ("//" is removed at the beginning of this line.) debug informations are printed to Serial.
/*
 * Version histoty:
 * 0.1.0 - Initial commit: Basic funktionalites are build in. No parameters are programmable, but the required program structure is in place.
 * 
 * 
 *  **** Ardhino nano Pin definition/layout:***
 * Pin  0: Serial RX
 * Pin  1: Serial TX
 * Pin  2: Channel pin 1:
 * Pin  3: Channel pin 2: 
 * Pin  4: 
 * Pin  5: 
 * Pin  6: 
 * Pin  7: 
 * Pin  8: 
 * Pin  9: 
 * Pin  10: 
 * Pin  11: 
 * Pin  12: 
 * Pin  13: LED_BUILTIN. 
 * Pin 14 (A0):
 * Pin 15 (A1): 
 * Pin 16 (A2):
 * Pin 17 (A3):
 * Pin 18 (A4):
 * Pin 19 (A5):

 */
#include <EEPROM.h>
#include "EEPROMAnything.h"


 /*
 * ######################################################################################################################################
 *                       C  O  N  F  I  G  U  T  A  B  L  E       D  E  F  I  N  I  T  I  O  N  S
 * ######################################################################################################################################
*/
#define CONFIGURATION_VERSION 1
#define MAX_NUMBER_OF_CHANNELS 10

#define CONFIG_ADDRESS 0

/* Incapsulate strings i a P(string) macro definition to handle strings in PROGram MEMory (PROGMEM) to reduce valuable memory  
 * MACRO for string handling from PROGMEM
 * https://todbot.com/blog/2008/06/19/how-to-do-big-strings-in-arduino/
 * max 149 chars at once ...
 * ----------------------------------------------------------------------------------------------------------------------------------------------------- (149 characters)
 */
 char p_buffer[150];
#define P(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)

/*
 *  #####################################################################################################################
 *                       V  A  R  I  A  B  L  E      D  E  F  I  N  A  I  T  O  N  S
 *  #####################################################################################################################
 */
const int channelPin[MAX_NUMBER_OF_CHANNELS] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

unsigned long channelTimeStamp[MAX_NUMBER_OF_CHANNELS];
boolean pinState[MAX_NUMBER_OF_CHANNELS];

struct config_t                                     // Configuration are stored in a structure, and read from EEPROM. If no available, default parameters are written.
  {
    int structVersion;                              //A way to determind the correct confiugration structure.
    int numberOfChannels;
    char pulseOrder;                                          //A character value, which describe Simultaneously (S), Sequential (Q), Randum (R) 
    unsigned long pulseOrderInterval ;                        //In Sequentially mode a value between the pulses on each channel
    unsigned long pulseLength[MAX_NUMBER_OF_CHANNELS];        //Puls Length is in millisecunds.
    unsigned long pulseInterval[MAX_NUMBER_OF_CHANNELS];      //Number of pulses per minut is re-calculated and stored as millisecunds between eash pulse
    uint8_t pulseActive[MAX_NUMBER_OF_CHANNELS];              //Defined with HIGH or LOW weather the active part of the pulse is HIGH or LOW
    uint8_t pulsePassive[MAX_NUMBER_OF_CHANNELS];             //Defined with HIGH or LOW weather the passive part of the pulse is HIGH or LOW
  } configuration;
/*
 * ###################################################################################################
 *                       F  U  N  C  T  I  O  N      D  E  F  I  N  I  T  I  O  N  S
 * ###################################################################################################
*/
void setConfigurationDefaults() {
                                                              #ifdef DEBUG
                                                                Serial.println(P("Setting configuration defaultes!"));
                                                              #endif
  
  configuration.structVersion = CONFIGURATION_VERSION;
  configuration.numberOfChannels = 1;
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    configuration.pulseLength[ii] = 50;
    configuration.pulseInterval[ii] = 1000;
    configuration.pulseActive[ii] = LOW;
    configuration.pulsePassive[ii] = HIGH;
  }
  configuration.pulseOrder = 'R';
  configuration.pulseOrderInterval = 100;
  int charsWritten = EEPROM_writeAnything( CONFIG_ADDRESS, configuration);
                                                              #ifdef DEBUG
                                                                if ( charsWritten == sizeof( configuration))
                                                                  Serial.println(P("Configuration sucessfully written to EEMPROM"));
                                                                else
                                                                  Serial.println(P("F A I L E D   Writing configuration to EEPROM"));
                                                              #endif
  
}

void printConfiguration() {
  Serial.print(P("Structure version: "));
  Serial.println(configuration.structVersion);
  Serial.print(P("Number of channels: "));
  Serial.println(configuration.numberOfChannels);
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    Serial.print(P("Channel "));
    Serial.print(ii);
    Serial.print(P(" Pulse length: "));
    Serial.print(configuration.pulseLength[ii]);
    Serial.print(P(" Pulse interval: "));
    Serial.println(configuration.pulseInterval[ii]);
  }
  Serial.print(P("Pulse order: "));
  Serial.println(configuration.pulseOrder);
  Serial.print(P("Pulse order interval: "));
  Serial.println(configuration.pulseOrderInterval);
}
/*
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 *                       S E T U P      B e g i n
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 */

void setup() {
  pinMode( LED_BUILTIN, OUTPUT);
  digitalWrite( LED_BUILTIN, HIGH);
  Serial.begin( 115200);
                                                              #ifdef DEBUG
                                                                while (!Serial) {
                                                                  ;  // Wait for serial connectionsbefore proceeding
                                                                }
                                                                Serial.println(P(SKETCH_VERSION));
                                                                Serial.println(P("Hit [Enter] to start!"));
                                                                while (!Serial.available()) {
                                                                  ;  // In order to prevent unattended execution, wait for [Enter].
                                                                }
                                                              #endif
/*
 * Read configuration. If not allready set or incorrect version, set defaults.
 */
  int charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
  if (configuration.structVersion != CONFIGURATION_VERSION)
    setConfigurationDefaults();

  printConfiguration();


 /* 
  *  initialize output pins, ChannelTimeStamp
  */
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    pinMode( channelPin[ii], OUTPUT);
    channelTimeStamp[ii] = 0;
    pinState[ii] = LOW;
  }
                                                              #ifdef DEBUG
                                                                Serial.println(P("Initialized - Starting..."));
                                                              #endif

  digitalWrite( LED_BUILTIN, LOW);
}

/*
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 *                       L O O P     B e g i n
 * ###################################################################################################
 * ###################################################################################################
 * ###################################################################################################
 */
void loop() {
  
  unsigned long currentTimeStamp = millis();

  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    if ( pinState[ii] == LOW && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseInterval[ii]) {
      
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, HIGH);
        
      digitalWrite( channelPin[ii], configuration.pulseActive[ii]);
      pinState[ii] = HIGH;
      channelTimeStamp[ii] = currentTimeStamp;    
    } else if ( pinState[ii] == HIGH && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseLength[ii]){
       
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, LOW );
        
     digitalWrite( channelPin[ii], configuration.pulsePassive[ii]);
     pinState[ii] = LOW;
     channelTimeStamp[ii] = currentTimeStamp;
    }
  }
}
