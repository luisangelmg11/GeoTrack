#include <SPI.h>
#include <RH_RF95.h>

#define SS    5
#define RST   14
#define DIO0  26

#define FREQ  915.0

RH_RF95 rf95(SS, DIO0);
int count=0;
void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(RST, OUTPUT);
  // Reset manual del módulo
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

  rf95.setTxPower(13, false); // 5–23 dBm
  Serial.println("Emisor LoRa listo");
}

void loop() {
  Serial.println("Enviando paquete...");
 
  char payload[] = "Hola bellaka!";
  //String payload = "Hola Bellaka ";
 // payload = payload + count;
  //count+=1;
  rf95.send((uint8_t*)payload, sizeof(payload));
  rf95.waitPacketSent();

  Serial.println("Paquete enviado");
  delay(2000);
}