/*
 * lab-08-reto1-formato-completo.ino
 * Sesión 8 — Reto 1: Las dos ruedas, medidas (Con Formato Completo de Consola y OLED)
 * 
 * Conexiones L298N:
 *   - ENA  -> Pin D9 (PWM M1) [¡Retirar Jumper ENA!]
 *   - ENB  -> Pin D10 (PWM M2)
 *   - IN1  -> Pin D4  | IN2 -> Pin D5 (Motor 1 - Izquierdo)
 *   - IN3  -> Pin D6  | IN4 -> Pin D7 (Motor 2 - Derecho)
 *
 * Conexión Encoders (VCC 5V, GND común):
 *   - Encoder 1: Canal A -> Pin D2 (INT0), Canal B -> Pin D11
 *   - Encoder 2: Canal A -> Pin D3 (INT1), Canal B -> Pin D12
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA  64
#define DIRECCION_OLED 0x3C

Adafruit_SSD1306 display(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);

// --- ASIGNACIÓN DE PINES ---
const uint8_t PIN_ENA  = 9;
const uint8_t PIN_ENB  = 10;
const uint8_t PIN_IN1  = 4;
const uint8_t PIN_IN2  = 5;
const uint8_t PIN_IN3  = 6;
const uint8_t PIN_IN4  = 7;

const uint8_t PIN_ENC_A1 = 2;  // INT0 (Motor 1)
const uint8_t PIN_ENC_B1 = 11; // Canal B (Motor 1)
const uint8_t PIN_ENC_A2 = 3;  // INT1 (Motor 2)
const uint8_t PIN_ENC_B2 = 12; // Canal B (Motor 2)

// --- CONFIGURACIÓN DE PARÁMETROS ---
const float PULSOS_POR_REV = 700.0;  // 700.0 PPR para RISING
const unsigned long VENTANA_MS = 20; // Ventana interna de 20 ms[cite: 8]

uint8_t dutyActual = 150; // Duty PWM inicial

// --- VARIABLES GLOBALES DE ISR (Volatile) ---
volatile long pulsosMotor1 = 0;
volatile long pulsosMotor2 = 0;

void ISR_motor1() {
  if (!digitalRead(PIN_ENC_B1)) {
    pulsosMotor1++;
  } else {
    pulsosMotor1--;
  }
}

void ISR_motor2() {
  if (!digitalRead(PIN_ENC_B2)) {
    pulsosMotor2++;
  } else {
    pulsosMotor2--;
  }
}

// Variables de tiempo y filtrado
unsigned long tiempoAnterior = 0;
long ultimoConteoM1 = 0;
long ultimoConteoM2 = 0;
uint8_t divisorImpresion = 0;
bool oledDisponible = false;

void setup() {
  Serial.begin(115200); // 115200 baudios requeridos[cite: 8]

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_ENB, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT);
  pinMode(PIN_IN4, OUTPUT);

  pinMode(PIN_ENC_A1, INPUT_PULLUP);
  pinMode(PIN_ENC_B1, INPUT_PULLUP);
  pinMode(PIN_ENC_A2, INPUT_PULLUP);
  pinMode(PIN_ENC_B2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A1), ISR_motor1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A2), ISR_motor2, RISING);

  // Sentido inicial: Ambos en Avance (FWD)
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
    display.println(F("Dos Ruedas Medidas"));
    display.display();
    delay(1000);
  }

  // === IMPRESIÓN DEL MENÚ INICIAL EN CONSOLA SERIE ===
  Serial.println(F("========================================================================================="));
  Serial.println(F(" Reto 1: Medicion de Velocidad, Vueltas y Sentido en 2 Motores                           "));
  Serial.println(F("========================================================================================="));
  Serial.println(F("Comandos por Monitor Serie:"));
  Serial.println(F("  'f' -> Motor 1: Avance (IN1=1, IN2=0)   |  'd' -> Motor 2: Avance (IN3=1, IN4=0)"));
  Serial.println(F("  'r' -> Motor 1: Reversa (IN1=0, IN2=1)  |  'e' -> Motor 2: Reversa (IN3=0, IN4=1)"));
  Serial.println(F("  'x' -> Motor 1: FRENO ACTIVO (IN1=1,IN2=1)|  'c' -> Motor 2: FRENO ACTIVO (IN3=1,IN4=1)"));
  Serial.println(F("  's' -> Motor 1: Detener libre (Duty 0)  |  'a' -> Motor 2: Detener libre (Duty 0)"));
  Serial.println(F("  ---------------------------------------------------------------------------------------"));
  Serial.println(F("  'q' -> FRENO ACTIVO en AMBOS motores"));
  Serial.println(F("  '0' -> Reiniciar contadores de pulsos a cero"));
  Serial.println(F("  [0-255] -> Cambiar ciclo de trabajo PWM de ambos motores"));
  Serial.println(F("-----------------------------------------------------------------------------------------"));
}

void loop() {
  // 1. LECTURA DE COMANDOS POR MONITOR SERIE
  if (Serial.available() > 0) {
    char c = Serial.peek();

    // Comandos M1
    if (c == 'f' || c == 'F') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, LOW);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: Dirección ordenada AVANCE (CW)"));
    } else if (c == 'r' || c == 'R') {
      Serial.read();
      digitalWrite(PIN_IN1, LOW);
      digitalWrite(PIN_IN2, HIGH);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: Dirección ordenada REVERSA (CCW)"));
    } else if (c == 'x' || c == 'X') {
      Serial.read();
      digitalWrite(PIN_IN1, HIGH);
      digitalWrite(PIN_IN2, HIGH);
      analogWrite(PIN_ENA, dutyActual);
      Serial.println(F(">>> M1: FRENO ACTIVO (Brake)"));
    } else if (c == 's' || c == 'S') {
      Serial.read();
      analogWrite(PIN_ENA, 0);
      Serial.println(F(">>> M1: Detenido libre (Coast)"));
    }

    // Comandos M2
    else if (c == 'd' || c == 'D') {
      Serial.read();
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, LOW);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: Dirección ordenada AVANCE (CW)"));
    } else if (c == 'e' || c == 'E') {
      Serial.read();
      digitalWrite(PIN_IN3, LOW);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: Dirección ordenada REVERSA (CCW)"));
    } else if (c == 'c' || c == 'C') {
      Serial.read();
      digitalWrite(PIN_IN3, HIGH);
      digitalWrite(PIN_IN4, HIGH);
      analogWrite(PIN_ENB, dutyActual);
      Serial.println(F(">>> M2: FRENO ACTIVO (Brake)"));
    } else if (c == 'a' || c == 'A') {
      Serial.read();
      analogWrite(PIN_ENB, 0);
      Serial.println(F(">>> M2: Detenido libre (Coast)"));
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
      Serial.println(F(">>> FRENO ACTIVO EN AMBOS MOTORES"));
    } else if (c == '0') {
      Serial.read();
      noInterrupts();
      pulsosMotor1 = 0;
      pulsosMotor2 = 0;
      interrupts();
      Serial.println(F(">>> Contadores netos reiniciados a 0"));
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

  // 2. MUESTREO Y CÁLCULOS A 20 MS[cite: 8]
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    tiempoAnterior = tiempoActual;

    noInterrupts(); // Sección crítica[cite: 8]
    long pulsosM1 = pulsosMotor1;
    long pulsosM2 = pulsosMotor2;
    interrupts();

    long deltaM1 = pulsosM1 - ultimoConteoM1;
    long deltaM2 = pulsosM2 - ultimoConteoM2;
    ultimoConteoM1 = pulsosM1;
    ultimoConteoM2 = pulsosM2;

    float rpmM1 = (abs(deltaM1) / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);
    float rpmM2 = (abs(deltaM2) / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);
    
    // Cálculo de vueltas netas acumuladas
    float vueltasM1 = (float)pulsosM1 / PULSOS_POR_REV;
    float vueltasM2 = (float)pulsosM2 / PULSOS_POR_REV;

    // Determinar sentido por pines de control H
    String sentidoM1, sentidoM2;

    if (digitalRead(PIN_IN1) == HIGH && digitalRead(PIN_IN2) == HIGH) sentidoM1 = "BRAKE";
    else if (digitalRead(PIN_IN1) == HIGH && digitalRead(PIN_IN2) == LOW && rpmM1 > 1.0) sentidoM1 = "AVANCE (+)";
    else if (digitalRead(PIN_IN1) == LOW && digitalRead(PIN_IN2) == HIGH && rpmM1 > 1.0) sentidoM1 = "REVERSA (-)";
    else sentidoM1 = "QUIETO (0)";

    if (digitalRead(PIN_IN3) == HIGH && digitalRead(PIN_IN4) == HIGH) sentidoM2 = "BRAKE";
    else if (digitalRead(PIN_IN3) == HIGH && digitalRead(PIN_IN4) == LOW && rpmM2 > 1.0) sentidoM2 = "AVANCE (+)";
    else if (digitalRead(PIN_IN3) == LOW && digitalRead(PIN_IN4) == HIGH && rpmM2 > 1.0) sentidoM2 = "REVERSA (-)";
    else sentidoM2 = "QUIETO (0)";

    // 3. RENDERIZADO VISUAL A 240 MS
    divisorImpresion++;
    if (divisorImpresion >= 12) {
      divisorImpresion = 0;

      // === FORMATO COMPLETO EN EL MONITOR SERIE (COMO LA IMAGEN) ===[cite: 11]
      Serial.print(F("M1 -> Conteo: "));
      Serial.print(pulsosM1);
      Serial.print(F("\tVueltas: "));
      Serial.print(vueltasM1, 2);
      Serial.print(F("\tSentido: "));
      Serial.print(sentidoM1);
      Serial.print(F("\tRPM: "));
      Serial.print(rpmM1, 1);

      Serial.print(F("  ||  M2 -> Conteo: "));
      Serial.print(pulsosM2);
      Serial.print(F("\tVueltas: "));
      Serial.print(vueltasM2, 2);
      Serial.print(F("\tSentido: "));
      Serial.print(sentidoM2);
      Serial.print(F("\tRPM: "));
      Serial.println(rpmM2, 1);

      // === RENDERIZADO EN LA PANTALLA OLED ===
      if (oledDisponible) {
        display.clearDisplay();
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println(F(" Reto 1: Dos Ruedas "));
        display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

        display.setCursor(0, 16);
        //display.print(F("M1: "));
        display.print(rpmM1, 1);
        display.print(F(" RPM | "));
        display.println(sentidoM1);

        display.setCursor(0, 32);
        //display.print(F("M2: "));
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
}