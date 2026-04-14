#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

// ── OLED ──────────────────────────────────────────
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT  64
#define OLED_RESET     -1
#define OLED_ADDRESS  0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ── Beitian BE-182 ────────────────────────────────
#define GPS_RX_PIN 16      // ESP32 RX2 ← BE-182 TX
#define GPS_TX_PIN 17      // ESP32 TX2 → BE-182 RX
#define GPS_BAUD   115200  // ← CLAVE: BE-182 default es 115200

TinyGPSPlus gps;
HardwareSerial gpsSerial(2);

unsigned long lastUpdate = 0;

// ─────────────────────────────────────────────────
void oledMsg(String l1, String l2="", String l3="", String l4="") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(l1);
  if (l2 != "") display.println(l2);
  if (l3 != "") display.println(l3);
  if (l4 != "") display.println(l4);
  display.display();
}

void mostrarDiagnostico() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("BE-182 @ 115200");
  display.print("Chars: ");  display.println(gps.charsProcessed());
  display.print("OK: ");     display.print(gps.sentencesWithFix());
  display.print(" Err: ");   display.println(gps.failedChecksum());
  display.print("Sat: ");    display.println(gps.satellites.isValid() ? gps.satellites.value() : 0);

  // LED rojo = sin fix, LED rojo parpadeando = fix
  display.println("LED rojo fijo=sin fix");
  display.println("LED rojo parpadea=FIX");

  display.display();
}

void mostrarFix() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  display.println("=== FIX OK! ===");
  display.print("Lat: "); display.println(gps.location.lat(), 6);
  display.print("Lon: "); display.println(gps.location.lng(), 6);

  if (gps.altitude.isValid()) {
    display.print("Alt: ");
    display.print(gps.altitude.meters(), 1);
    display.println("m");
  }

  display.print("Sat:");
  display.print(gps.satellites.value());
  display.print(" HDOP:");
  display.println(gps.hdop.isValid() ? gps.hdop.value() / 100.0 : 0, 1);

  if (gps.time.isValid()) {
    char buf[20];
    snprintf(buf, sizeof(buf), "UTC %02d:%02d:%02d",
             gps.time.hour(), gps.time.minute(), gps.time.second());
    display.println(buf);
  }

  display.display();
}

// ─────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== Beitian BE-182 Test ===");
  Serial.println("Baud: 115200, Update: 10Hz");

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
      Serial.println("ERROR: OLED no encontrado");
      while (true);
    }
  }

  oledMsg("BE-182 GNSS", "Baud: 115200", "10Hz update", "Iniciando...");
  delay(1000);

  gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("GPS serial listo");

  oledMsg("GPS serial OK", "Esperando NMEA...");
  delay(500);
}

// ─────────────────────────────────────────────────
void loop() {
  while (gpsSerial.available()) {
    char c = gpsSerial.read();
    gps.encode(c);
    Serial.write(c);
  }

  if (millis() - lastUpdate >= 500) {  // 500ms porque update rate es 10Hz
    lastUpdate = millis();

    if (gps.location.isValid()) {
      mostrarFix();
    } else {
      mostrarDiagnostico();
    }

    Serial.printf("[STATUS] Chars:%lu OK:%lu Err:%lu Sat:%d Fix:%s\n",
      gps.charsProcessed(),
      gps.sentencesWithFix(),
      gps.failedChecksum(),
      gps.satellites.isValid() ? gps.satellites.value() : 0,
      gps.location.isValid() ? "SI" : "NO"
    );
  }
}