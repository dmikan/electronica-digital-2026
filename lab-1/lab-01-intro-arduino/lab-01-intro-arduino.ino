// ============================================================
// LABORATORIO 1: Introducción a Arduino y Señales Digitales
// ============================================================
// Sistema de entrada/salida digital mínimo que materializa el
// paradigma sensor -> procesador -> actuador:
//   SENSOR     botón pulsador (entrada digital, pull-down)
//   PROCESADOR Arduino Uno (lógica condicional)
//   ACTUADOR   dos LEDs con estados mutuamente excluyentes
// El Monitor Serie actúa como instrumento de observación del
// estado interno del programa.
//
// Comportamiento esperado:
//   botón presionado -> LED rojo ENCENDIDO, LED verde apagado
//   botón liberado   -> LED verde ENCENDIDO, LED rojo apagado
// ============================================================

// === DEFINICIÓN DE PINES ===
// Los números de pin son etiquetas lógicas del microcontrolador
// (ATmega328P); el pin 13 es además el LED de a bordo (L) de la
// placa Arduino Uno.
const int buttonPin = 2;   // Pin de la entrada digital (botón)
const int ledRojo   = 13;  // LED rojo: indica botón presionado
const int ledVerde  = 12;  // LED verde: indica botón liberado

void setup() {
  // === CONFIGURACIÓN INICIAL (se ejecuta una sola vez) ===

  // Cada pin GPIO debe declarar su dirección antes de usarse:
  // INPUT para leer (botón), OUTPUT para escribir (LEDs).
  pinMode(buttonPin, INPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  // Inicializa la UART a 9600 baudios. La velocidad debe ser la
  // misma seleccionada en el Monitor Serie para que los datos se
  // decodifiquen correctamente.
  Serial.begin(9600);
  Serial.println("Sistema iniciado. Presiona el boton...");
}

void loop() {
  // === CICLO PRINCIPAL (se repite indefinidamente) ===

  // digitalRead() devuelve HIGH cuando el pin lee ~5 V (botón
  // cerrado, conectado a 5V) y LOW cuando lee ~0 V (botón abierto,
  // fijado por la resistencia de pull-down a GND).
  int buttonState = digitalRead(buttonPin);

  // Lógica condicional: estados complementarios. La activación de
  // una salida implica la desactivación de la otra.
  if (buttonState == HIGH) {
    // Botón PRESIONADO: rojo ON, verde OFF.
    digitalWrite(ledRojo, HIGH);
    digitalWrite(ledVerde, LOW);
    Serial.println("PRESIONADO | Rojo: ON  | Verde: OFF");
  } else {
    // Botón LIBERADO: rojo OFF, verde ON.
    digitalWrite(ledRojo, LOW);
    digitalWrite(ledVerde, HIGH);
    Serial.println("LIBERADO   | Rojo: OFF | Verde: ON");
  }

  // Espera bloqueante de 100 ms entre lecturas: limita la frecuencia
  // de lectura a ~10 lecturas por segundo. No es un debounce: el
  // rebote mecánico del pulsador se estudia en la Sesión 3.
  delay(100);
}
