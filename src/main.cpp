#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

#include "secrets.h"

// ---- HC-SR04 Pins ----
#define TRIG_PIN 4
#define ECHO_PIN 5

long readDistanceOnceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, 30000);
  if (duration == 0) return -1;

  return (long)(duration * 0.0343 / 2.0);
}

long readDistanceMedianCm() {
  long a[5];

  for (int i = 0; i < 5; i++) {
    a[i] = readDistanceOnceCm();
    delay(30);
  }

  for (int i = 0; i < 5; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (a[j] < a[i]) {
        long t = a[i];
        a[i] = a[j];
        a[j] = t;
      }
    }
  }

  return a[2];
}

bool firebasePutInt(const char* pathNoSlash, int value) {

  String url = String(FIREBASE_RTDB_URL) + pathNoSlash + ".json";

  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;

  if (!http.begin(client, url)) {
    Serial.println("HTTP begin failed");
    return false;
  }

  http.addHeader("Content-Type", "application/json");

  int code = http.PUT(String(value));

  Serial.print("Firebase PUT | HTTP ");
  Serial.print(code);
  Serial.print(" | ");
  Serial.println(url);

  String resp = http.getString();
  if (resp.length()) {
    Serial.print("Response: ");
    Serial.println(resp);
  }

  http.end();

  return (code >= 200 && code < 300);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  Serial.print("Connecting to WiFi");

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");

    if (millis() - start > 20000) {
      Serial.println("\nWiFi connect timeout. Restarting...");
      ESP.restart();
    }
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

unsigned long lastSendMs = 0;

void loop() {

  if (millis() - lastSendMs >= 1000) {
    lastSendMs = millis();

    long d = readDistanceMedianCm();

    Serial.print("Distance (cm): ");
    Serial.println(d);

    firebasePutInt("lab5/distance_cm", (int)d);
  }
}
