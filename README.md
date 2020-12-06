# Programmable pulse generator for Arduino.
 The sketch will be build for Arduino NANO 

 The Programmable Pulse Generate will generate pulses on a number of output pins.

  Planned programmeable parameters:
 - Number of output pins (with a maximum of 10).
 - Puls length in millis for each channel.
 - Puls interval in millis for each channel.
 - Relation between pulses on the various channels. 
   - Individual     'I' Pulses starts Simultaneously, but act upon their own pulse length and interval. (Default)
   - Simultaneously 'S' All pulses starts a the same time,
   - Sequential     'Q' Pulses starts after another), with a programmable pause between the pulses. 
   - Randum         'R' Pulses starts randumly om each channel and follow their own length and interval.
 - The pulse length on each channel can be selected as LOW pulse or HIGH pulse.

 ### Version histoty:
 * 1.0.0 - Prod. version.
 * 0.2.0 - Versions 0.2.x is for developing Programmable interface.
         - Taking the REST approch for programming parameters. 
 * 0.1.1 - Building in funktionality for handeling different "pulse orders".
         - pinState re-defined to match ACTIVE / PASSIVE pulse instead of LOW / HIGH.
 * 0.1.0 - Initial commit: Basic funktionalites are build in. No parameters are programmable, but the required program structure is in place.

#### REST Approch
 * Communication will be over serial connection (USB).
 * When not connected, the sketch will start, using the last programmed configuration.

/configurations/{parameret}/{id}/{value} or /configurations/{parameret}/{value} 

                <numberOfChannels>  /<1 - 10> 
                <pulseOrder>        /<I, S, Q or R>
                <pulseOrderInterval>/<millisecunds>
                <pulseLength>       /<channel 1-10>/<millisecunds>
                <pulsePeriod>       /<channel 1-10>/<millisecunds>
                <pulseActive>       /<channel 1-10>/<LOW or HIGH>

 ##### Configuration examaples:
 * configurations/numberOfChannels/3
 * configurations/pulseOrder/S
 * configurations/pulseOrderInterval/250
 * configurations/pulseLength/1/100
 * configurations/pulsePeriod/1/500
 * configurations/pulseActive/1/HIGH



Project eligibility. 
The sketch was build for simulating the pulses on the open collector outputs on a number of Carlo Gavazzi energy meters Type EM23 DIN and Type EM111.
The project "EnergyRegistgration", which are build to register pulses on the open collector output on the above mentined meters has a problem, counting the pulses corectly. Some pulses are rigistered but not counted and some are counted too many.
This project is to investigate into this problem.

 






  