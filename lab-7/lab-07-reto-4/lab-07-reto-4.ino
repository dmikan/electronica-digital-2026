/*
 * lab-07-parte2-motor-driver.ino
 * Sesión 7 — Parte 2: Consigna de velocidad por Monitor Serie (Reto 4)
 *
 * Conexión del Display OLED SSD1306 (I2C):
 *   - VCC -> 5V del Arduino
 *   - GND -> GND común
 *   - SDA -> A4 (en Arduino Uno)
 *   - SCL -> A5 (en Arduino Uno)
 *
 * Conexión del Motor y Encoder:
 *   - ENA -> D9 (PWM)
 *   - IN1 -> D7, IN2 -> D8
 *   - Canal A -> D2 (INT0)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dimensiones de la pantalla OLED
#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  64
#define DIRECCION_OLED 0x3C  // Dirección I2C típica del OLED (puede ser 0x3D)

Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

// Definición de pines
const uint8_t PIN_ENA   = 9;
const uint8_t PIN_IN1   = 7;
const uint8_t PIN_IN2   = 8;
const uint8_t PIN_ENC_A = 2;

// --- PARAMETROS DE CARACTERIZACIÓN DEL RETO 3 ---
// Sustituye estos dos valores por los calculados en el Reto 3
const float UMBRAL_DUTY = 56.0;   // Menor duty > 0 que hace girar el motor
const float PENDIENTE_M = 0.75;   // Pendiente en RPM/Duty de la zona lineal

// Parámetros físicos y de temporización
const float PULSOS_POR_REV = 700.0;
const unsigned long VENTANA_MS = 1000;

// Variables globales
volatile unsigned long contadorPulsos = 0;
unsigned long tiempoAnterior = 0;

float rpmObjetivo = 0.0;
float rpmMedida   = 0.0;
uint8_t dutyActual = 0;

void contarPulso() {
  contadorPulsos++;
}

// Función para traducir RPM a Duty usando el modelo lineal inverso
uint8_t rpmADuty(float targetRPM) {
  if (targetRPM <= 0.0) return 0;
  
  // Ecuación inversa: Duty = Umbral + (RPM / m)
  float calcDuty = UMBRAL_DUTY + (targetRPM / PENDIENTE_M);
  
  // Limitar valor entre 0 y 255
  if (calcDuty > 255.0) calcDuty = 255.0;
  if (calcDuty < 0.0) calcDuty = 0.0;
  
  return (uint8_t)calcDuty;
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  pinMode(PIN_ENC_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), contarPulso, RISING);

  // Sentido de giro: Avance
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, 0);

  // Inicialización de la pantalla OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_OLED)) {
    Serial.println(F("Fallo al inicializar pantalla OLED SSD1306"));
    for (;;);
  }
  
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(F("Reto 4: Consigna RPM"));
  display.println(F("Esperando datos..."));
  display.display();

  Serial.println(F("=================================================="));
  Serial.println(F(" Reto 4: Consigna de Velocidad en Unidades Fisicas "));
  Serial.println(F("=================================================="));
  Serial.println(F("Ingresa la velocidad objetivo en RPM (ej. 60 o 120):"));
}

void loop() {
  // 1. Lectura NO BLOQUEANTE de comandos por Monitor Serie
  if (Serial.available() > 0) {
    float entradaRPM = Serial.parseFloat();
    // Limpiar saltos de línea residuales
    while (Serial.available() > 0) Serial.read();

    if (entradaRPM >= 0.0) {
      rpmObjetivo = entradaRPM;
      dutyActual = rpmADuty(rpmObjetivo);
      analogWrite(PIN_ENA, dutyActual);

      Serial.print(F(">>> Nueva Consigna: "));
      Serial.print(rpmObjetivo, 1);
      Serial.print(F(" RPM | Duty Calculado: "));
      Serial.println(dutyActual);
    }
  }

  // 2. Temporización NO BLOQUEANTE con millis() para el conteo de pulsos
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    // Lectura atómica dentro de sección crítica
    noInterrupts();
    unsigned long pulsosEnVentana = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    // Cálculo de RPM medida
    rpmMedida = (pulsosEnVentana / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);

    // Calcular error relativo porcentual
    float errorPorcentual = 0.0;
    if (rpmObjetivo > 0.0) {
      errorPorcentual = (abs(rpmMedida - rpmObjetivo) / rpmObjetivo) * 100.0;
    }

    // Reporte en Monitor Serie
    Serial.print(F("Consigna: "));
    Serial.print(rpmObjetivo, 1);
    Serial.print(F(" RPM | Medida: "));
    Serial.print(rpmMedida, 1);
    Serial.print(F(" RPM | Duty: "));
    Serial.print(dutyActual);
    Serial.print(F(" | Error: "));
    Serial.print(errorPorcentual, 1);
    Serial.println(F("%"));

    // 3. Actualización de la pantalla OLED
    display.clearDisplay();
    
    // Encabezado
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("   CONTROL VELOCIDAD   "));
    display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

    // Consigna (Objetivo)
    display.setCursor(0, 16);
    display.print(F("Objetivo: "));
    display.print(rpmObjetivo, 1);
    display.println(F(" RPM"));

    // Velocidad Medida
    display.setCursor(0, 30);
    display.print(F("Medida:   "));
    display.print(rpmMedida, 1);
    display.println(F(" RPM"));

    // Duty y Error
    display.setCursor(0, 44);
    display.print(F("Duty: "));
    display.print(dutyActual);
    display.print(F(" | Err: "));
    display.print(errorPorcentual, 0);
    display.println(F("%"));

    display.display();

    tiempoAnterior = tiempoActual;
  }
}