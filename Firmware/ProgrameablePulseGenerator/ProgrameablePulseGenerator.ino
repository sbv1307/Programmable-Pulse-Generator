/*
 * Programable puls generator.
 * 
 * It will generate pulses on a number of output pins. 
 * 
 * Programmable parameters:
 * Parameters are and their defaults:
 * - Number of 
 * channels (MAX 10):           1 
 * - Pulse length in millis:                50
 * - Pulse interval in millis:              1000
 * - Pulse order interval in millis:        250
 * - Number of pulses:                      1
 * - Run continously:                       true
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
#define SKETCH_VERSION "Programable puls generator - Version 1.2.4"

/*
 *    Define DEBUGGING
 */
// #define DEBUG       //If defined ("//" is removed at the beginning of this line.) debug informations are printed to Serial.
// #define PULSE_DEBUG // If defined output state for each channel is displayed. (Gives a lot of output...) 
/*
 * Version history:
 * 1.2.4 - Bugfix: Cannot set pulseLength. [if ( value = 0 ... changed to  if ( value >= 0 ...] Value would never be zero:-)
 * 1.2.3 - Bugfix in Sequencial run and documentatino update
 * 1.2.2 - Parameter for programmelable delay after "Run Infinite" to "Run Limited"
 * 1.2.1 - Issues regarding configurations being changed, when Puls Order is changed from I to Q, S or R. handleRest() si change to reload the stored configuration, before adding changes.
 * 1.2.0 - Configuration bugfixes - Configuration defaults change to confiture alle MAX_NUMBER_OF_CHANNELS channels.
 * 1.1.2 - Minor documentation corrections - No funktional changes
 * 1.1.1 - Documentation update - No funktional changes
 * 1.1.0 - Adding push button funktionality, which will fire a configured number of pulses on each channel. 
 *       - Configuration structure is changed.
 * 1.0.0 - Prod. version.
 * 0.2.0 - Versions 0.2.x is for developing Programmable interface.
 *       - 
 * 0.1.1 - Building in funktionality for handeling different "pulse orders".
 * 0.1.0 - Initial commit: Basic funktionalites are build in. No parameters are programmable, but the required program structure is in place.
 * 
 * 
 *  **** Ardhino nano Pin definition/layout:***
 * Pin  D0: Serial RX
 * Pin  D1: Serial TX
 * Pin  D2: Channel pin 1:
 * Pin  D3: Channel pin 2: 
 * Pin  D4: Channel pin 3:
 * Pin  D5: Channel pin 4:
 * Pin  D6: Channel pin 5:
 * Pin  D7: Channel pin 6:
 * Pin  D8: Channel pin 7:
 * Pin  D10: Channel pin 9:
 * Pin  D11: Channel pin 10:
 * Pin  D12: Channel pin 11:
 * Pin  D13: LED_BUILTIN. 
 * Pin  D14: (A0): RANDUM_SEED_PIN    Used for randumSeed() to generate different seed numbers each time the sketch runs.
 * Pin  D15: (A1): N/C
 * Pin  D16: (A2): N/C
 * Pin  D17: (A3): N/C
 * Pin  D18: (A4): N/C
 * Pin  D19: (A5): Button pin. Button to running in non continoougious mode (file configured numer of pulses)
 */
#include <EEPROM.h>
#include "EEPROMAnything.h"


 /*
 * ######################################################################################################################################
 *                       C  O  N  F  I  G  U  T  A  B  L  E       D  E  F  I  N  I  T  I  O  N  S
 * ######################################################################################################################################
*/
#define CONFIGURATION_VERSION 4
#define MAX_NUMBER_OF_CHANNELS 10
#define RANDUM_SEED_PIN 0
#define BUTTON_PIN 19
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
int numberOfPulses[MAX_NUMBER_OF_CHANNELS];

uint8_t pinState[MAX_NUMBER_OF_CHANNELS];           // A flag for storing the last state (Active / Passive) for the actual pin

// Variables for controlling button input
int lastButtonState = HIGH;   // the previous reading from the input pin
boolean buttonPressedOnce = false;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 250;    // the debounce time; increase if the output flickers


struct config_t                                     // Configuration are stored in a structure, and read from EEPROM. If no available, default parameters are written.
  {
    int structVersion;                              //A way to determind the correct confiugration structure.
    int numberOfChannels;
    int numberOfPulses[MAX_NUMBER_OF_CHANNELS];               //Number of pulses fired, when push button is pushed twice.
    boolean runContinous;                                     //When bottun is pushed twice, this will tuggle between true and false.
    unsigned long runlimitedDelay;                            // the number of miliconds before the pulesgenerator starts, after configuration has change to "Run Limited"        
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
void(* resetFunc) (void) = 0;//declare reset function at address 0

void setConfigurationDefaults() {
                                                              #ifdef DEBUG
                                                                Serial.println(P("Setting configuration defaultes!"));
                                                              #endif
  
  configuration.structVersion = CONFIGURATION_VERSION * 100 + MAX_NUMBER_OF_CHANNELS;
  configuration.numberOfChannels = MAX_NUMBER_OF_CHANNELS;
  configuration.runContinous = true;                            //Default run continously. if false only the configured number of pulses will be fired
  configuration.runlimitedDelay = 5000;                         // Default: wait five secunds before restarting after configuration has changed to "Run Limited"
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    configuration.pulseLength[ii] = 50;
    configuration.pulseInterval[ii] = 950;
    configuration.numberOfPulses[ii] = 10;
    configuration.pulseActive[ii] = LOW;                                  // This define if the pulse is active LOW or active HIGH
    if ( configuration.pulseActive[ii] == LOW )
      configuration.pulsePassive[ii] = HIGH;                                // Needs to be the oppersit ove active!
    else
      configuration.pulsePassive[ii] = LOW;
  }
  configuration.pulseOrder = 'I';
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
  Serial.print(P("\n\nStructure version: "));
  Serial.println(configuration.structVersion);
  Serial.print(P("Number of channels: "));
  Serial.println(configuration.numberOfChannels);
  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    Serial.print(P("Channel "));
    Serial.print(ii +1);
    Serial.print(P(" Pulse length: "));
    Serial.print(configuration.pulseLength[ii]);
    Serial.print(P(" Pulse period: "));
    Serial.print(configuration.pulseInterval[ii] + configuration.pulseLength[ii]);
    Serial.print(P(" Pulse active: "));
    if ( configuration.pulseActive[ii] == HIGH)
      Serial.print(P("HIGH"));
    else
      Serial.print(P("LOW"));
    Serial.print(P(". Number of pulses generated if Limited run: "));
    Serial.println( configuration.numberOfPulses[ii]);
  }
  if ( configuration.runContinous)
    Serial.println(P("Run Infinite. PRESS pushbutton twice to change!"));
  else
    Serial.println(P("Run Limited. PRESS pushbutton once to change!"));
  Serial.print(P("Delay restart when configuration changed to Run Limited for (miliconds): "));
  Serial.println(configuration.runlimitedDelay);  
  Serial.print(P("Pulse order: "));
  Serial.println(configuration.pulseOrder);
  Serial.print(P("Pulse order interval: "));
  Serial.println(configuration.pulseOrderInterval);
  Serial.print(P("Connect Pushbutton to pin#: "));
  Serial.println(BUTTON_PIN);
  Serial.println(P("\n\nHit <Enter> to get help!"));
}

/*
 * Handle REST function.
 * Expected character string:
 * /configurations/{parameret}/{id}/{value}, /configurations/{parameret}/{value} or configurations/{parameret}/ 
 * 
 *                <numberOfChannels>  /<1 - 10> 
 *                <pulseOrder>        /<I, S, Q or R>
 *                <runlimitedDelay>   /<millisecunds>
 *                <pulseOrderInterval>/<millisecunds>
 *                <pulseLength>       /<channel 1-10>/<millisecunds>
 *                <pulsePeriod>       /<channel 1-10>/<millisecunds>
 *                <pulseActive>       /<channel 1-10>/<LOW or HIGH>
 *                <setDefaults>       /
 *
 * Configuration examaples:
 * configurations/numberOfChannels/3
 * configurations/pulseOrder/S
 * configurations/runlimitedDelay/5000
 * configurations/pulseOrderInterval/250
 * configurations/pulseLength/1/100
 * configurations/pulsePeriod/1/500
 * configurations/pulseActive/1/HIGH
 * configurations/setDefaults/
 * 
 *
 */

void handleRest() {
  int charsRead = Serial.readBytesUntil( '/', p_buffer, 63);
  p_buffer[charsRead] = '\0';
  boolean saveChanges = true;

  
  if ( strcmp( "configurations", p_buffer) == 0) {
    //>>>>>>>>>>>>>>>>>>     Read stored configuraton, before adding changes   <<<<<<<<<<<<<<<<<
    charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
      
    //>>>>>>>>>>>>>>>>>>     Setting configurations   <<<<<<<<<<<<<<<<<
    int charsRead = Serial.readBytesUntil( '/', p_buffer, 63);
    p_buffer[charsRead] = '\0';
    if ( strcmp( "numberOfChannels", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting number of channels <<<<<<<<<<<<<<
      int numberOfChannels = Serial.parseInt();
      if ( numberOfChannels > 0 && numberOfChannels <= MAX_NUMBER_OF_CHANNELS)
        configuration.numberOfChannels = numberOfChannels;
    } else if ( strcmp( "pulseOrder", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting pulse order  <<<<<<<<<<<<<<
      char c = Serial.read();                                // Read expected '/' character
      int charsRead = Serial.readBytesUntil( '\n', p_buffer, 63);
      c = p_buffer[0];
      if ( c == 'I' || c == 'S' || c == 'Q' || c == 'R' )
        configuration.pulseOrder = c;
    } else if ( strcmp( "runlimitedDelay", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting run limited Delay <<<<<<<<<<<<<<
      int runlimitedDelay = Serial.parseInt();
      if ( runlimitedDelay >= 1000 && runlimitedDelay <= 60000)
        configuration.runlimitedDelay = runlimitedDelay;
    } else if ( strcmp( "pulseOrderInterval", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting pulse order interval <<<<<<<<<<<<<<
      int pulseOrderInterval = Serial.parseInt();
      if ( pulseOrderInterval > 0 && pulseOrderInterval <= 60000)
        configuration.pulseOrderInterval = pulseOrderInterval;
    } else if ( strcmp( "pulseLength", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting pulse length <<<<<<<<<<<<<<
      int channel = Serial.parseInt();

      if ( channel > 0 && channel <= configuration.numberOfChannels) {
        int value = Serial.parseInt();

        if ( value >= 0 && value <= 60000 )
          configuration.pulseLength[channel - 1] = value;        
      }

    } else if ( strcmp( "pulsePeriod", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting pulse period <<<<<<<<<<<<<<
      int channel = Serial.parseInt();
      if ( channel > 0 && channel <= configuration.numberOfChannels) {
        int value = Serial.parseInt();
        if ( value > configuration.pulseLength[channel - 1] && value <= 60000 )
          configuration.pulseInterval[channel - 1] = value - configuration.pulseLength[channel - 1];        
      }
    } else if ( strcmp( "numberOfPulses", p_buffer) == 0) {

      //>>>>>>>>>>>>>>>>     Setting Number of pulses fiered  <<<<<<<<<<<<<<
      int channel = Serial.parseInt();
      if ( channel > 0 && channel <= configuration.numberOfChannels) {
        int value = Serial.parseInt();
        if ( value > 1 && value <= 100000 )
          configuration.numberOfPulses[channel - 1] = value;       
      }
    } else if ( strcmp( "pulseActive", p_buffer) == 0) {
      
      //>>>>>>>>>>>>>>>>     Setting pulse acgive HIGH or LOW  <<<<<<<<<<<<<<
      int channel = Serial.parseInt();
      if ( channel > 0 && channel <= configuration.numberOfChannels) {
        char c = Serial.read();                                // Read expected '/' character
        int charsRead = Serial.readBytesUntil( '\n', p_buffer, 63);
        p_buffer[charsRead] = '\0';
        if ( strcmp( "HIGH", p_buffer) == 0) {
          configuration.pulseActive[channel - 1] = HIGH;
          configuration.pulsePassive[channel - 1] = LOW;                               // Needs to be the oppersit ove active!
        } else if ( strcmp( "LOW", p_buffer) == 0) {
          configuration.pulseActive[channel - 1] = LOW;
          configuration.pulsePassive[channel - 1] = HIGH;                               // Needs to be the oppersit ove active!
        }
      }
    } else if ( strcmp( "setDefaults", p_buffer) == 0) {

      //>>>>>>>>>>>>>>>>     Setting configurations to defautls values  <<<<<<<<<<<<<<
      setConfigurationDefaults(); 
      saveChanges = false;
    } else {
      Serial.print(P("\n\n>>>>>>>>>>>  Unknown parameter : "));
      Serial.println(p_buffer);
      saveChanges = false;
    }

    int charsWritten = 0;
    if (saveChanges)
      charsWritten = EEPROM_writeAnything( CONFIG_ADDRESS, configuration);
                                                              #ifdef DEBUG
                                                                if (saveChanges) {
                                                                  if ( charsWritten == sizeof( configuration))
                                                                    Serial.println(P("Configuration sucessfully written to EEMPROM"));
                                                                  else
                                                                    Serial.println(P("F A I L E D   Writing configuration to EEPROM"));
                                                                }
                                                              #endif

    delay(1000);
    resetFunc();                                  // Call reset, to start pulse generator with new configuration.
  } else {
    Serial.print("Unknown command - expected <configurations>: ");
    Serial.println(p_buffer);
    //                ----------------------------------------------------------------------------------------------------------------------------------------------------- (149 characters
    Serial.println(P("\n\nConfigurable parameters for keyword <configurations>:\n\n<numberOfChannels>  /<1 - 10>"));
    Serial.println(P("<pulseOrder>        /<I (Individual), S (Simultaneously), Q (Sequential) or R (Randum)>"));
    Serial.println(P("<runlimitedDelay>   /<millisecunds>\n<pulseOrderInterval>/<millisecunds>\n<pulseLength>       /<channel 1-10>/<millisecunds>"));
    Serial.println(P("<pulsePeriod>       /<channel 1-10>/<millisecunds>\n<pulseActive>       /<channel 1-10>/<LOW or HIGH>"));
    Serial.println(P("<numberOfPulses>    /<channel 1-10>/<1 - 100000>\n<setDefaults>       /\n\n"));
    Serial.println(P("Eksamples to copy-paste into the serial monitor:")); 
    Serial.println(P("configurations/numberOfChannels/3             Sets number of channels to 3"));
    Serial.println(P("configurations/pulseOrder/S                   Sets puls order to Simultaneously"));
    Serial.println(P("configurations/runlimitedDelay/5000           Sets delay for restart after change for configuration to Run Limited to 5000 ms."));
    Serial.println(P("configurations/pulseOrderInterval/250         Sets puls interval to 250 ms."));
    Serial.println(P("configurations/pulseLength/2/50               Sets the pulse lenngth for channel 2 to 50 ms."));
    Serial.println(P("configurations/pulsePeriod/1/500              Sets pulse period to 500 ms."));
    Serial.println(P("configurations/pulseActive/1/HIGH             Sets puls as a HIGH pulse"));
    Serial.println(P("configurations/numberOfPulses/1/1000          Sets the number of pulses to be fired, after second push on the push button."));
    Serial.println(P("configurations/setDefaults/                   Sets all configurable parameters to defaults.\n\n"));
  }  
  while (Serial.available()) {
    char c = Serial.read();
  }    
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
  digitalWrite( LED_BUILTIN, HIGH);                  // Initialization started
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
                                                                while (Serial.available()) {
                                                                  char c = Serial.read();  // Empty input buffer.
                                                                }
                                                                
                                                              #endif
  pinMode( BUTTON_PIN, INPUT_PULLUP);
/*
 * Read configuration. If not allready set or incorrect version, set defaults.
 */
  int charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
  if (configuration.structVersion != CONFIGURATION_VERSION * 100 + MAX_NUMBER_OF_CHANNELS)
    setConfigurationDefaults();

  charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
  if (configuration.structVersion != CONFIGURATION_VERSION * 100 + MAX_NUMBER_OF_CHANNELS) {
                                                              #ifdef DEBUG
                                                                Serial.println(P("FAILED to read configuration!!!"));
                                                                Serial.println(P("Using defaults!"));
                                                              #endif
  setConfigurationDefaults();
  }

  Serial.println(P("\n\nStored Configuration:"));
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
         channelOffsetStartTime[ii] = channelOffsetStartTime[ii - 1] + configuration.pulseLength[ii - 1] + configuration.pulseOrderInterval;  //To start the pulses in sequentiel order, calculate offset
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

  if ( configuration.pulseOrder == 'Q' || configuration.pulseOrder == 'S' || configuration.pulseOrder == 'R' ) {
    Serial.println(P("\n\nPuls order configured as Sequentiel (Q), Simultaneously (S) or Randum (R)"));
    Serial.println(P("Re-calculated configuration used:"));
    printConfiguration();
  }

/*
 * If programmed for limited run transfer number of pulses per channnel, other wise set to 1;
 */
for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
  if ( !configuration.runContinous) 
    numberOfPulses[ii] = configuration.numberOfPulses[ii];
  else
    numberOfPulses[ii] = 1; 
}
 
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

  digitalWrite( LED_BUILTIN, LOW);

  Serial.print(P("Initialized - Starting in "));
  Serial.print(configuration.pulseInterval[0]);
  Serial.println(P(" Miliseconds..."));

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

  /*
   * <<<<<<<<<<<<<<<<<<<<<<<<<<<<  Pulse generation section >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><
   */
  unsigned long currentTimeStamp = millis();

  for ( int ii = 0; ii < configuration.numberOfChannels; ii++) {
    if ( pinState[ii] == PASSIVE && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseInterval[ii] + channelOffsetStartTime[ii] && numberOfPulses[ii] > 0) {
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, HIGH);

                                                              #ifdef PULSE_DEBUG
                                                                Serial.print("Channel [");
                                                                Serial.print(ii);
                                                                Serial.println("] Active");
                                                              #endif
       
      digitalWrite( channelPin[ii], configuration.pulseActive[ii]);
      pinState[ii] = ACTIVE;
      channelTimeStamp[ii] = currentTimeStamp; 
      channelOffsetStartTime[ii] = 0;
      if ( !configuration.runContinous) {
        numberOfPulses[ii]--;
        if ( numberOfPulses[ii] == 0) {
          Serial.print(configuration.numberOfPulses[ii]);
          Serial.print(P(" pulses fired on channel ["));
          Serial.print(ii + 1);
          Serial.println(P("]. Press push button once to continue or twice to restart!"));
        }
      } 
        
      
   } else if ( pinState[ii] == ACTIVE && currentTimeStamp - channelTimeStamp[ii] > configuration.pulseLength[ii]){
       
      if ( ii == 0)
        digitalWrite( LED_BUILTIN, LOW );
        
                                                              #ifdef PULSE_DEBUG
                                                                Serial.print("Channel [");
                                                                Serial.print(ii);
                                                                Serial.println("] Passive");
                                                              #endif

    digitalWrite( channelPin[ii], configuration.pulsePassive[ii]);
      pinState[ii] = PASSIVE;
      channelTimeStamp[ii] = currentTimeStamp;
    }
  }

  /*
   * <<<<<<<<<<<<<<<<<<<<<<<<<<<<  Push button section >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>><
   */

  // read the state of the switch into a local variable:

  int currentButtonState = digitalRead( BUTTON_PIN);
  
  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (currentButtonState == LOW and currentButtonState != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
    lastButtonState = currentButtonState;
  } else if ( lastButtonState != currentButtonState) {
    if ((millis() - lastDebounceTime) > debounceDelay) {
                                                              #ifdef DEBUG
                                                                Serial.println(P("Button pressed"));
                                                              #endif
      if ( buttonPressedOnce) {
                                                              #ifdef DEBUG
                                                                Serial.println(P("Button pressed twice"));
                                                              #endif
      buttonPressedOnce = false;
      int charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
      configuration.runContinous = false;
      int charsWritten = EEPROM_writeAnything( CONFIG_ADDRESS, configuration);
                                                              #ifdef DEBUG
                                                                if ( charsWritten == sizeof( configuration))
                                                                  Serial.println(P("Intermittent run written to configuration.\n\nRestarting...\n\n"));
                                                                else
                                                                  Serial.println(P("F A I L E D   Writing configuration to EEPROM"));
                                                              #endif
      Serial.print(P("\nRestarting in "));
      Serial.print(configuration.runlimitedDelay);
      Serial.println(P(" mililconds..."));
      delay(configuration.runlimitedDelay);
      resetFunc();                                  // Call reset, to start pulse generator with new configuration.

      } else {
        int charsRead = EEPROM_readAnything( CONFIG_ADDRESS, configuration);
        if (configuration.runContinous == false) {
          configuration.runContinous = true;
          int charsWritten = EEPROM_writeAnything( CONFIG_ADDRESS, configuration);
                                                              #ifdef DEBUG
                                                                if ( charsWritten == sizeof( configuration))
                                                                  Serial.println(P("Continous run written to configuration.\n\nRestarting...\n\n"));
                                                                else
                                                                  Serial.println(P("F A I L E D   Writing configuration to EEPROM"));
                                                              #endif
      
          delay(1000);
          resetFunc();                                  // Call reset, to start pulse generator with new configuration.
        }
        buttonPressedOnce = true;
        lastButtonState = currentButtonState;
      }
      for ( int ii; ii < configuration.numberOfChannels; ii++)
        numberOfPulses[ii] = 0;
    }
  }

  /*
   * <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  Programming section  >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
   */
  if ( Serial.available())
    handleRest();
}
