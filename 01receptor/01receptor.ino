#include <SPI.h>
#include <RH_RF95.h>

#define SS    5
#define RST   14
#define DIO0  26

#define FREQ  915.0

RH_RF95 rf95(SS, DIO0);

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(RST, OUTPUT);
  digitalWrite(RST, LOW);
  delay(10);
  digitalWrite(RST, HIGH);
  delay(10);

  if (!rf95.init()) {
    Serial.println("ERROR: No se encontró el módulo LoRa");
    while (1);
  }

  if (!rf95.setFrequency(FREQ)) {
    Serial.println("ERROR: Frecuencia inválida");
    while (1);
  }

  rf95.setTxPower(13, false);
  Serial.println("Receptor LoRa listo, esperando...");
}

void loop() {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      buf[len] = '\0'; // terminar string
      Serial.print("Recibido: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.print(rf95.lastRssi());
      Serial.println(" dBm");
    } else {
      Serial.println("Error al recibir paquete");
    }
  }
}