/*
 * Programable puls generator.
 * 
 * It will generate pulses on a number of output pins. 
 * 
 * Programmable parameters:
 * Only the structure for programmable parameters are in place. The parameters hardcoded as "default parameters" in function setConfigurationDefaults().
 * Parameters are and their defaults:
 * - Number of channels (MAX 10):           1 
 * - Pulse length in millis:                50
 * - Pulse interval in millis:              1000
 * - Pulse order interval in millis:        250
 * - Pulse (the short part / Active part):  LOW
 * - Pulse order:                           Individual (I) 
 *  
 * - Pulse order - Relation between pulses on the various channels. 
 *   : Individual 'I' Pulses act upon their own pulse length and interval (Default).
 *   : Sequential 'Q' Pulses starts after another), with a programmable pause (Pulse order interval) between the pulses. 
 *   : Simultaneously 'S' All pulses starts a the same time.
 *   : Randum 'R' Pulses starts randumly om each channel and follow their own length and interval.
 * 
 */
#define SKETCH_VERSION "Programable puls generator - Version 0.1.1"

#define DEBUG  //If defined ("//" is removed at the beginning of this line.) debug informations are printed to Serial.
/*
 * Version histoty:
 * 0.1.1 - Building in funktionality for handeling different "pulse orders".
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
 * Pin 14 (A0): RANDUM_SEED_PIN    Used for randumSeed() to generate different seed numbers each time the sketch runs.
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
#define RANDUM_SEED_PIN 0
#define CONFIG_ADDRESS 0

#define ACTIVE  0X1                               // Could ofcause use HIGH / LOW, but it makes more sense to define ACTIVE and PASSIVE as
#define PASSIVE 0x0                               // ACTIVE can be a LOW pulse or a HIGH pulse. 

/*
 *  #####################################################################################################################
 *                       V  A  R  I  A  B  L  E      D  E  F  I  N  A  I  T  O  N  S
 *  #####################################################################################################################
 */
/* Incapsulate strings i a P(string) macro definition to handle strings in PROGram MEMory (PROGMEM) to reduce valuable memory  
 * MACRO for string handling from PROGMEM
 * https://todbot.com/blog/2008/06/19/how-to-do-big-strings-in-arduino/
 * max 149 chars at once ...
 * ----------------------------------------------------------------------------------------------------------------------------------------------------- (149 characters)
 */
 char p_buffer[150];
#define P(str) (strcpy_P(p_buffer, PSTR(str)), p_buffer)

const int channelPin[MAX_NUMBER_OF_CHANNELS] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

unsigned long channelTimeStamp[MAX_NUMBER_OF_CHANNELS];
unsigned long channelOffsetStartTime[MAX_NUMBER_OF_CHANNELS];

uint8_t pinState[MAX_NUMBER_OF_CHANNELS];           // A flag for storing the last state (Active / Passive) for the actual pin

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
  configuration.numberOfChannels = 2;
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    configuration.pulseLength[ii] = 50;
    configuration.pulseInterval[ii] = 1000;
    configuration.pulseActive[ii] = LOW;                                  // This define if the pulse is active LOW or active HIGH
    if ( configuration.pulseActive[ii] == LOW )
      configuration.pulsePassive[ii] = HIGH;                                // Needs to be the oppersit ove active!
    else
      configuration.pulsePassive[ii] = LOW;
  }
  configuration.pulseOrder = 'R';
  configuration.pulseOrderInterval = 250;
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
  unsigned long maxPulseLength = 0;
  unsigned long maxPulseInterval = 0;
  unsigned long maxPulsePeriod = 0;

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
  * Initialize output pins, ChannelTimeStamp
  *  
  * If relation between pulses on the various channels are configured as Sequential Q). pulseInterval are redefined
  * to be the sum of pulselength plus pulseOrderIngerval for each cahnnel. Plus pulseInterval for the first channel (channel[0])
  * Here the sum of all pulse length's plus pulsOrderInterval is calculated.
  * 
  */
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    pinMode( channelPin[ii], OUTPUT);
    digitalWrite( channelPin[ii], configuration.pulsePassive[ii]);
    channelTimeStamp[ii] = 0;
    pinState[ii] = PASSIVE;
    channelOffsetStartTime[ii] = 0;
    if ( configuration.pulseOrder == 'Q' ) {
       maxPulsePeriod = maxPulsePeriod + configuration.pulseLength[ii] + configuration.pulseOrderInterval;   //To ensure that all pulses on all channels can be within the same period
       if ( ii > 0 )
         channelOffsetStartTime[ii] = configuration.pulseLength[ii - 1] + configuration.pulseOrderInterval;  //To start the pulses in sequentiel order, calculate offset
    } else if ( configuration.pulseOrder == 'S' || configuration.pulseOrder == 'R' ) {
      if ( configuration.pulseLength[ii] > maxPulseLength)
        maxPulseLength = configuration.pulseLength[ii];
      if ( configuration.pulseInterval[ii] > maxPulseInterval)
        maxPulseInterval = configuration.pulseInterval[ii];
    }
  }
  
/*
 * If relation between pulses on the various channels are configured as Sequential Q).
 * Pulse interval for each puls are redefined to match, so that alle channels has the same.
 */
  if ( configuration.pulseOrder == 'Q') {
     maxPulsePeriod = maxPulsePeriod + configuration.pulseInterval[0];      // Adding pulseInterval for the first channel (channel[0])
     for ( int ii = 0; ii < configuration.numberOfChannels; ii++)
       configuration.pulseInterval[ii] = maxPulsePeriod - configuration.pulseLength[ii];   
  }

 /*
  * If relation between pulses on the various channels are configured as Simultaneously (S). pulseInterval are redefined
  * to the largest pulse period. This will make all pins active at the same time, but keeping the 
  * individual pulse length.
  * 
  * To ensure that "currentTimeStamp - channelTimeStamp[ii] > configuration.pulseInterval[ii]" for all channels when entering loop(), delay the startup with maxPulsePeriod.
  */ 
  if ( configuration.pulseOrder == 'S') {
    maxPulsePeriod = maxPulseLength + maxPulseInterval;
    for ( int ii = 0; ii < configuration.numberOfChannels; ii++)
      configuration.pulseInterval[ii] = maxPulsePeriod - configuration.pulseLength[ii];   
    delay(maxPulsePeriod);
  }

  /*
   * If relation between pulses on the various channels are configured as Randum (R), a randum value is configured for channelOffsetStartTime
   */
   if (configuration.pulseOrder == 'R') {
     maxPulsePeriod = maxPulseLength + maxPulseInterval;
     randomSeed(analogRead(RANDUM_SEED_PIN));
     for ( int ii = 0; ii < configuration.numberOfChannels; ii++)
       channelOffsetStartTime[ii] = random( maxPulsePeriod);
   }
                                                              #ifdef DEBUG
                                                                Serial.println(P("Initialized - Starting..."));
                                                              #endif

  digitalWrite( LED_BUILTIN, LOW);

/*
 * If relation between pulses on the various channels are configured as Sequential (Q) or Randum (R):
 * To ensure the first activation of pulses are activated accoring to time stamps, channelTimeStamp are set as close 
 * to currentTimeStamp as possible. 
 * It's assumed that the pulseInterval for the first pulse i longer, than it take to enter the first "if" statement
 * in loop().
 */
  if ( configuration.pulseOrder == 'Q' || configuration.pulseOrder == 'R') {
    
    unsigned long currentTimeStamp = millis();
    for ( int ii = 0; ii < configuration.numberOfChannels; ii++) 
      channelTimeStamp[ii] = currentTimeStamp;
  }
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
    if ( pinState[ii] == PASSIVE && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseInterval[ii] + channelOffsetStartTime[ii]) {
      
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, HIGH);
        
      digitalWrite( channelPin[ii], configuration.pulseActive[ii]);
      pinState[ii] = ACTIVE;
      channelTimeStamp[ii] = currentTimeStamp; 
      channelOffsetStartTime[ii] = 0;   
    } else if ( pinState[ii] == ACTIVE && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseLength[ii]){
       
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, LOW );
        
      digitalWrite( channelPin[ii], configuration.pulsePassive[ii]);
      pinState[ii] = PASSIVE;
      channelTimeStamp[ii] = currentTimeStamp;
    }
  }
}
