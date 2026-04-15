#include <SPI.h>
#include <RH_RF95.h>

#define SS    5
#define RST   14
#define DIO0  26
#define FREQ  915.0
#define NODO  "nodo2"   // <-- cambia a "nodo2" en el segundo ESP32

RH_RF95 rf95(SS, DIO0);
int contador = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  pinMode(RST, OUTPUT);
  digitalWrite(RST, LOW);  delay(10);
  digitalWrite(RST, HIGH); delay(10);

  if (!rf95.init()) {
    Serial.println("ERROR: LoRa no encontrado");
    while (1);
  }
  if (!rf95.setFrequency(FREQ)) {
    Serial.println("ERROR: Frecuencia invalida");
    while (1);
  }

  rf95.setTxPower(13, false);
  Serial.println("Emisor listo: " NODO);
}

void loop() {
  contador++;

  char msg[64];
  snprintf(msg, sizeof(msg), "%s:Hola #%d", NODO, contador);

  Serial.print("Enviando → "); Serial.println(msg);

  rf95.send((uint8_t*)msg, strlen(msg) + 1);
  rf95.waitPacketSent();

  Serial.println("Paquete enviado");
  delay(2000);
}