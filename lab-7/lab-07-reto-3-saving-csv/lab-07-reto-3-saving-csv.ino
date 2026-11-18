/*
 * lab-07-parte2-motor-driver.ino
 * Reto 3: Recepcion de Duty por Serial
 */

const uint8_t PIN_ENA   = 9;
const uint8_t PIN_IN1   = 7;
const uint8_t PIN_IN2   = 8;
const uint8_t PIN_ENC_A = 2;

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

  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, dutyActual);
}

void loop() {
  // Escuchar cambio de Duty desde Python
  if (Serial.available() > 0) {
    int nuevoDuty = Serial.parseInt();
    while (Serial.available() > 0) Serial.read();

    if (nuevoDuty >= 0 && nuevoDuty <= 255) {
      dutyActual = (uint8_t)nuevoDuty;
      analogWrite(PIN_ENA, dutyActual);
    }
  }

  // Enviar telemetría cada segundo
  unsigned long tiempoActual = millis();
  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    noInterrupts();
    unsigned long pulsosEnVentana = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    float frecuenciaHz = (pulsosEnVentana * 1000.0) / VENTANA_MS;
    float rpm = (pulsosEnVentana / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);

    // Formato tabulado: Duty \t Frecuencia \t RPM
    Serial.print(dutyActual);
    Serial.print(F("\t"));
    Serial.print(frecuenciaHz, 1);
    Serial.print(F("\t"));
    Serial.println(rpm, 2);

    tiempoAnterior = tiempoActual;
  }
}