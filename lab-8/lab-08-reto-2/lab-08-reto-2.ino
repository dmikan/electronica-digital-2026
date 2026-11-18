/*
 * lab-08-reto2-consigna-terminal.ino
 * Sesión 8 — Reto 2: La consigna por el terminal (Lazo Abierto + Telemetría a demanda)
 * 
 * Conexión del Módulo L298N:
 *   - ENA  -> Pin D9  (PWM Motor 1) [¡Retirar Jumper ENA!]
 *   - ENB  -> Pin D10 (PWM Motor 2)
 *   - IN1  -> Pin D4  | IN2 -> Pin D5 (Motor 1 - Izquierdo)
 *   - IN3  -> Pin D6  | IN4 -> Pin D7 (Motor 2 - Derecho)
 *
 * Conexión Encoders (VCC 5V Arduino, GND común):
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

// --- ASIGNACIÓN DE PINES ---
const uint8_t PIN_ENA  = 9;
const uint8_t PIN_ENB  = 10;
const uint8_t PIN_IN1  = 4;
const uint8_t PIN_IN2  = 5;
const uint8_t PIN_IN3  = 6;
const uint8_t PIN_IN4  = 7;

const uint8_t PIN_ENC_A1 = 2;  // INT0
const uint8_t PIN_ENC_A2 = 3;  // INT1

// --- PARÁMETROS DE MEDICIÓN ---
const float PULSOS_POR_REV = 700.0;
const unsigned long INTERVALO_MS = 200; // Muestreo no bloqueante de velocidad

// --- VARIABLES GLOBALES DE CONSIGNAS INDEPENDIENTES ---
uint8_t dutyMotor1 = 0;
uint8_t dutyMotor2 = 0;

// --- VARIABLES GLOBALES ISR ---
volatile long pulsosMotor1 = 0;
volatile long pulsosMotor2 = 0;

void ISR_motor1() { pulsosMotor1++; }
void ISR_motor2() { pulsosMotor2++; }

// Variables de tiempo y telemetría
unsigned long tiempoAnterior = 0;
float rpmM1_medida = 0.0;
float rpmM2_medida = 0.0;
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
  pinMode(PIN_ENC_A2, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A1), ISR_motor1, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A2), ISR_motor2, RISING);

  // Inicialmente ambos motores detenidos
  analogWrite(PIN_ENA, 0);
  analogWrite(PIN_ENB, 0);

  if (!display.begin(SSD1306_SWITCHCAPVCC, DIRECCION_OLED)) {
    oledDisponible = false;
  } else {
    oledDisponible = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("Sesion 8 - Reto 2"));
    display.println(F("Consignas Terminal"));
    display.display();
    delay(1000);
  }

  Serial.println(F("=================================================="));
  Serial.println(F("   Reto 2: Consigna Independiente y Lectura      "));
  Serial.println(F("=================================================="));
  Serial.println(F("FORMATO DE COMANDOS DE CONSIGNA:"));
  Serial.println(F("  '1 f 150' -> M1: Avance, Duty 150"));
  Serial.println(F("  '1 r 120' -> M1: Reversa, Duty 120"));
  Serial.println(F("  '1 s'     -> M1: Detenido"));
  Serial.println(F("  '2 f 200' -> M2: Avance, Duty 200"));
  Serial.println(F("  '2 r 90'  -> M2: Reversa, Duty 90"));
  Serial.println(F("  '2 s'     -> M2: Detenido"));
  Serial.println(F("COMANDO DE LECTURA (Telemetria a demanda):"));
  Serial.println(F("  'p'       -> Imprimir velocidades medidas reales"));
  Serial.println(F("--------------------------------------------------"));

  tiempoAnterior = millis();
}

void loop() {
  // 1. RECEPCIÓN DE CONSIGNAS Y LECTURA A DEMANDA (ROBUSTO Y DIRECTO)
  if (Serial.available() > 0) {
    String comando = Serial.readStringUntil('\n');
    comando.trim(); // Eliminar espacios iniciales, finales, \r y \n

    if (comando.length() > 0) {
      char selector = comando.charAt(0);

      // Comando de Lectura: 'p' o 'P'
      if (selector == 'p' || selector == 'P') {
        reportarLecturaConsola();
      }
      // Consigna Motor 1: '1'
      else if (selector == '1') {
        // Buscar la letra de dirección ('f', 'r' o 's') omitiendo espacios
        int posLetra = -1;
        for (int i = 1; i < comando.length(); i++) {
          char c = comando.charAt(i);
          if (c == 'f' || c == 'F' || c == 'r' || c == 'R' || c == 's' || c == 'S') {
            posLetra = i;
            break;
          }
        }

        if (posLetra != -1) {
          char dir = comando.charAt(posLetra);
          String numStr = comando.substring(posLetra + 1);
          numStr.trim();
          uint8_t duty = (uint8_t)numStr.toInt();

          if (dir == 'f' || dir == 'F') {
            dutyMotor1 = duty;
            digitalWrite(PIN_IN1, HIGH);
            digitalWrite(PIN_IN2, LOW);
            analogWrite(PIN_ENA, dutyMotor1);
            Serial.print(F(">>> CONSIGNA M1 -> AVANCE (CW) | Duty: "));
            Serial.println(dutyMotor1);
          } else if (dir == 'r' || dir == 'R') {
            dutyMotor1 = duty;
            digitalWrite(PIN_IN1, LOW);
            digitalWrite(PIN_IN2, HIGH);
            analogWrite(PIN_ENA, dutyMotor1);
            Serial.print(F(">>> CONSIGNA M1 -> REVERSA (CCW) | Duty: "));
            Serial.println(dutyMotor1);
          } else if (dir == 's' || dir == 'S') {
            dutyMotor1 = 0;
            analogWrite(PIN_ENA, 0);
            Serial.println(F(">>> CONSIGNA M1 -> DETENIDO"));
          }
        }
      }
      // Consigna Motor 2: '2'
      else if (selector == '2') {
        int posLetra = -1;
        for (int i = 1; i < comando.length(); i++) {
          char c = comando.charAt(i);
          if (c == 'f' || c == 'F' || c == 'r' || c == 'R' || c == 's' || c == 'S') {
            posLetra = i;
            break;
          }
        }

        if (posLetra != -1) {
          char dir = comando.charAt(posLetra);
          String numStr = comando.substring(posLetra + 1);
          numStr.trim();
          uint8_t duty = (uint8_t)numStr.toInt();

          if (dir == 'f' || dir == 'F') {
            dutyMotor2 = duty;
            digitalWrite(PIN_IN3, HIGH);
            digitalWrite(PIN_IN4, LOW);
            analogWrite(PIN_ENB, dutyMotor2);
            Serial.print(F(">>> CONSIGNA M2 -> AVANCE (CW) | Duty: "));
            Serial.println(dutyMotor2);
          } else if (dir == 'r' || dir == 'R') {
            dutyMotor2 = duty;
            digitalWrite(PIN_IN3, LOW);
            digitalWrite(PIN_IN4, HIGH);
            analogWrite(PIN_ENB, dutyMotor2);
            Serial.print(F(">>> CONSIGNA M2 -> REVERSA (CCW) | Duty: "));
            Serial.println(dutyMotor2);
          } else if (dir == 's' || dir == 'S') {
            dutyMotor2 = 0;
            analogWrite(PIN_ENB, 0);
            Serial.println(F(">>> CONSIGNA M2 -> DETENIDO"));
          }
        }
      }
    }
  }

  // 2. MUESTREO Y RENDERIZADO EN PANTALLA OLED
  unsigned long tiempoActual = millis();
  unsigned long dt = tiempoActual - tiempoAnterior;

  if (dt >= INTERVALO_MS) {
    tiempoAnterior = tiempoActual;

    noInterrupts();
    long rawPulsosM1 = pulsosMotor1;
    long rawPulsosM2 = pulsosMotor2;
    pulsosMotor1 = 0;
    pulsosMotor2 = 0;
    interrupts();

    rpmM1_medida = (rawPulsosM1 / PULSOS_POR_REV) * (60000.0 / (float)dt);
    rpmM2_medida = (rawPulsosM2 / PULSOS_POR_REV) * (60000.0 / (float)dt);

    if (oledDisponible) {
      display.clearDisplay();
      display.setTextSize(1);
      display.setCursor(0, 0);
      display.println(F("  Reto 2: Consignas  "));
      display.drawFastHLine(0, 10, 128, SSD1306_WHITE);

      display.setCursor(0, 16);
      display.print(F("1 Duty:"));
      display.print(dutyMotor1);
      display.print(F(" | "));
      display.print(rpmM2_medida, 1);
      display.print(F("RPM"));

      display.setCursor(0, 32);
      display.print(F("2 Duty:"));
      display.print(dutyMotor2);
      display.print(F(" | "));
      display.print(rpmM1_medida, 1);
      display.print(F("RPM"));

      display.setCursor(0, 48);
      //display.println(F("Enviar 'p' para leer"));

      display.display();
    }
  }
}

// Función auxiliar para emitir la lectura medida a demanda por el Monitor Serie[cite: 15]
void reportarLecturaConsola() {
  String sentidoM1 = (digitalRead(PIN_IN1) && !digitalRead(PIN_IN2)) ? "FWD" : ((!digitalRead(PIN_IN1) && digitalRead(PIN_IN2)) ? "REV" : "STOP");
  String sentidoM2 = (digitalRead(PIN_IN3) && !digitalRead(PIN_IN4)) ? "FWD" : ((!digitalRead(PIN_IN3) && digitalRead(PIN_IN4)) ? "REV" : "STOP");

  Serial.println(F("\n--- REPORTE DE VELOCIDAD MEDIDA ---"));
  Serial.print(F("Motor 1 (Izquierdo) -> Consigna PWM: "));
  Serial.print(dutyMotor1);
  Serial.print(F(" | Sentido: "));
  Serial.print(sentidoM1);
  Serial.print(F(" | Velocidad Medida: "));
  Serial.print(rpmM1_medida, 1);
  Serial.println(F(" RPM"));

  Serial.print(F("Motor 2 (Derecho)   -> Consigna PWM: "));
  Serial.print(dutyMotor2);
  Serial.print(F(" | Sentido: "));
  Serial.print(sentidoM2);
  Serial.print(F(" | Velocidad Medida: "));
  Serial.print(rpmM2_medida, 1);
  Serial.println(F(" RPM"));
  Serial.println(F("-----------------------------------\n"));
}