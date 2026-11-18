// ============================================================
// RETO 3: Medicion del Periodo T completo con LEDs Alternados
// ============================================================
// - Pin 2 (INT0): Entrada del Sensor IR (o Boton en simulacion)
// - Pin 13: LED Rojo
// - Pin 12: LED Verde
// ============================================================

const int sensorPin = 2;   // Pin 2 (INT0)
const int ledRojo   = 13;  
const int ledVerde  = 12;  

// === VARIABLES COMPARTIDAS CON LA ISR ===
volatile unsigned long tiempoInicioPeriodo = 0;
volatile unsigned long periodoMicrosCompleto = 0;
volatile unsigned int contadorPasos = 0;
volatile bool nuevoPeriodoListo = false;

// Debounce para evitar lecturas falsas al pasar el objeto (100 ms)
const unsigned long tiempoDebounceMicros = 100000; 
volatile unsigned long ultimoTiempoInterrupt = 0;

// Variable para alternar los LEDs al completar un periodo
bool estadoAlternadoLED = false;

// === RUTINA DE SERVICIO DE INTERRUPCION (ISR) ===
void ISR_medirPeriodo() {
  unsigned long tiempoAhora = micros();

  // Filtro anti-rebote
  if (tiempoAhora - ultimoTiempoInterrupt > tiempoDebounceMicros) {
    contadorPasos++;
    ultimoTiempoInterrupt = tiempoAhora;

    // Cada paso impar marca el inicio de la oscilacion
    if (contadorPasos % 2 != 0) {
      tiempoInicioPeriodo = tiempoAhora;
    } 
    // Cada paso par completa una oscilacion completa T
    else {
      periodoMicrosCompleto = tiempoAhora - tiempoInicioPeriodo;
      nuevoPeriodoListo = true;
    }
  }
}

void setup() {
  pinMode(sensorPin, INPUT);
  pinMode(ledRojo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  // Estado inicial de los LEDs
  digitalWrite(ledRojo, LOW);
  digitalWrite(ledVerde, HIGH);

  Serial.begin(9600);
  Serial.println("==========================================");
  Serial.println("Reto 3: Medicion de T Completo y LEDs Alternados");
  Serial.println("==========================================");

  // --- CONFIGURACION DE INTERRUPCION ---
  // Boton (Simulacion / Pull-Down): usa RISING
  // Sensor Real FC-51: usa FALLING
  attachInterrupt(digitalPinToInterrupt(sensorPin), ISR_medirPeriodo, RISING);
}

void loop() {
  // --- PROCESAMIENTO AL COMPLETAR CADA PERIODO T ---
  if (nuevoPeriodoListo) {
    unsigned long periodoLocal;

    // Seccion Critica
    noInterrupts();
    periodoLocal = periodoMicrosCompleto;
    nuevoPeriodoListo = false;
    interrupts();

    // Conversion a segundos
    float periodoSegundos = periodoLocal / 1000000.0;

    // Alternar el LED activo
    estadoAlternadoLED = !estadoAlternadoLED;
    if (estadoAlternadoLED) {
      digitalWrite(ledRojo, HIGH);
      digitalWrite(ledVerde, LOW);
    } else {
      digitalWrite(ledRojo, LOW);
      digitalWrite(ledVerde, HIGH);
    }

    // Reportar periodo medido por el Monitor Serie
    Serial.print("Periodo Completo (T): ");
    Serial.print(periodoSegundos, 4);
    Serial.println(" s");
  }
}
