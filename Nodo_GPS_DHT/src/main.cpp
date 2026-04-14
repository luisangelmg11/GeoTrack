// ============================================================
//  GeoTrack-Agro — NODO TRANSMISOR
//  MCU    : ESP32
//  Radio  : RFM69HCW  (SPI)
//  GPS    : NEO-6M    (UART2)
//  Sensor : DHT22     (GPIO digital)
//
//  Pinout RFM69  →  ESP32
//    MISO        →  GPIO 19
//    MOSI        →  GPIO 23
//    SCK         →  GPIO 18
//    NSS (CS)    →  GPIO  5
//    RESET       →  GPIO 14
//    DIO0        →  GPIO  2
//
//  Pinout GPS (UART2)
//    TX (GPS)    →  GPIO 16  (RX2 del ESP32)
//    RX (GPS)    →  GPIO 17  (TX2 del ESP32)
//
//  DHT22
//    DATA        →  GPIO  4
// ============================================================

#include <Arduino.h>
#include <SPI.h>
#include <RFM69.h>          // LowPowerLab/RFM69
#include <TinyGPSPlus.h>
#include <DHT.h>

// ─── Configuración de red LoRa ────────────────────────────
#define NETWORK_ID   100    // Mismo en TX y RX
#define NODE_ID        2    // ID único de este nodo
#define GATEWAY_ID     1    // ID del receptor (TTGO)
#define FREQUENCY    RF69_915MHZ   // 433 / 868 / 915
#define ENCRYPT_KEY  "GeoTrack2024Key!"  // 16 bytes exactos
#define IS_RFM69HW   true   // true si es HW/HCW (alta potencia)

// ─── Pines RFM69 ──────────────────────────────────────────
#define RFM69_CS      5
#define RFM69_RST    14
#define RFM69_IRQ     2

// ─── Pines GPS ────────────────────────────────────────────
#define GPS_RX_PIN   16     // ESP32 RX2 ← GPS TX
#define GPS_TX_PIN   17     // ESP32 TX2 → GPS RX
#define GPS_BAUD   9600

// ─── Sensor DHT ───────────────────────────────────────────
#define DHT_PIN       4
#define DHT_TYPE    DHT22   // Cambiar a DHT11 si corresponde

// ─── Intervalo de envío ───────────────────────────────────
#define TX_INTERVAL_MS  30000UL   // 30 segundos

// ─── Estructura del payload ───────────────────────────────
// Usamos un struct serializado para compactar los datos
struct Payload {
    uint8_t  nodeId;
    float    temperature;   // °C
    float    humidity;      // %
    double   latitude;
    double   longitude;
    float    altitude;      // metros
    uint8_t  satellites;
    uint32_t gpsDate;       // DDMMYYYY
    uint32_t gpsTime;       // HHMMSScc
    uint8_t  gpsValid;      // 1 = fix válido
};

// ─── Objetos ──────────────────────────────────────────────
RFM69      radio(RFM69_CS, RFM69_IRQ, IS_RFM69HW);
TinyGPSPlus gps;
DHT        dht(DHT_PIN, DHT_TYPE);
HardwareSerial gpsSerial(2);  // UART2

// ─── Variables de control ─────────────────────────────────
unsigned long lastTx = 0;

// ─── Funciones auxiliares ─────────────────────────────────
void resetRFM69() {
    pinMode(RFM69_RST, OUTPUT);
    digitalWrite(RFM69_RST, HIGH);
    delay(10);
    digitalWrite(RFM69_RST, LOW);
    delay(10);
}

void readGPS(unsigned long ms) {
    unsigned long start = millis();
    while (millis() - start < ms) {
        while (gpsSerial.available())
            gps.encode(gpsSerial.read());
    }
}

void printPayload(const Payload& p) {
    Serial.println(F("─────── Datos a enviar ───────"));
    Serial.printf("  Nodo       : %u\n", p.nodeId);
    Serial.printf("  Temp       : %.2f °C\n", p.temperature);
    Serial.printf("  Humedad    : %.2f %%\n", p.humidity);
    if (p.gpsValid) {
        Serial.printf("  Latitud    : %.6f\n", p.latitude);
        Serial.printf("  Longitud   : %.6f\n", p.longitude);
        Serial.printf("  Altitud    : %.1f m\n", p.altitude);
        Serial.printf("  Satélites  : %u\n", p.satellites);
        Serial.printf("  Fecha GPS  : %08u\n", p.gpsDate);
        Serial.printf("  Hora GPS   : %08u\n", p.gpsTime);
    } else {
        Serial.println("  GPS        : Sin fix");
    }
    Serial.println(F("─────────────────────────────"));
}

// ─── Setup ────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(F("\n=== GeoTrack-Agro TRANSMISOR ==="));

    // GPS
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println(F("[GPS] UART2 iniciado"));

    // DHT
    dht.begin();
    Serial.println(F("[DHT] Sensor iniciado"));

    // RFM69
    resetRFM69();
    if (!radio.initialize(FREQUENCY, NODE_ID, NETWORK_ID)) {
        Serial.println(F("[RFM69] ERROR: No se pudo inicializar"));
        while (true) delay(1000);
    }
    if (IS_RFM69HW) radio.setHighPower();
    radio.encrypt(ENCRYPT_KEY);
    radio.setPowerLevel(23);  // 0–31 (23 ≈ +13 dBm para HCW)
    Serial.printf("[RFM69] OK — NodeID=%u NetID=%u\n", NODE_ID, NETWORK_ID);
}

// ─── Loop ─────────────────────────────────────────────────
void loop() {
    // Alimentar el parser GPS continuamente
    while (gpsSerial.available())
        gps.encode(gpsSerial.read());

    if (millis() - lastTx >= TX_INTERVAL_MS) {
        lastTx = millis();

        // 1) Leer GPS (espera hasta 2 s para nuevas tramas)
        readGPS(2000);

        // 2) Leer DHT
        float temp = dht.readTemperature();
        float hum  = dht.readHumidity();
        if (isnan(temp) || isnan(hum)) {
            Serial.println(F("[DHT] Error de lectura, usando 0"));
            temp = 0.0f;
            hum  = 0.0f;
        }

        // 3) Armar payload
        Payload pkt;
        pkt.nodeId      = NODE_ID;
        pkt.temperature = temp;
        pkt.humidity    = hum;

        if (gps.location.isValid() && gps.location.age() < 5000) {
            pkt.gpsValid   = 1;
            pkt.latitude   = gps.location.lat();
            pkt.longitude  = gps.location.lng();
            pkt.altitude   = (float)gps.altitude.meters();
            pkt.satellites = (uint8_t)gps.satellites.value();

            // Fecha: DDMMYYYY
            pkt.gpsDate = (uint32_t)gps.date.day()   * 1000000UL
                        + (uint32_t)gps.date.month()  * 10000UL
                        + (uint32_t)gps.date.year();

            // Hora: HHMMSScc
            pkt.gpsTime = (uint32_t)gps.time.hour()   * 1000000UL
                        + (uint32_t)gps.time.minute()  * 10000UL
                        + (uint32_t)gps.time.second()  * 100UL
                        + (uint32_t)gps.time.centisecond();
        } else {
            pkt.gpsValid   = 0;
            pkt.latitude   = 0.0;
            pkt.longitude  = 0.0;
            pkt.altitude   = 0.0f;
            pkt.satellites = 0;
            pkt.gpsDate    = 0;
            pkt.gpsTime    = 0;
        }

        printPayload(pkt);

        // 4) Enviar por LoRa (con ACK, 3 reintentos)
        bool ok = radio.sendWithRetry(
            GATEWAY_ID,
            (const void*)&pkt,
            sizeof(pkt),
            3,      // reintentos
            50      // timeout ms por reintento
        );

        if (ok) {
            Serial.printf("[TX] Paquete enviado OK — RSSI=%d dBm\n", radio.RSSI);
        } else {
            Serial.println(F("[TX] Sin ACK del gateway"));
        }

        radio.sleep();  // Bajo consumo entre transmisiones
    }
}