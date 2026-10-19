/*
 * Demostración de la base de tiempo: millis() y micros() en paralelo.
 * Sesión 2 — worked example de la presentación de teoría.
 *
 * Propósito: mostrar que millis() (ms) y micros() (μs) son dos contadores
 * que avanzan desde el encendido/reset mientras el programa se ejecuta.
 * Se usa un delay(1000) como "sueño" del programa: durante ese segundo el
 * programa no hace nada, pero los contadores siguen avanzando. Al despertar,
 * ambos han avanzado ~1 s, y micros() ≈ 1000 × millis() (relación de unidades).
 *
 * Esto deja ver dos cosas que se explotan en la sesión:
 *   1. La resolución distinta: millis() en ms (1 ms), micros() en μs (4 μs).
 *   2. delay() BLOQUEA: mientras espera, el programa no puede detectar ningún
 *      evento. Esa limitación motiva las interrupciones (más adelante).
 *
 * No requiere circuito: solo la placa conectada por USB y el Monitor Serie.
 */

// Intervalo de "sueño" (ms) entre muestras de la base de tiempo.
const unsigned long ESPERA_MS = 1000;

void setup() {
  Serial.begin(9600);            // UART a 9600 baudios (igual que el monitor)
  Serial.println("millis [ms]   micros [us]");   // encabezado de columnas
}

void loop() {
  // Marcar el instante ANTES del sueño, con ambos contadores.
  unsigned long m0 = millis();
  unsigned long u0 = micros();

  delay(ESPERA_MS);              // "sleep": el programa se detiene ESPERA_MS

  // Marcar el instante DESPUÉS del sueño.
  unsigned long m1 = millis();
  unsigned long u1 = micros();

  // Intervalos transcurridos durante el sueño.
  unsigned long d_ms = m1 - m0;     // ≈ 1000 ms   (la resta maneja overflow)
  unsigned long d_us = u1 - u0;     // ≈ 1 000 000 μs

  // Imprimir el AVANCE (la diferencia) de ambos en paralelo: deja ver que
  // micros ≈ 1000 × millis (d_us ≈ 1000 × d_ms).
  Serial.print(d_ms);
  Serial.print("        ");
  Serial.println(d_us);
}
