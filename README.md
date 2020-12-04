# Programmable pulse generator for Arduino.
 The sketch will be build for Arduino NANO 

 The Programmable Pulse Generate will generate pulses on a number of output pins.

  Planned programmeable parameters:
 - Number of output pins (with a maximum of 10).
 - Puls length in millis for each channel.
 - Puls interval in millis for each channel.
 - Relation between pulses on the various channels. 
   : Simultaneously (All pulses starts a the same time),
   : Sequential (Pulses starts after another), with a programmable pause between the pulses. 
   : Randum (Pulses starts randumly om each channel and follow their own length and interval)
 - The pulse length on each channel can be selected as LOW pulse or HIGH pulse.

 * Version histoty:
 * 0.1.0 - Initial commit: Basic funktionalites are build in. No parameters are programmable, but the required program structure is in place.

Project eligibility. 
The sketch was build for simulating the pulses on the open collector outputs on a number of Carlo Gavazzi energy meters Type EM23 DIN and Type EM111.
The project "EnergyRegistgration", which are build to register pulses on the open collector output on the above mentined meters has a problem, counting the pulses corectly. Some pulses are rigistered but not counted and some are counted too many.
This project is to investigate into this problem.

 






  