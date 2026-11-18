from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

# Aplicar estilo de fondo oscuro
plt.style.use('dark_background')

# Cargar el archivo CSV generado más reciente
dir_output = Path("output")
archivos = sorted(dir_output.glob("barrido_5pasadas_*.csv"))

if not archivos:
    print("No se encontraron archivos de barrido en la carpeta 'output'.")
    exit()

archivo_csv = archivos[-1]
print(f"Cargando datos desde: {archivo_csv}")

df = pd.read_csv(archivo_csv)

# Agrupar por Duty y calcular Promedio de las 5 pasadas
resumen = df.groupby("duty").agg(
    rpm_promedio=("rpm", "mean")
).reset_index()

# Ordenar de 0 a 255 para trazar la curva correctamente
resumen = resumen.sort_values(by="duty").reset_index(drop=True)

print("\n--- RESUMEN DE LAS 5 PASADAS POR DUTY ---")
print(resumen.to_string(index=False))

# Trazado de la gráfica (Dark Theme)
plt.figure(figsize=(9, 5))
plt.plot(
    resumen["duty"], 
    resumen["rpm_promedio"], 
    '-', 
    color='#00E5FF',  # Cyan brillante para alto contraste
    linewidth=2, 
    label='Promedio de 5 valores de RPM por Duty'
)

plt.title("Caracterización Duty vs. RPM (Barrido Descendente Automatizado)", fontsize=12, fontweight='bold', color='white')
plt.xlabel("Ciclo de Trabajo (Duty [0 - 255])", fontsize=11, color='white')
plt.ylabel("Velocidad Medida (RPM)", fontsize=11, color='white')
plt.grid(True, linestyle='--', alpha=0.3, color='gray')
plt.legend(loc='upper left', facecolor='#1e1e1e', edgecolor='none')
plt.tight_layout()

# Guardar imagen (se convierte a str para evitar errores en Windows)
ruta_imagen = dir_output / f"{archivo_csv.stem}.png"
plt.savefig(str(ruta_imagen), dpi=300, facecolor='black')
print(f"\n[+] Gráfica guardada en: {ruta_imagen.resolve()}")
plt.show()