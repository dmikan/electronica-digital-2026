import csv
from datetime import datetime
from pathlib import Path

import serial
from serial.tools import list_ports

# Crear la carpeta output si no existe
DIR_OUTPUT = Path("output")
DIR_OUTPUT.mkdir(parents=True, exist_ok=True)

PUERTO = "COM3"  # Cambia por el puerto de tu Arduino.
BAUDIOS = 9600
# Guardar el archivo dentro de la carpeta output
ARCHIVO = DIR_OUTPUT / f"datos_{datetime.now():%Y%m%d_%H%M%S_%f}.csv"

print("Puertos disponibles:")
for puerto in list_ports.comports():
    print(f"  {puerto.device}: {puerto.description}")

filas = 0
try:
    with serial.Serial(PUERTO, BAUDIOS, timeout=1) as arduino:
        with ARCHIVO.open("x", newline="", encoding="utf-8") as archivo:
            escritor = csv.writer(archivo)
            escritor.writerow(["t_ms", "ldr", "pot"])
            archivo.flush()
            print(f"Guardando en: {ARCHIVO.resolve()}")
            print("Detén la captura con Ctrl+C; registra al menos 100 filas.")

            while True:
                linea = arduino.readline().decode("utf-8", errors="replace").strip()
                campos = linea.split(",")
                if len(campos) != 3:
                    continue
                try:
                    t_ms, ldr, pot = map(int, campos)
                except ValueError:
                    continue  # Ignora encabezados y líneas incompletas.
                if t_ms < 0 or not (0 <= ldr <= 1023 and 0 <= pot <= 1023):
                    continue
                escritor.writerow([t_ms, ldr, pot])
                archivo.flush()
                filas += 1
                print(f"{filas}: {t_ms},{ldr},{pot}")
except KeyboardInterrupt:
    print(f"\nCaptura finalizada. Filas guardadas: {filas}")
except (serial.SerialException, OSError) as error:
    print(f"Error: {error}")
    print(f"Filas guardadas: {filas}")