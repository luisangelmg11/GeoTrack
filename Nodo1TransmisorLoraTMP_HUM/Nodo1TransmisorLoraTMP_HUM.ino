
#include <SPI.h>
#include <LoRa.h>
#include <DHT.h>


//define the pins used by the transceiver module
#define ss 5
#define rst 15
#define dio0 2

#define DHTPIN 22
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
const int trigPin = 13;
const int echoPin = 12;

int counter = 0;

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);
  dht.begin();
  while (!Serial);
  Serial.println("LoRa Sender");

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  //setup LoRa transceiver module
  LoRa.setPins(ss, rst, dio0);
  
  while (!LoRa.begin(915E6)) {
    Serial.println(".");
    delay(500);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
}

float distancia(){

  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  long duration = pulseIn(echoPin, HIGH);
  float distance = duration * 0.034 / 2; // Cálculo en cm
  return distance;
}
void loop() {

       float t = dht.readTemperature();
       float h = dht.readHumidity();
       float d = 100-distancia();
       String message = "N2: " + String(t) + " C : " + String(h) + "%"+" Nivel: "+String(d); // Formato: N1:temp:hum
       // String message = "N1:" + String(t);
        LoRa.beginPacket();
        LoRa.print(message);
        LoRa.endPacket();
        Serial.println("Enviado: " + message);
        delay(15000);
      
}