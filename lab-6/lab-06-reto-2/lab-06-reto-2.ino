/*
 * lab-06-parte1-scanner-oled.ino
 *
 * Reconocimiento del bus I2C y puesta en marcha del display OLED SSD1306 128x64.
 * Permite cambiar de canal (Potenciómetro / Fotoresistencia) desde el Monitor Serie.
 *
 * Circuito: módulo OLED alimentado a 5 V, SDA a A4, SCL a A5.
 *           Potenciómetro en A2, Fotoresistencia en A0.
 *
 * Monitor Serie: 9600 baudios.
 */

#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Geometría del módulo
const uint8_t ANCHO_PX = 128;
const uint8_t ALTO_PX = 64;

// Instancia del display
Adafruit_SSD1306 display(ANCHO_PX, ALTO_PX, &Wire, -1);

// Dirección del display
uint8_t direccionOLED = 0x3C;

// Enumeración para la selección de canales
enum TipoCanal {
  Potenciometro,
  Fotoresistencia
};

// Variables globales para el canal y pin analógico actual
TipoCanal canalActual = Potenciometro;
uint8_t analogPin = A2;

bool esDireccionDeSSD1306(uint8_t dir) {
  return (dir == 0x3C) || (dir == 0x3D);
}

uint8_t barrerBus() {
  uint8_t encontrados = 0;

  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("Dispositivo en 0x"));
      if (dir < 16) {
        Serial.print('0');
      }
      Serial.println(dir, HEX);
      encontrados++;

      if (esDireccionDeSSD1306(dir)) {
        direccionOLED = dir;
      }
    }
  }

  if (encontrados == 0) {
    Serial.println(F("Sin dispositivos en el bus"));
  }

  return encontrados;
}

// Función para actualizar el pin analógico según el canal seleccionado
void actualizarPinCanal() {
  switch (canalActual) {
    case Potenciometro:
      analogPin = A2;
      break;
    case Fotoresistencia:
      analogPin = A0;
      break;
    default:
      analogPin = A2;
      break;
  }
}

// Procesa los caracteres ingresados desde el Monitor Serie
void atenderMonitorSerie() {
  if (Serial.available() > 0) {
    char opcion = Serial.read();

    if (opcion == '1') {
      canalActual = Potenciometro;
      actualizarPinCanal();
      Serial.println(F("Canal seleccionado: 1 - Potenciometro (A2)"));
    } else if (opcion == '2') {
      canalActual = Fotoresistencia;
      actualizarPinCanal();
      Serial.println(F("Canal seleccionado: 2 - Fotoresistencia (A0)"));
    }
  }
}

void mostrarEstado(unsigned long t) {
  int lectura = analogRead(analogPin);
  float porcentaje = lectura * 100.0 / 1023.0;

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  // Muestra el nombre según el canal activo
  if (canalActual == Potenciometro) {
    display.println(F("Potenciometro (A2)"));
  } else {
    display.println(F("Fotoresistencia (A0)"));
  }

  display.println(); // Línea de separación

  display.print(F("Cuentas: "));
  display.println(lectura);

  display.print(F("Porcentaje: "));
  display.print(porcentaje, 1);
  display.println(F(" %"));

  display.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println(F("Barrido del bus I2C"));
  uint8_t dispositivos = barrerBus();
  Serial.print(F("Dispositivos encontrados: "));
  Serial.println(dispositivos);

  if (!display.begin(SSD1306_SWITCHCAPVCC, direccionOLED)) {
    Serial.println(F("El display no respondio en la direccion detectada"));
    Serial.println(F("Verificar SDA, SCL, VCC y GND del modulo"));
    while (true) {
    }
  }

  Serial.print(F("Display inicializado en 0x"));
  Serial.println(direccionOLED, HEX);

  // Instrucciones iniciales en el Monitor Serie
  Serial.println(F("---"));
  Serial.println(F("Seleccion de canal por Monitor Serie:"));
  Serial.println(F("  Escribe '1' para Potenciometro (A2)"));
  Serial.println(F("  Escribe '2' para Fotoresistencia (A0)"));
  Serial.println(F("---"));

  actualizarPinCanal();
}

void loop() {
  // Comprobar constantemente si el usuario envió un comando por Serial
  atenderMonitorSerie();

  static unsigned long tUltimaActualizacion = 0;
  unsigned long ahora = millis();

  // Refresco del display OLED cada 500 ms
  if (ahora - tUltimaActualizacion >= 500) {
    tUltimaActualizacion = ahora;
    mostrarEstado(ahora);
  }
}
