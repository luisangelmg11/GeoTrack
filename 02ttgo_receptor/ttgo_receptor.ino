#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <RH_RF95.h>

// ── Pines OLED ────────────────────────────────────
#define OLED_SDA   4
#define OLED_SCL   15
#define OLED_RST   16
#define SCREEN_W   128
#define SCREEN_H   64

// ── Pines LoRa ────────────────────────────────────
#define LORA_SS    18
#define LORA_RST   14
#define LORA_DIO0  26
#define FREQ       915.0

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, OLED_RST);
RH_RF95 rf95(LORA_SS, LORA_DIO0);

// ── Nodos ─────────────────────────────────────────
#define MAX_NODOS 4

struct Nodo {
  String nombre   = "";
  String mensaje  = "";
  int    contador = 0;
  int    rssi     = 0;
};

Nodo nodos[MAX_NODOS];
int  totalNodos = 0;
int  pktsTotal  = 0;

int obtenerNodo(const String& nombre) {
  for (int i = 0; i < totalNodos; i++)
    if (nodos[i].nombre == nombre) return i;
  if (totalNodos < MAX_NODOS) {
    nodos[totalNodos].nombre = nombre;
    return totalNodos++;
  }
  return -1;
}

void actualizarPantalla() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Cabecera
  display.setCursor(0, 0);
  display.print("LoRa 915MHz");
  String pt = "p:" + String(pktsTotal);
  display.setCursor(SCREEN_W - pt.length() * 6, 0);
  display.print(pt);
  display.drawFastHLine(0, 9, SCREEN_W, SSD1306_WHITE);

  if (totalNodos == 0) {
    display.setCursor(0, 20);
    display.println("Esperando nodos...");
    display.display();
    return;
  }

  // Filas de nodos — cada una ocupa 19px
  // Layout: "nodo1 #12  -65dB"
  //         " Hola #12       "
  int y = 12;
  for (int i = 0; i < totalNodos; i++) {
    // Línea 1: nombre + contador + rssi
    display.setCursor(0, y);
    display.print(nodos[i].nombre);
    display.print(" #");
    display.print(nodos[i].contador);

    String rssiStr = String(nodos[i].rssi) + "dB";
    display.setCursor(SCREEN_W - rssiStr.length() * 6, y);
    display.print(rssiStr);

    // Línea 2: mensaje truncado
    y += 9;
    display.setCursor(2, y);
    String msg = nodos[i].mensaje;
    if (msg.length() > 20) msg = msg.substring(0, 20);
    display.print(msg);

    y += 10;
    if (i < totalNodos - 1)
      display.drawFastHLine(0, y - 1, SCREEN_W, SSD1306_WHITE);
  }

  display.display();
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TTGO LoRa32 V1.0 Receptor ===");

  // ── OLED ──
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);  delay(50);
  digitalWrite(OLED_RST, HIGH); delay(50);
  Wire.begin(OLED_SDA, OLED_SCL);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("ERROR: OLED no encontrado");
    while (true);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("TTGO LoRa32 V1.0");
  display.println("Iniciando LoRa...");
  display.display();

  // ── LoRa (RadioHead) ──
  SPI.begin(5, 19, 27, LORA_SS);  // SCK=5, MISO=19, MOSI=27

  pinMode(LORA_RST, OUTPUT);
  digitalWrite(LORA_RST, LOW);  delay(10);
  digitalWrite(LORA_RST, HIGH); delay(10);

  if (!rf95.init()) {
    display.println("LoRa FALLO!");
    display.display();
    Serial.println("ERROR: rf95.init() fallo");
    while (true);
  }
  if (!rf95.setFrequency(FREQ)) {
    display.println("Frecuencia FALLO!");
    display.display();
    Serial.println("ERROR: setFrequency fallo");
    while (true);
  }

  rf95.setTxPower(13, false);

  Serial.println("LoRa OK, esperando paquetes...");
  display.println("LoRa OK!");
  display.println("Esperando nodos...");
  display.display();
  delay(1000);

  actualizarPantalla();
}

void loop() {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      buf[len] = '\0';
      String raw = String((char*)buf);
      int rssi   = rf95.lastRssi();

      Serial.print("RX [");
      Serial.print(rssi);
      Serial.print(" dBm]: ");
      Serial.println(raw);

      // Parsear "nodo1:mensaje"
      int sep = raw.indexOf(':');
      if (sep > 0) {
        String nombre  = raw.substring(0, sep);
        String mensaje = raw.substring(sep + 1);

        int idx = obtenerNodo(nombre);
        if (idx >= 0) {
          nodos[idx].mensaje  = mensaje;
          nodos[idx].rssi     = rssi;
          nodos[idx].contador++;
        }
        pktsTotal++;
        actualizarPantalla();
      }
    }
  }
}