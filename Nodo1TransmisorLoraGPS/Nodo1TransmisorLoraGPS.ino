
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h> // Usar HardwareSerial para ESP32

//define the pins used by the transceiver module
#define ss 5
#define rst 15
#define dio0 2

#define DHTPIN 22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
TinyGPSPlus gps;
HardwareSerial neogps(2); // Usar Serial 2 del ESP32

int counter = 0;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  dht.begin();
  neogps.begin(9600, SERIAL_8N1, 16, 17); // Baud rate del GPS y pines RX/TX
  while (!Serial);
  Serial.println("LoRa Sender");

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  //replace the LoRa.begin(---E-) argument with your location's frequency 
  //433E6 for Asia
  //868E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
   // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}
void loop() {
 while (neogps.available() > 0) {
   if (gps.encode(neogps.read())) {
     if (gps.location.isValid()) {
        float t = dht.readTemperature();
       String gpsData = String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
        String message = "N1:" + String(t) + ":" + gpsData; // Formato: N1:temp:lat,lng
       // String message = "N1:" + String(t);
        LoRa.beginPacket();
        LoRa.print(message);
        LoRa.endPacket();
        Serial.println("Enviado: " + message);
        delay(15000);
      }
    }
  }
}