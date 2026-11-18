/*
 * lab-07-parte2-motor-driver.ino
 * Sesión 7 — Parte 2: base del accionamiento (Reto 1)
 *
 * Control del motor por el puente H con un ciclo de trabajo fijo. El grupo
 * monta la etapa de potencia, ajusta la fuente y verifica el sentido de giro.
 *
 * Circuito (módulo L298N):
 *   D9    ENA  habilitación del canal A (PWM). El jumper ENA del módulo se
 *              retira antes de conectar este pin.
 *   D7    IN1  primera entrada de dirección del canal A
 *   D8    IN2  segunda entrada de dirección del canal A
 *   OUT1/OUT2  bornes del motor
 *   Bornera de potencia (VS): fuente externa, 7.2 a 7.5 V, límite 0.3 A
 *   GND del módulo y GND del Arduino en común
 *
 * D2 queda declarado como canal A del encoder del motor. Este sketch no
 * configura la interrupción ni cuenta pulsos.
 *
 * Monitor Serie: 9600 baudios.
 */

const uint8_t PIN_ENA = 9;      // D9: salida PWM (~), Timer1 a 490 Hz
const uint8_t PIN_IN1 = 7;
const uint8_t PIN_IN2 = 8;
const uint8_t PIN_ENC_A = 2;    // D2 = INT0

// Ciclo de trabajo fijo de arranque, en la escala de analogWrite(). Se ajusta
// editando esta constante. Con 255 el motor recibe el voltaje máximo que la
// fuente y el driver permiten, que es el punto donde se mide la tensión entre
// sus bornes.
const uint8_t DUTY = 255;

void setup() {
  Serial.begin(9600);

  pinMode(PIN_ENA, OUTPUT);
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  // Avance: IN1 en alto e IN2 en bajo. Intercambiar los dos niveles invierte
  // el sentido de giro.
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, LOW);
  analogWrite(PIN_ENA, DUTY);

  Serial.println(F("ENA=D9  IN1=D7  IN2=D8  encoder A=D2"));
  Serial.print(F("Duty fijo: "));
  Serial.println(DUTY);
}

void loop() {
  // Sin bucle de trabajo: el accionamiento es de lazo abierto y el ciclo de
  // trabajo es constante.
}
