/*
 * lab-06-reto-3-oled.ino
 *
 * Reto 3: Dos valores en pantalla a ritmo fijo (OLED SSD1306 128x64)
 *
 * Muestra simultáneamente Potenciómetro (A2) y Fotoresistencia (A0)
 * junto con un contador de cuadros que se incrementa a 2 Hz (cada 500 ms).
 * En 10 segundos el contador avanza exactamente 20 unidades.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dimensiones de la pantalla OLED
const uint8_t ANCHO_PX = 128;
const uint8_t ALTO_PX = 64;

// Instancia de la pantalla
Adafruit_SSD1306 display(ANCHO_PX, ALTO_PX, &Wire, -1);

// Dirección I2C típica
uint8_t direccionOLED = 0x3C;

// Asignación de pines analógicos
const uint8_t PIN_POT = A2;
const uint8_t PIN_LDR = A0;

// Contador de cuadros/refrescos de pantalla
unsigned long contadorCuadros = 0;

void mostrarEstado() {
  // Lectura de ambos canales analógicos
  int valPot = analogRead(PIN_POT);
  int valLdr = analogRead(PIN_LDR);

  float pctPot = valPot * 100.0 / 1023.0;
  float pctLdr = valLdr * 100.0 / 1023.0;

  // Limpiar el buffer para evitar caracteres residuales
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Título / Encabezado
  display.setCursor(0, 0);
  display.print(F("RETO 3 | Cuadro: "));
  display.println(contadorCuadros);

  display.println(F("---------------------"));

  // Canal 1: Potenciómetro
  display.print(F("POT (A2): "));
  display.print(valPot);
  display.print(F(" ("));
  display.print(pctPot, 1);
  display.println(F("%)"));

  // Canal 2: Fotoresistencia
  display.print(F("LDR (A0): "));
  display.print(valLdr);
  display.print(F(" ("));
  display.print(pctLdr, 1);
  display.println(F("%)"));

  // Enviar buffer a la pantalla física
  display.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, direccionOLED)) {
    Serial.println(F("Error al inicializar display OLED"));
    while (true);
  }

  display.clearDisplay();
  display.display();
}

void loop() {
  static unsigned long tUltimaActualizacion = 0;
  unsigned long ahora = millis();

  // Ritmo fijo de 2 actualizaciones por segundo (cada 500 ms)
  if (ahora - tUltimaActualizacion >= 500) {
    tUltimaActualizacion = ahora;
    contadorCuadros++; // Incrementa 1 unidad por cada actualización (20 en 10 s)
    mostrarEstado();
  }
}
