import csv
from datetime import datetime
from pathlib import Path
import time
import serial

DIR_OUTPUT = Path("output")
DIR_OUTPUT.mkdir(parents=True, exist_ok=True)

PUERTO = "COM3"  # Cambia al puerto de tu Arduino
BAUDIOS = 9600
ARCHIVO = DIR_OUTPUT / f"barrido_5pasadas_{datetime.now():%Y%m%d_%H%M%S}.csv"

# Definir la secuencia de valores de Duty (de 255 a 0)
# Puedes usar una lista de puntos específicos o un rango con paso definido:
VALORES_DUTY = list(range(255, -1, -1))
# Si prefieres ir de 255 a 0 bajando de 15 en 15, usa:
# VALORES_DUTY = list(range(255, -1, -15))

PASADAS_POR_DUTY = 5  # Cantidad de muestras a registrar por cada valor

try:
    with serial.Serial(PUERTO, BAUDIOS, timeout=1.5) as arduino:
        print(f"[+] Conectado a {PUERTO}. Esperando reinicio de la placa...")
        time.sleep(2)  # Espera a que termine el reset de Arduino
        
        # Limpiar buffer de entrada
        arduino.reset_input_buffer()

        with ARCHIVO.open("w", newline="", encoding="utf-8") as archivo:
            escritor = csv.writer(archivo)
            escritor.writerow(["pasada", "duty", "frecuencia_hz", "rpm"])
            archivo.flush()

            print(f"[+] Guardando datos en: {ARCHIVO.resolve()}\n")
            print("=" * 60)

            for duty in VALORES_DUTY:
                # 1. Enviar el nuevo valor de Duty al Arduino
                comando = f"{duty}\n"
                arduino.write(comando.encode("utf-8"))
                print(f"\n>>> Fijando Duty en: {duty}")
                
                # Tiempo de estabilización (esperar a que el motor alcance la velocidad)
                time.sleep(1.5)
                
                # Limpiar cualquier mensaje viejo retenido en el buffer
                arduino.reset_input_buffer()

                muestras_tomadas = 0
                while muestras_tomadas < PASADAS_POR_DUTY:
                    linea = arduino.readline().decode("utf-8", errors="replace").strip()
                    campos = linea.split("\t")

                    if len(campos) == 3:
                        try:
                            d_leido = int(campos[0].strip())
                            frec = float(campos[1].strip())
                            rpm = float(campos[2].strip())

                            # Validar que la lectura corresponda al Duty actual
                            if d_leido == duty:
                                muestras_tomadas += 1
                                escritor.writerow([muestras_tomadas, duty, frec, rpm])
                                archivo.flush()
                                print(f"  [Muestra {muestras_tomadas}/{PASADAS_POR_DUTY}] Duty: {duty} | Frecuencia: {frec:.1f} Hz | RPM: {rpm:.2f}")
                        except ValueError:
                            pass  # Ignorar impresiones no numéricas

            print("\n" + "=" * 60)
            print("[✓] Barrido automatizado finalizado con éxito.")

except KeyboardInterrupt:
    print("\n[!] Proceso interrumpido manualmente.")
except (serial.SerialException, OSError) as error:
    print(f"\n[!] Error en la comunicación serial: {error}")