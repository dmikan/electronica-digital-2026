/*
 * lab-08-reto1-dos-ruedas-definitivo.ino
 * Sesión 8 — Reto 1: Las dos ruedas, medidas (Conteo Absoluto Estable)
 * 
 * Conexiones Módulo L298N:
 *   - ENA  -> Pin D9  (PWM Motor 1) [¡Retirar Jumper ENA!]
 *   - ENB  -> Pin D10 (PWM Motor 2)
 *   - IN1  -> Pin D4  | IN2 -> Pin D5 (Motor 1 - Izquierdo)
 *   - IN3  -> Pin D6  | IN4 -> Pin D7 (Motor 2 - Derecho)
 *
 * Conexión Encoders (VCC a 5V Arduino, GND común):
 *   - Encoder 1: Canal A -> Pin D2 (INT0)
 *   - Encoder 2: Canal A -> Pin D3 (INT1)
 *
 * Conexión Pantalla OLED SSD1306 (I2C):
 *   - VCC -> 5V, GND -> GND, SDA -> A4, SCL -> A5
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  64
#define DIRECCION_OLED 0x3C

Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

// --- PINES DE MOTORES Y ENCODERS ---
const uint8_t PIN_ENA  = 9;
const uint8_t PIN_ENB  = 10;
const uint8_t PIN_IN1  = 4;
const uint8_t PIN_IN2  = 5;
const uint8_t PIN_IN3  = 6;
const uint8_t PIN_IN4  = 7;

const uint8_t PIN_ENC_A1 = 2;  // INT0 (Motor 1)
const uint8_t PIN_ENC_A2 = 3;  // INT1 (Motor 2)

// --- PARÁMETROS DE MEDICIÓN ---
const float PULSOS_POR_REV = 700.0;     // 700.0 PPR para flanco RISING
const unsigned long INTERVALO_MS = 200; // Intervalo de refresco (200 ms)

uint8_t dutyActual = 150; // PWM inicial (0-255)

// --- VARIABLES GLOBALES DE ISR (Volatile) ---
volatile long pulsosMotor1 = 0;
volatile long pulsosMotor2 = 0;

// ISRs ultra-estables: Conteo incremental directo (Sin auto-cancelación)
void ISR_motor1() {
  pulsosMotor1++;
}

void ISR_motor2() {
  pulsosMotor2++;
}

// Variables de seguimiento temporal
unsigned long tiempoAnterior = 0;
bool oledDisponible = false;

void setup() {
  Serial.begin(115200); // 115200 baudios requeridos por la práctica

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  pinMode(PIN_ENC_A1, INPUT_PULLUP);
  pinMode(PIN_ENC_A2, INPUT_PULLUP);

  // Interrupciones independientes activadas en flanco de subida RISING
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A1), ISR_motor1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A2), ISR_motor2, RISING);

  // Sentido de marcha inicial: Ambos motores en Avance (FWD)
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, HIGH);
  digitalWrite(PIN_IN4, LOW);

  analogWrite(PIN_ENA, dutyActual);
  analogWrite(PIN_ENB, dutyActual);

  if (!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_OLED)) {
    Serial.println(F("ADVERTENCIA: OLED no detectada en 0x3C."));
    oledDisponible = false;
  } else {
    oledDisponible = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Sesion 8 - Reto 1"));
    display.println(F("Lectura 100% Estable"));
    display.display();
    delay(1000);
  }

  Serial.println(F("=========================================================="));
  Serial.println(F(" RETO 1: MEDICIÓN CONTINUA Y ESTABLE DE RPM EN 2 MOTORES  "));
  Serial.println(F("=========================================================="));

  tiempoAnterior = millis();
}

void loop() {
  // 1. COMANDOS POR MONITOR SERIE (FRENO Y DIRECCIÓN)
  if (Serial.available() > 0) {
    char c = Serial.peek();

    // Motor 1 (Izquierdo)
    if (c == 'f' || c == 'F') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, LOW);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: AVANCE (CW)"));
    } else if (c == 'r' || c == 'R') {
      Serial.read();
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, HIGH);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: REVERSA (CCW)"));
    } else if (c == 'x' || c == 'X') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, HIGH);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: FRENO ACTIVO"));
    } else if (c == 's' || c == 'S') {
      Serial.read();
      analogWrite(PIN_ENA, 0);
      Serial.println(F(">>> M1: Detenido"));
    }

    // Motor 2 (Derecho)
    else if (c == 'd' || c == 'D') {
      Serial.read();
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, LOW);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: AVANCE (CW)"));
    } else if (c == 'e' || c == 'E') {
      Serial.read();
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: REVERSA (CCW)"));
    } else if (c == 'c' || c == 'C') {
      Serial.read();
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: FRENO ACTIVO"));
    } else if (c == 'a' || c == 'A') {
      Serial.read();
      analogWrite(PIN_ENB, 0);
      Serial.println(F(">>> M2: Detenido"));
    }

    // Comandos Generales
    else if (c == 'q' || c == 'Q') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, HIGH);
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENA, dutyActual);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> FRENO ACTIVO AMBOS MOTORES"));
    } else if (c == '0') {
      Serial.read();
      noInterrupts();
      pulsosMotor1 = 0;
      pulsosMotor2 = 0;
      interrupts();
      Serial.println(F(">>> Pulsos reiniciados a 0"));
    } else {
      int val = Serial.parseInt();
      if (val >= 0 && val <= 255) {
        dutyActual = (uint8_t)val;
        analogWrite(PIN_ENA, dutyActual);
        analogWrite(PIN_ENB, dutyActual);
        Serial.print(F(">>> Duty PWM asignado a: "));
        Serial.println(dutyActual);
      }
    }
    while (Serial.available() > 0) Serial.read();
  }

  // 2. MEDICIÓN CADA 200 MS CON CÁLCULO PRECISO
  unsigned long tiempoActual = millis();
  unsigned long dt = tiempoActual - tiempoAnterior;

  if (dt >= INTERVALO_MS) {
    tiempoAnterior = tiempoActual;

    // Sección crítica atómica
    noInterrupts();
    long rawPulsosM1 = pulsosMotor1;
    long rawPulsosM2 = pulsosMotor2;
    pulsosMotor1 = 0;
    pulsosMotor2 = 0;
    interrupts();

    // Cálculo exacto de RPM basado en los pulsos brutos acumulados
    float rpmM1 = (rawPulsosM1 / PULSOS_POR_REV) * (60000.0 / (float)dt);
    float rpmM2 = (rawPulsosM2 / PULSOS_POR_REV) * (60000.0 / (float)dt);

    // Determinación de sentido impecable por pines del Puente H
    String sentidoM1, sentidoM2;

    if (digitalRead(PIN_IN1) == HIGH && digitalRead(PIN_IN2) == HIGH) {
      sentidoM1 = "BRAKE";
    } else if (digitalRead(PIN_IN1) == HIGH && digitalRead(PIN_IN2) == LOW && rpmM1 > 1.0) {
      sentidoM1 = "FWD";
    } else if (digitalRead(PIN_IN1) == LOW && digitalRead(PIN_IN2) == HIGH && rpmM1 > 1.0) {
      sentidoM1 = "REV";
    } else {
      sentidoM1 = "STOP   ";
    }

    if (digitalRead(PIN_IN3) == HIGH && digitalRead(PIN_IN4) == HIGH) {
      sentidoM2 = "BRAKE";
    } else if (digitalRead(PIN_IN3) == HIGH && digitalRead(PIN_IN4) == LOW && rpmM2 > 1.0) {
      sentidoM2 = "FWD";
    } else if (digitalRead(PIN_IN3) == LOW && digitalRead(PIN_IN4) == HIGH && rpmM2 > 1.0) {
      sentidoM2 = "REV";
    } else {
      sentidoM2 = "STOP   ";
    }

    // 3. REPORTE EN MONITOR SERIE Y OLED
    Serial.print(F("M1 -> Pulsos: "));
    Serial.print(rawPulsosM1);
    Serial.print(F("\tRPM: "));
    Serial.print(rpmM1, 1);
    Serial.print(F("\tSentido: "));
    Serial.print(sentidoM1);

    Serial.print(F("\t| M2 -> Pulsos: "));
    Serial.print(rawPulsosM2);
    Serial.print(F("\tRPM: "));
    Serial.print(rpmM2, 1);
    Serial.print(F("\tSentido: "));
    Serial.println(sentidoM2);

    if (oledDisponible) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F(" Reto 1: Dos Ruedas "));
      display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

      display.setCursor(0, 16);
      display.print(F("M1: "));
      display.print(rpmM1, 1);
      display.print(F(" RPM | "));
      display.println(sentidoM1);

      display.setCursor(0, 32);
      display.print(F("M2: "));
      display.print(rpmM2, 1);
      display.print(F(" RPM | "));
      display.println(sentidoM2);

      display.setCursor(0, 48);
      display.print(F("Duty PWM: "));
      display.print(dutyActual);

      display.display();
    }
  }
}