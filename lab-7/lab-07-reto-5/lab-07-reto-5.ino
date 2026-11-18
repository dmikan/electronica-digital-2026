/*
 * lab-07-parte2-motor-driver.ino
 * Sesión 7 — Parte 2: Cuadratura: dirección y vueltas netas (Reto 5)
 *
 * Conexión del Encoder:
 *   - VCC -> 5V del Arduino (¡Nunca a la fuente externa!)
 *   - GND -> GND común
 *   - Canal A -> Pin D2 (INT0)
 *   - Canal B -> Pin D3
 *
 * Conexión del Display OLED SSD1306 (I2C):
 *   - VCC -> 5V, GND -> GND, SDA -> A4, SCL -> A5
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  64
#define DIRECCION_OLED 0x3C

Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

// Definición de pines
const uint8_t PIN_ENA   = 9;
const uint8_t PIN_IN1   = 7;
const uint8_t PIN_IN2   = 8;
const uint8_t PIN_ENC_A = 2;    // Interrupción INT0 (Canal A)
const uint8_t PIN_ENC_B = 3;    // Lectura digital (Canal B)

// --- CONFIGURACIÓN DE RESOLUCIÓN ---
// Si interrupción en A es RISING -> 700.0 (1 flanco por periodo)
// Si interrupción en A es CHANGE -> 1400.0 (2 flancos por periodo)
const float PULSOS_POR_REV = 700.0;
const unsigned long VENTANA_MS = 1000;

// Variables globales de la ISR (deben ser volatile y con signo)
volatile long contadorPulsosNetos = 0;  // Acumulativo neto (suma/resta)
volatile int8_t ultimoSentido = 0;       // +1: Avance, -1: Reversa, 0: Detenido

// Variables de medición de velocidad y tiempo
unsigned long tiempoAnterior = 0;
long ultimoConteoParaRPM = 0;

uint8_t dutyActual = 150; // Duty de prueba
bool direccionForward = true;

// Rutina de Servicio de Interrupción (ISR) optimizada y muy breve
void ISR_canalA() {
  // Se lee el estado del Canal B en el momento exacto de la interrupción en A
  bool estadoB = digitalRead(PIN_ENC_B);
  
  // Para flanco RISING en Canal A:
  // Si B es LOW -> sentido positivo (+1)
  // Si B es HIGH -> sentido negativo (-1)
  if (!estadoB) {
    contadorPulsosNetos++;
    ultimoSentido = 1;
  } else {
    contadorPulsosNetos--;
    ultimoSentido = -1;
  }
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  // Configuración de los pines del encoder
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  pinMode(PIN_ENC_B, INPUT_PULLUP);

  // Configurar interrupción en Canal A (flanco RISING para 700 PPR)
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), ISR_canalA, RISING);

  // Sentido inicial: Avance (IN1=HIGH, IN2=LOW)
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, dutyActual);

  // Inicialización de la OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_OLED)) {
    Serial.println(F("Fallo al inicializar pantalla OLED"));
    for (;;);
  }

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Reto 5: Cuadratura"));
  display.display();

  Serial.println(F("=================================================="));
  Serial.println(F(" Reto 5: Decodificacion de Sentido y Conteo Neto  "));
  Serial.println(F("=================================================="));
  Serial.println(F("Comandos por Monitor Serie:"));
  Serial.println(F("  'f' -> Cambiar a sentido Avance (IN1=1, IN2=0)"));
  Serial.println(F("  'r' -> Cambiar a sentido Reversa (IN1=0, IN2=1)"));
  Serial.println(F("  's' -> Detener el motor (Duty=0)"));
  Serial.println(F("  '0' -> Reiniciar el contador neto a cero"));
  Serial.println(F("  [0-255] -> Cambiar el ciclo de trabajo (Duty)"));
  Serial.println(F("--------------------------------------------------"));
}

void loop() {
  // 1. Control por Monitor Serie
  if (Serial.available() > 0) {
    char c = Serial.peek();
    
    if (c == 'f' || c == 'F') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, LOW);
      direccionForward = true;
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> Dirección ordenada: AVANCE (CW)"));
    } else if (c == 'r' || c == 'R') {
      Serial.read();
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, HIGH);
      direccionForward = false;
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> Dirección ordenada: REVERSA (CCW)"));
    } else if (c == 's' || c == 'S') {
      Serial.read();
      analogWrite(PIN_ENA, 0);
      Serial.println(F(">>> Motor Detenido"));
    } else if (c == '0') {
      Serial.read();
      noInterrupts();
      contadorPulsosNetos = 0;
      interrupts();
      Serial.println(F(">>> Conteo neto reiniciado a 0"));
    } else {
      int val = Serial.parseInt();
      if (val >= 0 && val <= 255) {
        dutyActual = (uint8_t)val;
        analogWrite(PIN_ENA, dutyActual);
        Serial.print(F(">>> Duty cambiado a: "));
        Serial.println(dutyActual);
      }
    }
    while (Serial.available() > 0) Serial.read();
  }

  // 2. Procesamiento en ventana fija
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    // Lectura atómica de la sección crítica
    noInterrupts();
    long pulsosActuales = contadorPulsosNetos;
    int8_t sentidoActual = ultimoSentido;
    interrupts();

    // Cálculo del delta de pulsos en la ventana para hallar RPM
    long deltaPulsos = pulsosActuales - ultimoConteoParaRPM;
    ultimoConteoParaRPM = pulsosActuales;

    float rpmMedida = (abs(deltaPulsos) / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);
    float vueltasTotales = (float)pulsosActuales / PULSOS_POR_REV;

    // Reporte por el Monitor Serie
    Serial.print(F("Conteo Neto: "));
    Serial.print(pulsosActuales);
    Serial.print(F("\tVueltas: "));
    Serial.print(vueltasTotales, 2);
    Serial.print(F("\tSentido: "));
    if (deltaPulsos > 0) Serial.print(F("AVANCE (+)"));
    else if (deltaPulsos < 0) Serial.print(F("REVERSA (-)"));
    else Serial.print(F("QUIETO (0)"));
    Serial.print(F("\tRPM: "));
    Serial.println(rpmMedida, 1);

    // 3. Actualización de la Pantalla OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F(" Reto 5: Cuadratura "));
    display.drawFastHLine(0, 9, 128, SSD1306_WHITE);

    // Conteo neto
    display.setCursor(0, 14);
    display.print(F("Pulsos:  "));
    display.println(pulsosActuales);

    // Vueltas netas
    display.setCursor(0, 27);
    display.print(F("Vueltas: "));
    display.print(vueltasTotales, 2);

    // Sentido detectado por el encoder
    display.setCursor(0, 40);
    display.print(F("Sentido: "));
    if (sentidoActual > 0 && deltaPulsos > 0) display.println(F("AVANCE (+)"));
    else if (sentidoActual < 0 && deltaPulsos < 0) display.println(F("REVERSA (-)"));
    else display.println(F("DETENIDO"));

    // Velocidad instantánea
    display.setCursor(0, 53);
    display.print(F("RPM: "));
    display.print(rpmMedida, 1);
    display.print(F(" | Duty:"));
    display.print(dutyActual);

    display.display();

    tiempoAnterior = tiempoActual;
  }
}