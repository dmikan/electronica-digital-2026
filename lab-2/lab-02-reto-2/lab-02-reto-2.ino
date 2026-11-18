// ============================================================
// RETO 2: Conteo de eventos por interrupción (Hardware + ISR)
// ============================================================
// Mantiene el circuito de la Sesión 1:
//   - Botón en Pin 2 (INT0) con resistencia pull-down de 10 kΩ.
//   - LED Rojo en Pin 13 y LED Verde en Pin 12.
// ============================================================

// === DEFINICIÓN DE PINES ===
const int buttonPin = 2;   // Pin de entrada con interrupción externa (INT0)
const int ledRojo   = 13;  // Indicador visual
const int ledVerde  = 12;  // Indicador visual

// === VARIABLES COMPARTIDAS CON LA ISR ===
// 'volatile' evita que el compilador optimice/cachee la variable en registros
volatile unsigned int contadorPulsaciones = 0;
volatile unsigned long ultimoTiempoInterrupt = 0; // Para filtro de debouncing
const unsigned long tiempoDebounceMicros = 50000;  // 50 ms en microsegundos

// === RUTINA DE SERVICIO DE INTERRUPCIÓN (ISR) ===
void ISR_contarPulsacion() {
  unsigned long tiempoActual = micros();
  
  // Filtro Anti-Rebote (Debounce): ignora pulsos mecánicos falsos dentro de 50 ms
  if (tiempoActual - ultimoTiempoInterrupt > tiempoDebounceMicros) {
    contadorPulsaciones++;
    ultimoTiempoInterrupt = tiempoActual;
  }
}

void setup() {
  // Configuración de direcciones GPIO
  pinMode(buttonPin, INPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  // Estado inicial de actuadores (Botón liberado: Verde ON, Rojo OFF)
  digitalWrite(ledRojo, LOW);
  digitalWrite(ledVerde, HIGH);

  // Inicialización de la UART a 9600 baudios
  Serial.begin(9600);
  Serial.println("==========================================");
  Serial.println("Reto 2 Iniciado: Conteo por Interrupcion");
  Serial.println("==========================================");

  // Configuración de la Interrupción Externa en el Pin 2 (INT0)
  // RISING: detecta el flanco de subida (cuando se presiona el botón)
  attachInterrupt(digitalPinToInterrupt(buttonPin), ISR_contarPulsacion, RISING);
}

void loop() {
  // --- Lógica de Actuadores (Visualización de Estado) ---
  int estadoBoton = digitalRead(buttonPin);
  if (estadoBoton == HIGH) {
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledVerde, LOW);
  } else {
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, HIGH);
  }

  // --- Sección Crítica: Lectura segura de la variable 'volatile' ---
  noInterrupts(); // Pausa interrupciones para evitar lecturas corruptas de 16 bits
  unsigned int valorConteo = contadorPulsaciones;
  interrupts();   // Reactiva interrupciones inmediatamente

  // --- Reporte por Monitor Serie cada 500 ms ---
  static unsigned long ultimoReporte = 0;
  if (millis() - ultimoReporte >= 500) {
    ultimoReporte = millis();
    Serial.print("Pulsaciones acumuladas: ");
    Serial.println(valorConteo);
  }
}
