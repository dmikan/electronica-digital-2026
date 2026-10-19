/*
 * Demostración de interrupción externa: conteo de flancos de un botón.
 * Sesión 2 — worked example de la presentación de teoría.
 *
 * Propósito: mostrar que una interrupción RISING cuenta CADA flanco físico
 * del botón, con resolución de microsegundos e independiente del loop().
 *
 * Circuito requerido (el mismo de la Sesión 1):
 *   - Botón en D2 (INT0) con resistencia pull-down de 10 kΩ a GND.
 *   - Al presionar, el pin pasa a HIGH -> se genera un flanco RISING.
 *
 * Qué se observa: al presionar UNA vez y soltar, el contador suele
 * incrementar MÁS de uno. No es un error del programa: cada rebote mecánico
 * del contacto (una ráfaga de transiciones en ~1-10 ms) genera su propia
 * interrupción. Con polling y delay() ese fenómeno quedaba por debajo de la
 * resolución del muestreo; con interrupción se hace visible.
 */

// Pin del botón en una entrada de interrupción externa del Arduino Uno.
const int PIN_BOTON = 2;        // D2 = INT0

// Contador modificado dentro de la ISR: obligatorio usar volatile.
volatile unsigned int conteo = 0;

void contar() {
  // ISR: lo más breve posible. Solo incrementa el contador.
  conteo++;
}

void setup() {
  Serial.begin(9600);
  pinMode(PIN_BOTON, INPUT);            // botón con pull-down externo (S1)
  // RISING: la ISR se ejecuta en cada flanco ascendente (al presionar).
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON), contar, RISING);
  Serial.println("Presiona el boton varias veces. Cada pulsacion dispara");
  Serial.println("una ISR que incrementa 'conteo'. Observa cuanto sube cada");
  Serial.println("vez. Resumen cada 2 s:");
}

unsigned long ultimoResumen = 0;

void loop() {
  // Cada 2 s reporta el valor acumulado del contador (no bloquea la ISR).
  if (millis() - ultimoResumen >= 2000) {
    ultimoResumen = millis();
    // Suspende las interrupciones por un instante para leer el contador con
    // seguridad; las ISR se retoman después de la lectura.
    noInterrupts();
    unsigned int valor = conteo;
    interrupts();
    Serial.print("conteo acumulado = ");
    Serial.println(valor);
  }
}
