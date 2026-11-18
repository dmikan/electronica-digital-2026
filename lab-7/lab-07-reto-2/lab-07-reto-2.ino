/*
 * lab-07-parte2-motor-driver.ino
 * Sesión 7 — Parte 2: La velocidad medida (Reto 2)
 *
 * Conexión del Encoder:
 *   - VCC -> 5V del Arduino (¡Nunca a la fuente externa!)
 *   - GND -> GND común
 *   - Canal A -> Pin D2 (INT0)
 */

const uint8_t PIN_ENA   = 9;    // Salida PWM a ENA del L298N
const uint8_t PIN_IN1   = 7;    // Dirección IN1
const uint8_t PIN_IN2   = 8;    // Dirección IN2
const uint8_t PIN_ENC_A = 2;    // D2 = INT0 (Canal A del encoder)

// Definición del Duty Cycle actual a probar (0 a 255)
// Cambiar este valor para hacer el barrido (ej. 0, 80, 160, 255)
const uint8_t DUTY = 255;

// Constantes físicas del sistema
const float PULSOS_POR_REV = 700.0;
const unsigned long VENTANA_MS = 1000; // Ventana de medición de 1 segundo (1000 ms)

// Variables de interrupción (deben ser volatile)
volatile unsigned long contadorPulsos = 0;

// Control de tiempo para millis()
unsigned long tiempoAnterior = 0;

//Rutina de Servicio de Interrupción (ISR)
void contarPulso() {
  contadorPulsos++;
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  
  // Configuración del pin del encoder con interrupción en flanco de subida
  pinMode(PIN_ENC_A, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_A), contarPulso, RISING);

  // Sentido de giro: Avance
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, DUTY);

  Serial.println(F("--- Reto 2: Medición de Velocidad con Encoder ---"));
  Serial.print(F("Duty configurado: "));
  Serial.println(DUTY);
  Serial.println(F("Tiempo(ms)\tPulsos\tFrecuencia(Hz)\tRPM"));
}

void loop() {
  unsigned long tiempoActual = millis();

  // Verifica si ya transcurrió la ventana de tiempo fijada
  if (tiempoActual - tiempoAnterior >= VENTANA_MS) {
    // 1. Lectura atómica de los pulsos para evitar lecturas corruptas por la ISR
    noInterrupts();
    unsigned long pulsosEnVentana = contadorPulsos;
    contadorPulsos = 0; // Reinicia el contador para la siguiente ventana
    interrupts();

    // 2. Cálculos de frecuencia y velocidad
    // Frecuencia en Hz (pulsos por segundo)
    float frecuenciaHz = (pulsosEnVentana * 1000.0) / VENTANA_MS;
    
    // Cálculo de RPM: (Pulsos / 700) * (60 s / Ventana en s)
    float rpm = (pulsosEnVentana / PULSOS_POR_REV) * (60000.0 / VENTANA_MS);

    // 3. Imprimir reporte por el Monitor Serie
    Serial.print(tiempoActual);
    Serial.print(F("\t\t"));
    Serial.print(pulsosEnVentana);
    Serial.print(F("\t\t"));
    Serial.print(frecuenciaHz, 2);
    Serial.print(F("\t\t"));
    Serial.println(rpm, 2);

    // Actualiza la marca de tiempo para la siguiente ventana
    tiempoAnterior = tiempoActual;
  }
}