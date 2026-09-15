
#include <WiFi.h>
#include <ESP32Ping.h>
#include <HTTPClient.h>

const char* ssid     = "WIFI";
const char* password = "PASSWORD";
IPAddress targetIP(8, 8, 8, 8);  

const char* SERVER_URL = "http://192.168.18.XXX:5000/readings"; 

const unsigned long PROBE_INTERVAL_MS = 5000; 

struct Reading {
  unsigned long timestamp;
  char status[6];   // "OK" o "LOST"
  float latency;    
};

#define BUFFER_CAPACITY 300  
Reading buffer[BUFFER_CAPACITY];
int bufferCount = 0;

unsigned long probeCount = 0;
unsigned long lossCount  = 0;

void connectWiFi() {
  Serial.print("Conectando a WiFi");
  WiFi.begin(ssid, password);
  unsigned long start = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
    delay(300);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado. IP local: " + WiFi.localIP().toString());
  } else {
    Serial.println("\nSin conexión por ahora (se reintentará).");
  }
}

void addToBuffer(unsigned long t, const char* status, float latency) {
  if (bufferCount >= BUFFER_CAPACITY) {
    for (int i = 1; i < BUFFER_CAPACITY; i++) buffer[i - 1] = buffer[i];
    bufferCount--;
  }
  buffer[bufferCount].timestamp = t;
  strncpy(buffer[bufferCount].status, status, sizeof(buffer[bufferCount].status));
  buffer[bufferCount].latency = latency;
  bufferCount++;
}

bool sendReading(unsigned long t, const char* status, float latency) {
  if (WiFi.status() != WL_CONNECTED) return false;

  HTTPClient http;
  http.setConnectTimeout(2000);
  http.setTimeout(2000);
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");

  String payload = "[{\"timestamp_ms\":" + String(t) +
                    ",\"status\":\"" + String(status) + "\"" +
                    ",\"latency_ms\":" + String(latency, 1) + "}]";

  int code = http.POST(payload);
  http.end();
  return (code > 0 && code < 300);
}

void flushBuffer() {
  if (bufferCount == 0 || WiFi.status() != WL_CONNECTED) return;

  Serial.println(">>> Reconectado. Sincronizando " + String(bufferCount) + " lecturas retenidas...");

  String payload = "[";
  for (int i = 0; i < bufferCount; i++) {
    if (i > 0) payload += ",";
    payload += "{\"timestamp_ms\":" + String(buffer[i].timestamp) +
               ",\"status\":\"" + String(buffer[i].status) + "\"" +
               ",\"latency_ms\":" + String(buffer[i].latency, 1) + "}";
  }
  payload += "]";

  HTTPClient http;
  http.setConnectTimeout(3000);
  http.setTimeout(6000);
  http.begin(SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  http.end();

  if (code > 0 && code < 300) {
    Serial.println(">>> Sincronización exitosa (" + String(bufferCount) + " lecturas). HTTP " + String(code));
    bufferCount = 0;
  } else {
    Serial.println(">>> Fallo al sincronizar (HTTP " + String(code) + "). Se reintentará.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  connectWiFi();
  Serial.println("timestamp_ms,probe_id,status,latency_ms,plr_running_pct,buffer_pendiente");
}

void loop() {
  bool estabaDesconectado = (WiFi.status() != WL_CONNECTED);

  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  if (estabaDesconectado && WiFi.status() == WL_CONNECTED && bufferCount > 0) {
    flushBuffer();
  }

  probeCount++;
  unsigned long t0 = millis();
  String status;
  float latency = -1;

  if (WiFi.status() == WL_CONNECTED) {
    bool success = Ping.ping(targetIP, 1);
    if (success) {
      latency = Ping.averageTime();
      status = "OK";
    } else {
      lossCount++;
      status = "LOST";
    }
  } else {
    lossCount++;
    status = "LOST";
  }

  float plrPct = (100.0 * lossCount) / probeCount;

  bool enviado = sendReading(t0, status.c_str(), latency);
  if (!enviado) {
    addToBuffer(t0, status.c_str(), latency);
  }

  Serial.print(t0); Serial.print(",");
  Serial.print(probeCount); Serial.print(",");
  Serial.print(status); Serial.print(",");
  Serial.print(latency, 1); Serial.print(",");
  Serial.print(plrPct, 2); Serial.print(",");
  Serial.println(bufferCount);

  delay(PROBE_INTERVAL_MS);
}
