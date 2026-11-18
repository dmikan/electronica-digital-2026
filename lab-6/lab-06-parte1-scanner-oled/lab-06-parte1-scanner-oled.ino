/*
 * lab-06-parte1-scanner-oled.ino
 *
 * Reconocimiento del bus I2C y puesta en marcha del display OLED SSD1306 128x64.
 *
 * Circuito: modulo OLED alimentado a 5 V, SDA a A4, SCL a A5.
 *
 * El sketch barre las direcciones 1..126 del bus, informa por el Monitor Serie
 * cuales responden, inicializa el display en la direccion encontrada y muestra
 * un mensaje con la direccion y el tiempo transcurrido.
 *
 * Monitor Serie: 9600 baudios.
 */
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Geometria del modulo. El OLED de 0.96" es de 128x64; el de 0.91" es de
// 128x32. La altura decide el tamano del buffer: 64 filas = 1024 bytes,
// 32 filas = 512 bytes. Si el modulo es de 0.91", cambiar ALTO_PX a 32.
const uint8_t ANCHO_PX = 128;
const uint8_t ALTO_PX = 64;

// El pin de reset se declara como -1: el modulo no expone linea de reset propia
// y el estado inicial lo fija la alimentacion.
Adafruit_SSD1306 display(ANCHO_PX, ALTO_PX, &Wire, -1);

// Direccion del display. El barrido del bus la confirma o la corrige.
uint8_t direccionOLED = 0x3C;

// Direcciones tipicas de los modulos SSD1306. Cualquier otro dispositivo que
// responda al barrido se informa, pero no se toma como display.
bool esDireccionDeSSD1306(uint8_t dir) {
  return (dir == 0x3C) || (dir == 0x3D);
}

// Recorre el bus completo e imprime una linea por dispositivo encontrado.
// Devuelve la cantidad de dispositivos que respondieron.
uint8_t barrerBus() {
  uint8_t encontrados = 0;

  for (uint8_t dir = 1; dir < 127; dir++) {
    Wire.beginTransmission(dir);
    if (Wire.endTransmission() == 0) {
      Serial.print(F("Dispositivo en 0x"));
      if (dir < 16) {
        Serial.print('0');
      }
      Serial.println(dir, HEX);
      encontrados++;

      if (esDireccionDeSSD1306(dir)) {
        direccionOLED = dir;
      }
    }
  }

  if (encontrados == 0) {
    Serial.println(F("Sin dispositivos en el bus"));
  }

  return encontrados;
}

void mostrarEstado(unsigned long t) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.println(F("Bus I2C operativo"));

  display.print(F("OLED en 0x"));
  if (direccionOLED < 16) {
    display.print('0');
  }
  display.println(direccionOLED, HEX);

  display.print(F("t = "));
  display.print(t / 1000);
  display.println(F(" s"));

  // El refresco envia el buffer completo en tramas de 16 bytes: 64 transferencias
  // y ~104 ms a 100 kHz. No conviene repetirlo en cada vuelta del bucle.
  display.display();
}

void setup() {
  Serial.begin(9600);
  Wire.begin();

  Serial.println(F("Barrido del bus I2C"));
  uint8_t dispositivos = barrerBus();
  Serial.print(F("Dispositivos encontrados: "));
  Serial.println(dispositivos);

  // Inicializacion con la direccion detectada. Si falla, se informa y el
  // sketch queda detenido: sin bus no hay display que actualizar.
  if (!display.begin(SSD1306_SWITCHCAPVCC, direccionOLED)) {
    Serial.println(F("El display no respondio en la direccion detectada"));
    Serial.println(F("Verificar SDA, SCL, VCC y GND del modulo"));
    while (true) {
    }
  }

  Serial.print(F("Display inicializado en 0x"));
  Serial.println(direccionOLED, HEX);
}

void loop() {
  static unsigned long tUltimaActualizacion = 0;

  unsigned long ahora = millis();

  // Refresco cada 500 ms: la trama completa tarda decenas de milisegundos y no
  // necesita repetirse en cada vuelta del bucle.
  if (ahora - tUltimaActualizacion >= 500) {
    tUltimaActualizacion = ahora;
    mostrarEstado(ahora);
  }
}
