/*
 * lab-06-reto-4-oled.ino
 *
 * Reto 4: Panel de tres pantallas con estadísticas por canal (OLED SSD1306)
 *
 * Circuito:
 *   - OLED SSD1306 I2C (SDA -> A4, SCL -> A5)
 *   - Potenciómetro -> A2
 *   - Fotoresistencia (LDR) -> A0
 *   - Pulsador en D2 (Pulldown: Resistencia 10k a GND, botón a +5V)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Geometría del OLED
const uint8_t ANCHO_PX = 128;
const uint8_t ALTO_PX = 64;

// Instancia OLED
Adafruit_SSD1306 display(ANCHO_PX, ALTO_PX, &Wire, -1);
uint8_t direccionOLED = 0x3C;

// Asignación de Pines
const uint8_t PIN_POT = A2;
const uint8_t PIN_LDR = A0;
const uint8_t PIN_BOTON = 2;

// Anti-rebote para el pulsador (D2)
int estadoBotonPrevio = LOW;
unsigned long ultimoTiempoDebounce = 0;
const unsigned long DELAY_DEBOUNCE = 50; // 50 ms antirrebote

// Control de pantalla (0: General, 1: Detalle POT, 2: Detalle LDR)
uint8_t pantallaActual = 0;

// Variables para estadísticas acumuladas (POT)
int minPot = 1023;
int maxPot = 0;
unsigned long sumaPot = 0;

// Variables para estadísticas acumuladas (LDR)
int minLdr = 1023;
int maxLdr = 0;
unsigned long sumaLdr = 0;

// Contador de muestras totales procesadas
unsigned long totalMuestras = 0;

// Lee el pulsador con filtro antirrebote
void verificarPulsador() {
  int lectura = digitalRead(PIN_BOTON);

  if (lectura != estadoBotonPrevio) {
    ultimoTiempoDebounce = millis();
  }

  if ((millis() - ultimoTiempoDebounce) > DELAY_DEBOUNCE) {
    static int estadoBotonEstable = LOW;
    if (lectura != estadoBotonEstable) {
      estadoBotonEstable = lectura;
      // Al detectar flanco de subida (presionado)
      if (estadoBotonEstable == HIGH) {
        pantallaActual = (pantallaActual + 1) % 3; // Modulo 3: conmuta 0 -> 1 -> 2 -> 0
      }
    }
  }
  estadoBotonPrevio = lectura;
}

// Procesa lecturas analógicas y actualiza mínimos, máximos y promedios
void actualizarEstadisticas(int valPot, int valLdr) {
  totalMuestras++;

  // Actualizar Potenciómetro
  if (valPot < minPot) minPot = valPot;
  if (valPot > maxPot) maxPot = valPot;
  sumaPot += valPot;

  // Actualizar Fotoresistencia
  if (valLdr < minLdr) minLdr = valLdr;
  if (valLdr > maxLdr) maxLdr = valLdr;
  sumaLdr += valLdr;
}

void renderizarOLED(int valPot, int valLdr, float promPot, float promLdr) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);

  switch (pantallaActual) {
    case 0: // PANTALLA GENERAL
      display.println(F("=== VISTA GENERAL ==="));
      display.println();
      display.print(F("POT (A2): "));
      display.print(valPot);
      display.print(F(" ("));
      display.print(valPot * 100.0 / 1023.0, 0);
      display.println(F("%)"));

      display.println();
      display.print(F("LDR (A0): "));
      display.print(valLdr);
      display.print(F(" ("));
      display.print(valLdr * 100.0 / 1023.0, 0);
      display.println(F("%)"));
      break;

    case 1: // DETALLE POTENCIÓMETRO
      display.println(F("== DETALLE: POT (A2) =="));
      display.print(F("Actual:  ")); display.println(valPot);
      display.print(F("Minimo:  ")); display.println(minPot);
      display.print(F("Maximo:  ")); display.println(maxPot);
      display.print(F("Promed:  ")); display.println(promPot, 1);
      break;

    case 2: // DETALLE FOTORESISTENCIA
      display.println(F("== DETALLE: LDR (A0) =="));
      display.print(F("Actual:  ")); display.println(valLdr);
      display.print(F("Minimo:  ")); display.println(minLdr);
      display.print(F("Maximo:  ")); display.println(maxLdr);
      display.print(F("Promed:  ")); display.println(promLdr, 1);
      break;
  }

  display.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  pinMode(PIN_BOTON, INPUT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, direccionOLED)) {
    Serial.println(F("Error al iniciar OLED"));
    while (true);
  }

  display.clearDisplay();
  display.display();

  Serial.println(F("POT_Val,LDR_Val,POT_Prom,LDR_Prom"));
}

void loop() {
  // Verificación constante del botón en cada iteración
  verificarPulsador();

  static unsigned long tUltimaActualizacion = 0;
  unsigned long ahora = millis();

  // Ritmo fijo de muestreo y refresco (cada 500 ms)
  if (ahora - tUltimaActualizacion >= 500) {
    tUltimaActualizacion = ahora;

    // Adquisición de canales
    int valPot = analogRead(PIN_POT);
    int valLdr = analogRead(PIN_LDR);

    // Actualización de variables estadísticas
    actualizarEstadisticas(valPot, valLdr);

    float promPot = (float)sumaPot / totalMuestras;
    float promLdr = (float)sumaLdr / totalMuestras;

    // Emisión continua por Monitor Serie (ininterrumpida por cambios de pantalla)
    Serial.print(valPot); Serial.print(F(","));
    Serial.print(valLdr); Serial.print(F(","));
    Serial.print(promPot, 1); Serial.print(F(","));
    Serial.println(promLdr, 1);

    // Renderizar en la pantalla activa
    renderizarOLED(valPot, valLdr, promPot, promLdr);
  }
}
