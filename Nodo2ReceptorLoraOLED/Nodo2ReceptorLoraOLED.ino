#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Definición de pines para TTGO LoRa32 v1.6 / v2.1
#define SCK     5
#define MISO    19
#define MOSI    27
#define SS      18
#define RST     14
#define DIO0    26

// Pines del OLED
#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Frecuencia (433E6, 868E6 o 915E6 según tu región)
#define BAND 915E6 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);

  // Inicializar pines OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(20);
  digitalWrite(OLED_RST, HIGH);

  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Error en OLED"));
    for(;;);
  }

  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("Iniciando Receptor...");
  display.display();

  // Configurar LoRa SPI y radio
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("Fallo LoRa!");
    display.display();
    while (1);
  }
  LoRa.setSyncWord(0xF3);
  Serial.println("LoRa Initializing OK!");
  
  display.clearDisplay();
  display.setCursor(0,0);
  display.print("LoRa OK!");
  display.display();
  delay(1000);
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String packet = "";
    while (LoRa.available()) {
      packet = LoRa.readString();
    }

    int rssi = LoRa.packetRssi();

    // Mostrar en OLED
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(1);
    display.println("FloodGuard:");
    display.setCursor(0,10);
    display.println(packet);
    display.display();

int posicionEtiqueta = packet.indexOf("Nivel:");

if (posicionEtiqueta != -1) {
    // 2. Extraemos el texto que sigue después de "Nivel: " (que son 6 caracteres + el espacio)
    // El valor empieza en: posicionEtiqueta + longitud de "Nivel: "
    String valorExtraido = packet.substring(posicionEtiqueta + 7);
    
    // 3. Convertimos a float
    float nivelFloat = valorExtraido.toFloat();

    // Verificación en el Monitor Serial
    Serial.print("Nivel convertido: ");
    Serial.println(nivelFloat, 2); // Imprime con 2 decimales
    if (nivelFloat >90){
      display.setCursor(10,30);
    display.setTextSize(2);
    display.println("ALERTA!!");
    display.display();
    }
    
  }// Mostrar en Serial
    Serial.print("Mensaje: ");
    Serial.println(packet);
}

    
}