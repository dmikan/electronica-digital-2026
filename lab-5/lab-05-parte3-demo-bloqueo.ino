/*
 * lab-05-parte3-demo-bloqueo.ino
 * Sesión 5 — demo del problema: qué le hace delay() al canal de control
 *
 * ESTE SKETCH NO ES EL PUNTO DE PARTIDA DEL PROTOCOLO DEL GRUPO.
 * Es un experimento de diagnóstico: sirve para VER el bloqueo, nada más.
 *
 * === LO QUE HAY QUE OBSERVAR ===
 *
 *   1. Enviar "B" y, mientras el LED parpadea, enviar "S".
 *      La respuesta NO llega hasta que el parpadeo termina.
 *
 *   2. Enviar "B" y presionar el botón varias veces durante el parpadeo.
 *      Al terminar, "S" reporta MENOS pulsaciones de las que se hicieron.
 *
 * === PREGUNTA DEL RETO 4 ===
 *
 * ¿Resolvería attachInterrupt() (Sesión 2) los dos problemas?
 * Resolvería el conteo — una interrupción sí corre durante el delay() —
 * pero "S" seguiría sin responder: el delay() bloquea loop(), y el puerto
 * serie solo se lee desde ahí.
 *
 * === CIRCUITO ===
 *   D2  — Botón con pull-down 10 kΩ (HIGH = presionado)
 *   D13 — LED integrado
 *
 * Autor: Ricardo Amézquita Orozco
 * Curso: Electrónica Digital 2016684 — 2026-2
 */

const int PIN_BOTON = 2;
const int PIN_LED   = 13;

const int PARPADEOS = 5;          // cuántas veces parpadea con "B"

int  contadorPulsaciones = 0;
bool estadoAnteriorBoton = LOW;

// ============================================================
void setup() {
  pinMode(PIN_BOTON, INPUT);
  pinMode(PIN_LED, OUTPUT);

  Serial.begin(9600);
  Serial.println("=== Demo: bloqueo por delay() ===");
  Serial.println("B = parpadear (bloqueante)   S = pulsaciones");
}

// ============================================================
void loop() {
  atenderComando();
  detectarPulsaciones();
}

// ============================================================
// Lee UN carácter del puerto y actúa.
//
// Los comandos son de un solo carácter, así que no hace falta buffer ni
// parser: es lo mínimo para poder operar el demo desde el Monitor Serie.
// Se ignoran el retorno de carro y el salto de línea para que funcione
// con cualquier configuración de terminador del Monitor.
// ============================================================
void atenderComando() {
  if (Serial.available() == 0) return;

  char c = Serial.read();

  if (c == 'B') {
    parpadear(PARPADEOS);        // <-- llamada bloqueante
  } else if (c == 'S') {
    Serial.print("pulsaciones: ");
    Serial.println(contadorPulsaciones);
  }
  // Cualquier otro carácter se ignora ('\n', '\r', etc.).
}

// ============================================================
// Parpadea N veces. TARDA N segundos en volver, y mientras corre
// NO se ejecuta nada más: ni el puerto ni el botón.
//
// Esta es la función que hay que convertir a no bloqueante.
// ============================================================
void parpadear(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_LED, HIGH);
    delay(500);
    digitalWrite(PIN_LED, LOW);
    delay(500);
  }
}

// ============================================================
// Solo cuenta si loop() vuelve a ejecutarse. Durante el parpadeo
// bloqueante no se ejecuta, y las pulsaciones de ese lapso se pierden.
// ============================================================
void detectarPulsaciones() {
  bool estadoActual = digitalRead(PIN_BOTON);

  if (estadoActual == HIGH && estadoAnteriorBoton == LOW) {
    contadorPulsaciones++;
  }

  estadoAnteriorBoton = estadoActual;
}
