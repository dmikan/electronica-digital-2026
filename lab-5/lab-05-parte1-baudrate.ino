/*
 * lab-05-parte1-baudrate.ino
 * Sesión 5 — Parte 1: Diagnóstico del enlace y caracterización temporal
 *
 * === QUÉ TRAE ESTE SKETCH Y QUÉ DEBE HACER EL GRUPO ===
 *
 * Trae RESUELTO el instrumento: la rutina que mide cuánto tarda la trama
 * en salir por el cable (micros() + Serial.flush()).
 *
 * NO trae resuelto el trabajo de la sesión:
 *   Reto 1 — encontrar por qué la comunicación entrega caracteres ilegibles
 *            y corregirlo, explicando el mecanismo.
 *   Reto 2 — correr esta medición a los cuatro baudrates de la tabla,
 *            calcular el tiempo TEÓRICO a partir de la trama UART y
 *            contrastar ambos valores.
 * La deducción de cuántos bits se transmiten por byte es del grupo.
 *
 * === RETO 1 — PUNTO DE PARTIDA ===
 *
 * El valor de BAUDRATE de abajo NO coincide con el que debe usar el
 * Monitor Serie. Carga el sketch, abre el Monitor a 9600 y observa.
 *
 * === RETO 2 — CÓMO SE USA ===
 *
 * Para cada baudrate de la tabla:
 *   1. cambiar BAUDRATE aquí abajo,
 *   2. poner el Monitor Serie en ese mismo valor,
 *   3. recargar el sketch,
 *   4. leer el tiempo medido.
 * La medición corre UNA vez al arranque; el programa se detiene después
 * de reportarla. Eso es intencional: el número queda en pantalla hasta
 * que cambies de baudrate.
 *
 * === CIRCUITO ===
 *   D2  — Botón con pull-down 10 kΩ (no se usa en esta parte)
 *   D13 — LED integrado: parpadea al iniciar como confirmación de arranque
 *
 * Autor: Ricardo Amézquita Orozco
 * Curso: Electrónica Digital 2016684 — 2026-2
 */

// === PINES ===
const int PIN_LED = 13;

// === BAUDRATE ===
// Velocidad de comunicación serial. El Monitor Serie debe estar en el
// MISMO valor. Reto 2: cambiar este valor y recargar, una vez por fila
// de la tabla.
long BAUDRATE = 4800;

// === TRAMA DE PRUEBA ===
// Se transmite siempre el mismo texto para que el número de bytes sea
// constante entre mediciones. Cuántos bytes se transmiten por trama NO
// se dice aquí: se deduce del texto y de lo que agrega println().
const char TEXTO_PRUEBA[] = "Hola, esto es una prueba de tiempo de transmision UART.";

// ============================================================
// CONFIGURACIÓN
// ============================================================
void setup() {

  pinMode(PIN_LED, OUTPUT);

  Serial.begin(BAUDRATE);

  // Parpadeo de confirmación: indica que el Arduino está funcionando.
  for (int i = 0; i < 3; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(200);
    digitalWrite(PIN_LED, LOW);
    delay(200);
  }

  Serial.println("=== Sesion 5 - Parte 1: baudrate y tiempos ===");
  Serial.println();
}

// ============================================================
// BUCLE PRINCIPAL
// ============================================================
void loop() {

  medirTiempoTransmision();

  // El programa se detiene a propósito: la medición corre una sola vez
  // y el resultado queda en pantalla. Para medir a otro baudrate hay que
  // cambiar BAUDRATE y recargar el sketch.
  while (true) {}
}

// ============================================================
// FUNCIÓN AUXILIAR: medición de tiempo de transmisión
// ============================================================
//
// Mide cuánto tiempo real tarda en salir la trama completa al baudrate
// configurado en BAUDRATE.
//
// Técnica:
//   1. marca el tiempo con micros() antes de println()
//   2. llama Serial.flush() para esperar a que los últimos bits salgan
//      físicamente por el cable, no solo del buffer
//   3. marca el tiempo después del flush
//   4. imprime el resultado
//
void medirTiempoTransmision() {
  unsigned long t0 = micros();
  Serial.println(TEXTO_PRUEBA);
  Serial.flush();  // espera hasta que el último bit haya salido
  unsigned long t1 = micros();

  unsigned long tiempoMedido_us = t1 - t0;

  Serial.print("Baudrate: ");
  Serial.print(BAUDRATE);
  Serial.print(" | Tiempo medido: ");
  Serial.print(tiempoMedido_us);
  Serial.println(" us");
}
