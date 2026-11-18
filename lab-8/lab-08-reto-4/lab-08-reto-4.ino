/*
 * lab-08-reto4-lazo-posicion-rampa.ino
 * Sesión 8 — Reto 4: Lazo de posición sobre rampa de ángulo
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 display(128, 64, &Wire, -1);

// --- PINES DE MOTORES ---
const uint8_t PIN_ENA = 9;
const uint8_t PIN_ENB = 10;
const uint8_t PIN_IN1 = 4;
const uint8_t PIN_IN2 = 5;
const uint8_t PIN_IN3 = 6;
const uint8_t PIN_IN4 = 7;

// CORRECCIÓN FÍSICA (M1 en D3, M2 en D2)
const bool ENCODERS_CRUZADOS = true;
const uint8_t PIN_ENC_A1 = ENCODERS_CRUZADOS ? 3 : 2;
const uint8_t PIN_ENC_A2 = ENCODERS_CRUZADOS ? 2 : 3;

// --- PARÁMETROS DE CONTROL ---
const float PULSOS_POR_REV = 700.0f;
const unsigned long PERIODO_CONTROL_MS = 20;

// Pendiente de la rampa en grados/segundo (Exigidas: 400, 500, 600 deg/s)
float velocidadRampa = 500.0f; 

// Ganancias PI calibradas para seguimiento de posición angular
const float Kp = 0.8f;
const float Ki = 0.4f;

// Variables de posición angular acumulada
float anguloRef = 0.0f;        // Consigna variante (rampa)
long totalPulsosM1 = 0;        // Contador absoluto continuo
long totalPulsosM2 = 0;

float integralM1 = 0.0f;
float integralM2 = 0.0f;

uint8_t dutyM1 = 0;
uint8_t dutyM2 = 0;

// Interrupciones de conteo continuo
volatile long pulsosMotor1 = 0;
volatile long pulsosMotor2 = 0;

void ISR_motor1() { pulsosMotor1++; }
void ISR_motor2() { pulsosMotor2++; }

unsigned long tiempoAnterior = 0;
uint8_t divisorImpresion = 0;
bool oledDisponible = false;

void setup() {
  Serial.begin(115200);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  // Sentido de marcha: Avance (FWD)
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  digitalWrite(PIN_IN3, HIGH);
  digitalWrite(PIN_IN4, LOW);

  pinMode(PIN_ENC_A1, INPUT_PULLUP);
  pinMode(PIN_ENC_A2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A1), ISR_motor1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A2), ISR_motor2, RISING);

  oledDisponible = display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  if (oledDisponible) {
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Reto 4: Posicion"));
    display.println(F("Rampa de Angulo"));
    display.display();
    delay(1000);
  }

  Serial.println(F("=========================================================="));
  Serial.println(F(" RETO 4: LAZO DE POSICIÓN ANGULAR (RAMPA DINÁMICA @ 20 ms)"));
  Serial.println(F("=========================================================="));
  Serial.println(F("Comandos por Monitor Serie para cambiar la pendiente:"));
  Serial.println(F("  '400' -> Rampa a 400 deg/s"));
  Serial.println(F("  '500' -> Rampa a 500 deg/s"));
  Serial.println(F("  '600' -> Rampa a 600 deg/s"));
  Serial.println(F("----------------------------------------------------------"));

  tiempoAnterior = millis();
}

void loop() {
  // 1. CAMBIO DE PENDIENTE POR CONSOLA
  if (Serial.available() > 0) {
    int val = Serial.parseInt();
    if (val >= 100 && val <= 1000) {
      velocidadRampa = (float)val;
      Serial.print(F(">>> Pendiente de Rampa cambiada a: "));
      Serial.print(velocidadRampa);
      Serial.println(F(" deg/s"));
    }
    while (Serial.available() > 0) Serial.read();
  }

  // 2. LAZO DE CONTROL PI CADA 20 MS
  unsigned long tiempoActual = millis();
  unsigned long dt_ms = tiempoActual - tiempoAnterior;

  if (dt_ms < PERIODO_CONTROL_MS) return;

  tiempoAnterior = tiempoActual;
  float dt_seg = dt_ms / 1000.0f; // 0.02 s

  // Copia atómica y acumulado absoluto
  noInterrupts();
  long deltaPulsosM1 = pulsosMotor1;
  long deltaPulsosM2 = pulsosMotor2;
  pulsosMotor1 = 0;
  pulsosMotor2 = 0;
  interrupts();

  totalPulsosM1 += deltaPulsosM1;
  totalPulsosM2 += deltaPulsosM2;

  // A. GENERAR LA CONSIGNA DINÁMICA (RAMPA DE ÁNGULO)
  anguloRef += (velocidadRampa * dt_seg);

  // B. MEDIR ÁNGULOS GIRADOS REALES (GRADOS)
  float anguloMedidoM1 = (totalPulsosM1 / PULSOS_POR_REV) * 360.0f;
  float anguloMedidoM2 = (totalPulsosM2 / PULSOS_POR_REV) * 360.0f;

  // C. CÁLCULO DE ERRORES DE SEGUIMIENTO (CONSIGNA - MEDIDO)
  float errorM1 = anguloRef - anguloMedidoM1;
  float errorM2 = anguloRef - anguloMedidoM2;

  // D. CONTROL PI INTUITIVO (MOTOR 1)
  float P_M1 = Kp * errorM1;
  integralM1 += (Ki * errorM1 * dt_seg);
  integralM1 = constrain(integralM1, 0.0f, 255.0f);
  dutyM1 = (uint8_t)constrain(P_M1 + integralM1, 0.0f, 255.0f);

  // E. CONTROL PI INTUITIVO (MOTOR 2)
  float P_M2 = Kp * errorM2;
  integralM2 += (Ki * errorM2 * dt_seg);
  integralM2 = constrain(integralM2, 0.0f, 255.0f);
  dutyM2 = (uint8_t)constrain(P_M2 + integralM2, 0.0f, 255.0f);

  // Aplicar PWM
  analogWrite(PIN_ENA, dutyM1);
  analogWrite(PIN_ENB, dutyM2);

  // 3. REPORTE EN PUERTO SERIE Y OLED (CADA 200 MS)
  divisorImpresion++;
  if (divisorImpresion >= 10) {
    divisorImpresion = 0;

    Serial.print(F("Rampa: "));
    Serial.print((int)velocidadRampa);
    Serial.print(F(" deg/s | Ref: "));
    Serial.print(anguloRef, 0);

    Serial.print(F(" deg || M1 -> Err: "));
    Serial.print(errorM1, 1);
    Serial.print(F(" deg  Duty: "));
    Serial.print(dutyM1);

    Serial.print(F(" || M2 -> Err: "));
    Serial.print(errorM2, 1);
    Serial.print(F(" deg  Duty: "));
    Serial.print(dutyM2);

    Serial.print(F(" | dt: "));
    Serial.print(dt_ms);
    Serial.println(F(" ms"));

    if (oledDisponible) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.print(F("R4: Rampa "));
      display.print((int)velocidadRampa);
      display.println(F(" deg/s"));
      display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

      display.setCursor(0, 16);
      display.print(F("M1 Err: "));
      display.print(errorM1, 1);
      display.print(F(" deg | D:"));
      display.print(dutyM1);

      display.setCursor(0, 32);
      display.print(F("M2 Err: "));
      display.print(errorM2, 1);
      display.print(F(" deg | D:"));
      display.print(dutyM2);

      display.setCursor(0, 48);
      display.print(F("Ref: "));
      display.print(anguloRef, 0);
      display.print(F(" deg"));

      display.display();
    }
  }
}