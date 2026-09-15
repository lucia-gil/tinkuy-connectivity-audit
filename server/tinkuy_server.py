from flask import Flask, request, jsonify
import csv
import os
from datetime import datetime

app = Flask(__name__)
CSV_FILE = "tinkuy_cloud_received.csv"

if not os.path.exists(CSV_FILE):
    with open(CSV_FILE, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["received_at_server", "timestamp_ms_esp32", "status", "latency_ms"])


@app.route("/readings", methods=["POST"])
def receive_readings():
    data = request.get_json(force=True, silent=True) or []
    if not isinstance(data, list):
        data = [data]

    now = datetime.now().isoformat(timespec="seconds")

    with open(CSV_FILE, "a", newline="") as f:
        writer = csv.writer(f)
        for r in data:
            writer.writerow([
                now,
                r.get("timestamp_ms"),
                r.get("status"),
                r.get("latency_ms"),
            ])

    etiqueta = "lote sincronizado" if len(data) > 1 else "lectura en vivo"
    print(f"[{now}] Recibí {len(data)} registro(s) ({etiqueta}).")

    return jsonify({"received": len(data)}), 200


@app.route("/", methods=["GET"])
def health():
    return "TINKUY cloud simulator activo. Esperando datos en /readings (POST)."


if __name__ == "__main__":
    print("Servidor TINKUY (simulador de nube) corriendo en el puerto 5000...")
    print(f"Guardando datos recibidos en: {CSV_FILE}")
    app.run(host="0.0.0.0", port=5000)
