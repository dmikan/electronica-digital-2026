/*
 * lab-06-parte2-multicanal-csv.ino
 *
 * Base de adquisicion de dos canales con registro CSV por el puerto serie.
 *
 * Circuito:
 *   A0  LDR con divisor (resistencia de referencia de S3)
 *   A2  potenciometro (extremos a 5 V y GND, cursor al pin analogico)
 *
 * A1 queda libre. A4 y A5 dejan de ser entradas analogicas cuando el bus I2C
 * entra en servicio.
 *
 * Cada muestra se emite como una linea:
 *
 *   t_ms,ldr,pot
 *
 * donde t_ms es el valor de millis() en el instante del muestreo y los dos
 * campos restantes son cuentas del ADC (0..1023).
 *
 * El intervalo de muestreo es un parametro de diseno del grupo. El planificador
 * que lo sostiene sin depender de la duracion de una vuelta ya esta resuelto
 * aqui: la referencia se reasigna en cada muestra.
 *
 * Monitor Serie: 9600 baudios.
 */

const uint8_t PIN_LDR = A0;
const uint8_t PIN_POT = A2;

// Intervalo de muestreo, en milisegundos. Valor de partida: el grupo lo ajusta.
const unsigned long T_MUESTREO_MS = 250;

void setup() {
  Serial.begin(9600);

  // Los dos pines son entradas analogicas: analogRead() no requiere pinMode().
  // A4 y A5 quedan reservados al bus I2C cuando se invoca Wire.begin().

  // Encabezado del archivo, una sola vez por sesion de captura.
  Serial.println(F("t_ms,ldr,pot"));
}

void loop() {
  static unsigned long tUltimaMuestra = 0;

  unsigned long ahora = millis();

  if (ahora - tUltimaMuestra >= T_MUESTREO_MS) {
    // Referencia de la proxima muestra: se toma el instante actual, de modo
    // que el intervalo se cuenta desde esta muestra y no desde el arranque.
    tUltimaMuestra = ahora;

    int ldr = analogRead(PIN_LDR);
    int pot = analogRead(PIN_POT);

    Serial.print(ahora);
    Serial.print(',');
    Serial.print(ldr);
    Serial.print(',');
    Serial.println(pot);
  }
}
