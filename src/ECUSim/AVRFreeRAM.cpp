#include <Arduino.h>
#include "AVRFreeRAM.h"

/*
  See:
  https://garretlab.web.fc2.com/arduino.cc/docs/learn/programming/memory-guide/
*/

void display_freeram() {
  Serial.print(F("- SRAM left: "));
  Serial.println(freeRam());
}


int freeRam() {
  extern int __heap_start,*__brkval;
  int v;
  return (int)&v - (__brkval == 0  
    ? (int)&__heap_start : (int) __brkval);  
}