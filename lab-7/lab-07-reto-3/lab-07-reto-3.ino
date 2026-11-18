/*
 * lab-07-parte2-motor-driver.ino
 * Sesión 7 — Parte 2: Caracterización Duty vs RPM (Reto 3)
 *
 * Permite cambiar el ciclo de trabajo (duty) desde el Monitor Serie 
 * enviando un número entre 0 y 255 (o ingresando valores predefinidos).
 */

const uint8_t PIN_ENA   = 9;    // Salida PWM a ENA del L298N
const uint8_t PIN_IN1   = 7;    // Dirección IN1
const uint8_t PIN_IN2   = 8;    // Dirección IN2
const uint8_t PIN_ENC_A = 2;    // D2 = INT0 (Canal A del encoder)

const float PULSOS_POR_REV = 700.0;
const unsigned long VENTANA_MS = 1000; // Ventana de 1 segundo

volatile unsigned long contadorPulsos = 0;
unsigned long tiempoAnterior = 0;
uint8_t dutyActual = 0;

void contarPulso() {
  contadorPulsos++;
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), contarPulso, RISING);

  // Sentido de giro inicial (Avance)
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, dutyActual);

  Serial.println(F("=================================================="));
  Serial.println(F("  Reto 3: Caracterizacion de la Respuesta Duty-RPM  "));
  Serial.println(F("=================================================="));
  Serial.println(F("Instrucciones: Envia un valor de Duty (0 a 255) por el Monitor Serie."));
  Serial.println(F("--------------------------------------------------"));
  Serial.println(F("Duty\tPulsos/s\tRPM Medidas"));
}

void loop() {
  // Lectura de nuevo Duty enviado por el Monitor Serie
  if (Serial.available() > 0) {
    int nuevoDuty = Serial.parseInt();
    // Limpiar el buffer por si quedan saltos de linea
    while (Serial.available() > 0) Serial.read();

    if (nuevoDuty >= 0 && nuevoDuty <= 255) {
      dutyActual = (uint8_t)nuevoDuty;
      analogWrite(PIN_ENA, dutyActual);
      Serial.print(F(">>> Cambio de Duty a: "));
      Serial.println(dutyActual);
    }
  }

  unsigned long tiempoActual = millis();

  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    // Lectura atómica dentro de la sección crítica
    noInterrupts();
    unsigned long pulsosEnVentana = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    // Cálculos
    float frecuenciaHz = (pulsosEnVentana * 1000.0) / VENTANA_MS;
    float rpm = (pulsosEnVentana / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);

    // Salida tabulada para facilitar el registro
    Serial.print(dutyActual);
    Serial.print(F("\t"));
    Serial.print(frecuenciaHz, 1);
    Serial.print(F("\t\t"));
    Serial.println(rpm, 2);

    tiempoAnterior = tiempoActual;
  }
}