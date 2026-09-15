import serial
import csv
import time
from datetime import datetime

PUERTO = "COM3" 
BAUD_RATE = 115200
ARCHIVO_SALIDA = f"tinkuy_datos_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

def main():
    print(f"Conectando a {PUERTO} a {BAUD_RATE} baudios...")
    ser = serial.Serial(PUERTO, BAUD_RATE, timeout=2)
    time.sleep(2)  # dar tiempo a que el ESP32 reinicie tras abrir el puerto

    print(f"Guardando datos en: {ARCHIVO_SALIDA}")
    print("Presiona Ctrl+C para detener el experimento.\n")

    with open(ARCHIVO_SALIDA, "w", newline="") as f:
        writer = csv.writer(f)
        header_written = False
        columnas_esperadas = None

        try:
            while True:
                linea = ser.readline().decode("utf-8", errors="ignore").strip()
                if not linea:
                    continue

                print(linea) 
                partes = linea.split(",")

                if not header_written:
                    writer.writerow(partes)
                    header_written = True
                    columnas_esperadas = len(partes)
                    continue

                if len(partes) == columnas_esperadas:
                    writer.writerow(partes)
                    f.flush()  

        except KeyboardInterrupt:
            print("\nExperimento detenido. Datos guardados en:", ARCHIVO_SALIDA)
        finally:
            ser.close()


if __name__ == "__main__":
    main()
