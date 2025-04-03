// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
#include <CAN.h>

// OBD-II PIDs für verschiedene Motorparameter
const byte ENGINE_RPM = 0x0C;
const byte VEHICLE_SPEED = 0x0D;
const byte ENGINE_COOLANT_TEMP = 0x05;
const byte THROTTLE_POSITION = 0x11;
const byte FUEL_LEVEL = 0x2F;

// Array mit verschiedenen PIDs
const byte pids[] = {ENGINE_RPM, VEHICLE_SPEED, ENGINE_COOLANT_TEMP, THROTTLE_POSITION, FUEL_LEVEL};
const int pidCount = sizeof(pids) / sizeof(pids[0]);

unsigned long lastSentTime = 0;
const unsigned long sendInterval = 1000; // 1 Sekunde zwischen Anfragen

// Zuletzt angefragte PID, um die Antwort zuzuordnen
byte lastRequestedPid = 0;

// Funktion zum Parsen der OBD-II-Daten
void parseOBDData(byte pid, byte data[], int length) {
  // Prüfen, ob die Antwort gültig ist (Service 0x41 = Antwort auf Service 0x01)
  if (length >= 3 && data[0] >= 0x01 && data[1] == 0x41 && data[2] == pid) {
    Serial.print("Wert für PID 0x");
    if (pid < 0x10) Serial.print("0");
    Serial.print(pid, HEX);
    Serial.print(": ");
    
    switch (pid) {
      case ENGINE_RPM: {
        // RPM = ((A * 256) + B) / 4
        if (length >= 5) {
          unsigned int rpm = ((data[3] * 256.0) + data[4]) / 4.0;
          Serial.print(rpm);
          Serial.println(" U/min");
        }
        break;
      }
      
      case VEHICLE_SPEED: {
        // Geschwindigkeit in km/h (direkter Wert)
        if (length >= 4) {
          Serial.print(data[3]);
          Serial.println(" km/h");
        }
        break;
      }
      
      case ENGINE_COOLANT_TEMP: {
        // Temperatur = A - 40
        if (length >= 4) {
          int temp = data[3] - 40;
          Serial.print(temp);
          Serial.println(" °C");
        }
        break;
      }
      
      case THROTTLE_POSITION: {
        // Position = (A * 100) / 255
        if (length >= 4) {
          float position = (data[3] * 100.0) / 255.0;
          Serial.print(position);
          Serial.println(" %");
        }
        break;
      }
      
      case FUEL_LEVEL: {
        // Füllstand = (A * 100) / 255
        if (length >= 4) {
          float level = (data[3] * 100.0) / 255.0;
          Serial.print(level);
          Serial.println(" %");
        }
        break;
      }
      
      default:
        Serial.println("Unbekannter Parameter");
        break;
    }
  } else {
    Serial.println("Ungültige oder fehlerhafte Antwort");
  }
}

void onReceive(int packetSize) {
  // Standard CAN-ID für OBD-II-Antworten von der ECU
  if (CAN.packetId() == 0x7E8 || (CAN.packetId() >= 0x7E8 && CAN.packetId() <= 0x7EF)) {
    // OBD-II-Antwort empfangen
    Serial.print("Empfangen von ECU (ID 0x");
    Serial.print(CAN.packetId(), HEX);
    Serial.print("): ");
    
    // Daten in ein Array kopieren
    byte data[8];
    int dataLength = 0;
    
    while (CAN.available() && dataLength < 8) {
      data[dataLength] = CAN.read();
      dataLength++;
    }
    
    // Daten als Hex ausgeben
    Serial.print("Data: ");
    for (int i = 0; i < dataLength; i++) {
      if (data[i] < 0x10) {
        Serial.print("0");
      }
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    // Daten parsen und interpretieren
    parseOBDData(lastRequestedPid, data, dataLength);
    
    Serial.println();
  } else {
    // Andere CAN-Nachrichten (nicht OBD-II)
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
      
      // Ausgabe als Hexadezimalwerte für bessere Lesbarkeit
      Serial.print("Data: ");
      while (CAN.available()) {
        byte b = CAN.read();
        if (b < 0x10) {
          Serial.print("0");
        }
        Serial.print(b, HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("CAN Receiver and OBD-II PID Requester");
  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  // register the receive callback
  CAN.onReceive(onReceive);
  
  Serial.println("OBD-II PID Requester started!");
}

void loop() {
  // Zeit zum Senden einer neuen Anfrage
  if (millis() - lastSentTime >= sendInterval) {
    // Wähle eine zufällige PID aus
    byte randomPid = pids[random(0, pidCount)];
    lastRequestedPid = randomPid;  // Speichern der angefragten PID
    
    // Standard OBD-II Anfrage senden
    // 7DF ist die Standard-Broadcast-Adresse für OBD-II Anfragen
    CAN.beginPacket(0x7DF);
    
    // OBD-II Anfrage:
    // Erstes Byte ist die Anzahl der zusätzlichen Datenbytes (immer 2 für einfache Anfragen)
    CAN.write(0x02);
    
    // Service-Modus 01: Aktuelle Daten
    CAN.write(0x01);
    
    // Die gewählte PID
    CAN.write(randomPid);
    
    // Auffüllen mit Nullen (gemäß OBD-II Protokoll)
    for (int i = 0; i < 5; i++) {
      CAN.write(0x00);
    }
    
    // Packet senden
    CAN.endPacket();
    
    // Status ausgeben
    Serial.print("Angefragt PID 0x");
    if (randomPid < 0x10) {
      Serial.print("0");
    }
    Serial.println(randomPid, HEX);
    
    lastSentTime = millis();
  }
  
  // Der Rest wird durch den onReceive-Callback verarbeitet
}




