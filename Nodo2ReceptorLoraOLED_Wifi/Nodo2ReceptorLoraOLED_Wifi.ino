#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <ThingSpeak.h>

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

const char* ssid = "UAM-ROBOTICA";
const char* password = "m4nt32024uat";
unsigned long myChannelNumber = 3263918; // Tu Channel ID
const char* myWriteAPIKey = "8IBJDIY1424LF5H7";

WiFiClient client;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  ThingSpeak.begin(client);
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

void procesarMensaje(String data) {
  // data = "N1: 25.00 C : 65.50% Nivel: 84.14"
  
   // --- PROCESAMIENTO PARA NODO 1 ---
    if (data.startsWith("N1")) {
      // Extraer datos (basado en tu formato: "N1: 25.00 C : 65.50% Nivel: 84.14")
      float temp1 = data.substring(data.indexOf("N1:") + 3, data.indexOf(" C")).toFloat();
      float hum1  = data.substring(data.indexOf(":", data.indexOf(" C")) + 1, data.indexOf("%")).toFloat();
      float niv1  = data.substring(data.indexOf("Nivel:") + 6).toFloat();

      // Asignar a Fields 1, 2 y 3
      ThingSpeak.setField(1, temp1);
      ThingSpeak.setField(2, hum1);
      ThingSpeak.setField(3, niv1);
      
      Serial.println("Datos N1 listos para Field 1,2,3");
    } 
    
    // --- PROCESAMIENTO PARA NODO 2 ---
    else if (data.startsWith("N2")) {
      // Suponiendo que el Nodo 2 envía el mismo formato pero inicia con N2
      float temp2 = data.substring(data.indexOf("N2:") + 3, data.indexOf(" C")).toFloat();
      float hum2  = data.substring(data.indexOf(":", data.indexOf(" C")) + 1, data.indexOf("%")).toFloat();
      float niv2  = data.substring(data.indexOf("Nivel:") + 6).toFloat();

      // Asignar a Fields 4, 5 y 6
      ThingSpeak.setField(4, temp2);
      ThingSpeak.setField(5, hum2);
      ThingSpeak.setField(6, niv2);
      
      Serial.println("Datos N2 listos para Field 4,5,6");
    }

    // --- ENVÍO ÚNICO A THINGSPEAK ---
    int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
    
    if(x == 200) {
      Serial.println("ThingSpeak actualizado con éxito.");
    } else {
      Serial.println("Error en ThingSpeak: " + String(x));
    }  Serial.println("Error en la actualización. HTTP error code " + String(x));
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

    procesarMensaje(packet);
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
      display.setCursor(0,30);
    display.setTextSize(2);
    if (packet.startsWith("N1")) 
    display.println("N1 ALERTA!");
    else
    display.println("N2 ALERTA!");
    display.display();
    }
    
  }// Mostrar en Serial
    Serial.print("Mensaje: ");
    Serial.println(packet);
}

    
}