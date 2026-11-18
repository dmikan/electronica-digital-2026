/*
 * lab-08-reto3-pi-intuitivo.ino
 * Sesión 8 — Reto 3: Lazo PI a 100 RPM (Lógica PI Transparente e Intuitiva)
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

// CORRECCIÓN FÍSICA: Canales A asignados a tus interrupciones reales
const bool ENCODERS_CRUZADOS = true;
const uint8_t PIN_ENC_A1 = ENCODERS_CRUZADOS ? 3 : 2;
const uint8_t PIN_ENC_A2 = ENCODERS_CRUZADOS ? 2 : 3;

// --- PARÁMETROS DE CONTROL ---
const float PULSOS_POR_REV = 700.0f;
const unsigned long PERIODO_CONTROL_MS = 20;
const float CONSIGNA_RPM = 100.0f;

// Ganancias del Controlador PI
const float Kp = 1.2f;
const float Ki = 0.8f;

// --- VARIABLES GLOBALES ---
volatile long pulsosMotor1 = 0;
volatile long pulsosMotor2 = 0;

// Variables explícitas para el Término Integral (I)
float integralM1 = 0.0f;
float integralM2 = 0.0f;

// Filtro de RPM para lectura suave
float rpmFiltradaM1 = 0.0f;
float rpmFiltradaM2 = 0.0f;

uint8_t dutyM1 = 0;
uint8_t dutyM2 = 0;

// Detección de fallos por desconexión de encoder
unsigned long sinPulsosM1 = 0;
unsigned long sinPulsosM2 = 0;
bool fallo = false;

unsigned long tiempoAnterior = 0;
uint8_t divisorImpresion = 0;
bool oledDisponible = false;

void ISR_motor1() { pulsosMotor1++; }
void ISR_motor2() { pulsosMotor2++; }

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

  // Sentido de marcha inicial: Avance (FWD)
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
    display.println(F("Reto 3: Lazo PI"));
    display.println(F("Consigna: 100 RPM"));
    display.display();
    delay(1000);
  }

  Serial.println(F("=================================================="));
  Serial.println(F(" RETO 3: CONTROL PI INTUITIVO (100 RPM @ 20 ms)  "));
  Serial.println(F("=================================================="));

  tiempoAnterior = millis();
}

void loop() {
  unsigned long tiempoActual = millis();
  unsigned long dt_ms = tiempoActual - tiempoAnterior;

  // Garantizar el periodo de control exacto a 20 ms
  if (dt_ms < PERIODO_CONTROL_MS) return;

  tiempoAnterior = tiempoActual;
  float dt_seg = dt_ms / 1000.0f; // dt en segundos (0.02s)

  // 1. Lectura atómica de pulsos acumulados
  noInterrupts();
  long rawPulsosM1 = pulsosMotor1;
  long rawPulsosM2 = pulsosMotor2;
  pulsosMotor1 = 0;
  pulsosMotor2 = 0;
  interrupts();

  // 2. Cálculo de RPM instantáneas
  float rpmMedidaM1 = (rawPulsosM1 * 60000.0f) / (PULSOS_POR_REV * dt_ms);
  float rpmMedidaM2 = (rawPulsosM2 * 60000.0f) / (PULSOS_POR_REV * dt_ms);

  // 3. Filtro de paso bajo (Suavizado de lectura)
  rpmFiltradaM1 += 0.25f * (rpmMedidaM1 - rpmFiltradaM1);
  rpmFiltradaM2 += 0.25f * (rpmMedidaM2 - rpmFiltradaM2);

  // 4. Protección por falta de pulsos (Evita que el motor se queme si el encoder falla)
  sinPulsosM1 = (dutyM1 >= 100 && rawPulsosM1 == 0) ? sinPulsosM1 + dt_ms : 0;
  sinPulsosM2 = (dutyM2 >= 100 && rawPulsosM2 == 0) ? sinPulsosM2 + dt_ms : 0;

  if (!fallo && (sinPulsosM1 >= 1000 || sinPulsosM2 >= 1000)) {
    fallo = true;
    analogWrite(PIN_ENA, 0);
    analogWrite(PIN_ENB, 0);
    Serial.println(F("¡FALLO DETECTADO! Sin pulsos en encoder. Salidas apagas."));
  }

  if (fallo) {
    dutyM1 = 0;
    dutyM2 = 0;
  } else {
    
    // ========================================================
    // --- CONTROLADOR PI INTUITIVO Y DIRECTO (MOTOR 1) ---
    // ========================================================
    float errorM1 = CONSIGNA_RPM - rpmFiltradaM1;
    
    // Término Proporcional: P = Kp * error
    float P_M1 = Kp * errorM1;
    
    // Acumulación del Término Integral: I = I + (Ki * error * dt)
    integralM1 += (Ki * errorM1 * dt_seg);
    integralM1 = constrain(integralM1, 0.0f, 255.0f); // Anti-windup intuitivo (0 a 255)
    
    // Salida PI = P + I
    float salidaM1 = P_M1 + integralM1;
    dutyM1 = (uint8_t)constrain(salidaM1, 0.0f, 255.0f);


    // ========================================================
    // --- CONTROLADOR PI INTUITIVO Y DIRECTO (MOTOR 2) ---
    // ========================================================
    float errorM2 = CONSIGNA_RPM - rpmFiltradaM2;
    
    // Término Proporcional: P = Kp * error
    float P_M2 = Kp * errorM2;
    
    // Acumulación del Término Integral: I = I + (Ki * error * dt)
    integralM2 += (Ki * errorM2 * dt_seg);
    integralM2 = constrain(integralM2, 0.0f, 255.0f); // Anti-windup intuitivo (0 a 255)
    
    // Salida PI = P + I
    float salidaM2 = P_M2 + integralM2;
    dutyM2 = (uint8_t)constrain(salidaM2, 0.0f, 255.0f);
  }

  // Aplicar seña PWM corregida a los motores
  analogWrite(PIN_ENA, dutyM1);
  analogWrite(PIN_ENB, dutyM2);

  // 5. REPORTE EN PUERTO SERIE Y OLED (CADA 200 MS)
  divisorImpresion++;
  if (divisorImpresion >= 10) {
    divisorImpresion = 0;

    float errorM1_print = CONSIGNA_RPM - rpmFiltradaM1;
    float errorM2_print = CONSIGNA_RPM - rpmFiltradaM2;

    Serial.print(F("Ref: 100 RPM | M1 -> RPM: "));
    Serial.print(rpmFiltradaM1, 1);
    Serial.print(F("  Duty: "));
    Serial.print(dutyM1);
    Serial.print(F("  Err: "));
    Serial.print(errorM1_print, 1);

    Serial.print(F(" || M2 -> RPM: "));
    Serial.print(rpmFiltradaM2, 1);
    Serial.print(F("  Duty: "));
    Serial.print(dutyM2);
    Serial.print(F("  Err: "));
    Serial.print(errorM2_print, 1);

    Serial.print(F(" | dt: "));
    Serial.print(dt_ms);
    Serial.println(F(" ms"));

    if (oledDisponible) {
      display.clearDisplay();
      display.setCursor(0, 0);
      display.println(fallo ? F("FALLO ENCODER") : F("Reto 3: Lazo PI RPM"));
      display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

      display.setCursor(0, 16);
      display.print(F("1: "));
      display.print(rpmFiltradaM1, 1);
      display.print(F(" RPM | D:"));
      display.print(dutyM1);

      display.setCursor(0, 32);
      display.print(F("2: "));
      display.print(rpmFiltradaM2, 1);
      display.print(F(" RPM | D:"));
      display.print(dutyM2);

      display.setCursor(0, 48);
      display.print(F("dt: "));
      display.print(dt_ms);
      display.print(F(" ms | Ref:100"));

      display.display();
    }
  }
}