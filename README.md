# ECD311-POC


quick proof of concept for using an ESP32 as a replacement for an existing Raspberry Pi

using 2 ESP32 dev boards to emulate what is currently an Arduino Mega sending data to a Raspberry Pi via serial; ideally will wake from either deep sleep or light sleep via GPIO or UART, write the recieved data to a local file, and send that data to a remote server via SCP every ~minute depending on data load

currently writes a single file with fixed contents to a remote server once, then terminates

#### update 2023-04-23

this is no longer likely to be at all relevant for the project in its current state as we found a raspberry pi, but if it needs replacement at some point in the future this could act as a rough baseline to work off of

also includes a partial port of the ESP32 code to something that could run on a hosted system