#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

const int LED_PIN = 2;

const int PIN_SERVO_DIR = 13;
const uint8_t PIN_EN = 27;    // R_EN y L_EN
const uint8_t PIN_RPWM = 25;  
const uint8_t PIN_LPWM = 26;  

Servo servoDireccion;

typedef struct MensajeDatos {
    int velocidad; 
    int angulo;    
} MensajeDatos;

MensajeDatos datosEntrantes;

void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
    digitalWrite(LED_PIN, HIGH);
    memcpy(&datosEntrantes, datos, sizeof(datosEntrantes));

    int velocidad_final = datosEntrantes.velocidad;
    int angulo_final = datosEntrantes.angulo;

    // 1. SERVO
    servoDireccion.write(angulo_final);

    // 2. MOTOR DE TRACCIÓN (A prueba de fallos absolutos)
    if (velocidad_final > 0) {
        // Avanzar
        digitalWrite(PIN_EN, HIGH);      // Habilita el puente H SOLO cuando nos vamos a mover
        ledcWrite(1, 0);                 
        ledcWrite(0, velocidad_final);   
    } else if (velocidad_final < 0) {
        // Retroceder
        digitalWrite(PIN_EN, HIGH);      // Habilita el puente H SOLO cuando nos vamos a mover
        ledcWrite(0, 0);                 
        ledcWrite(1, abs(velocidad_final)); 
    } else {
        // Detenido absoluto
        digitalWrite(PIN_EN, LOW);       // Corta la energía del puente H por completo
        ledcWrite(0, 0);
        ledcWrite(1, 0);
    }

    digitalWrite(LED_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    delay(2000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // --- CONFIGURACIÓN SEGURA DEL MOTOR ---
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW); // MUY IMPORTANTE: Iniciar con el motor CORTADO de energía

    ledcSetup(0, 5000, 8); 
    ledcSetup(1, 5000, 8); 
    ledcAttachPin(PIN_RPWM, 0);
    ledcAttachPin(PIN_LPWM, 1);

    ledcWrite(0, 0);
    ledcWrite(1, 0);
    // --------------------------------------

    servoDireccion.setPeriodHertz(50);
    ESP32PWM::allocateTimer(2); 
    servoDireccion.attach(PIN_SERVO_DIR, 500, 2500);
    servoDireccion.write(90);

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    delay(10);
}
