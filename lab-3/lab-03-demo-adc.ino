/*
 * Patrón de referencia: lectura del ADC con un potenciómetro y conversión a voltaje.
 * Sesión 3 — worked example de la presentación de teoría.
 *
 * Propósito: introducir el ADC de 10 bits del ATmega328P como instrumento de
 * medición. Un voltaje continuo en el pin analógico se convierte en un entero
 * de 0 a 1023 (analogRead), y ese número se interpreta como voltaje mediante la
 * fórmula de conversión. El potenciómetro es el primer transductor: un divisor
 * de voltaje ajustable cuyo cursor entrega un voltaje proporcional a la posición.
 *
 * Circuito requerido:
 *   - Potenciómetro (10 kΩ): extremos a 5 V y GND; cursor (terminal central) a A0.
 *   - Multímetro en el mismo nodo (cursor↔GND) para verificar la conversión.
 *
 * Qué se observa: la lectura cruda raw es proporcional al voltaje medido con el
 * multímetro, con pendiente ≈ 1023/5 = 204.6 cuentas por voltio. raw ≈ 0 en el
 * extremo de GND y raw ≈ 1023 en el extremo de 5 V. La proporcionalidad confirma
 * que el ADC es un instrumento lineal y verificable, base para interpretar
 * cualquier otro transductor resistivo leído con un divisor.
 */

// === DEFINICIÓN DE PINES Y CONSTANTES ===
const int PIN_POT = A0;          // Entrada analógica del cursor del potenciómetro.

// Referencia del ADC: 5 V (alimentación de la placa). El numerador de la
// conversión es la referencia; el denominador es la cuenta máxima (1023).
const float VREF = 5.0;
const float CUENTA_MAX = 1023.0;

// Intervalo de muestreo (ms). El muestreo periódico no bloqueante (base de
// tiempo de S2) fija el ritmo de adquisición sin detener el programa.
const unsigned long PERIODO_MS = 500;

unsigned long instanteAnterior = 0;   // Marca de tiempo de la última muestra.

void setup() {
  Serial.begin(9600);
  // No se requiere pinMode para entradas analógicas: analogRead() configura el
  // pin como entrada del multiplexor del ADC.
}

void loop() {
  // Muestreo periódico no bloqueante: el cuerpo solo se ejecuta al cumplirse
  // PERIODO_MS; entre muestras el programa queda libre (patrón instalado en S2).
  if (millis() - instanteAnterior >= PERIODO_MS) {
    instanteAnterior = millis();

    // Lectura cruda del ADC: 0..1023 según el voltaje en el cursor.
    int raw = analogRead(PIN_POT);

    // Conversión raw -> voltaje. Multiplicar en float (1023.0) evita la división
    // entera, que truncaría el resultado a 0 para todo raw < 1023.
    float voltaje = raw * (VREF / CUENTA_MAX);

    // Reporte en columnas numéricas puras (raw y voltaje separados por
    // tabulación): el Monitor Serie los lista y el Serial Plotter los grafica
    // como dos curvas. Sin prefijos de texto ("raw =", "V ="), que el Plotter
    // no puede interpretar como números.
    Serial.print(raw);
    Serial.print("\t");
    Serial.println(voltaje, 3);   // 3 decimales, coherente con ~4.9 mV por cuenta.
  }
}
