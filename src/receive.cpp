// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <CAN.h>
void onReceive(int packetSize) {
  // received a packet
  Serial.print("Received ");
  if (CAN.packetExtended()) {
    Serial.print("extended ");
  }
  if (CAN.packetRtr()) {
    // Remote transmission request, packet contains no data
    Serial.print("RTR ");
  }
  Serial.print("packet with id 0x");
  Serial.print(CAN.packetId(), HEX);
  if (CAN.packetRtr()) {
    Serial.print(" and requested length ");
    Serial.println(CAN.packetDlc());
  } else {
    Serial.print(" and length ");
    Serial.println(packetSize);
    // only print packet data for non-RTR packets
    while (CAN.available()) {
      Serial.print((char)CAN.read());
    }
    Serial.println();
  }
  Serial.println();
}
void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("CAN Receiver Callback");
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  // register the receive callback
  CAN.onReceive(onReceive);
}
void loop() {
  // do nothing
}




