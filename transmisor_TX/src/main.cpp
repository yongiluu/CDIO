#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 2;

// MAC del receptor_RX
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9F, 0x19, 0x60};

typedef struct MensajeDatos {
    int velocidad; 
    int angulo;    
} MensajeDatos;

MensajeDatos myData;
esp_now_peer_info_t peerInfo;

const int PIN_JOYSTICK_VELOCIDAD = 34; // Y
const int PIN_JOYSTICK_ANGULO = 35;    // X

// --- ZONA MUERTA (MÁS AMPLIA PARA EVITAR SENSIBILIDAD EXTREMA) ---
const int CENTRO_MIN = 1400; 
const int CENTRO_MAX = 2000;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        digitalWrite(LED_PIN, HIGH);
        delay(5); 
        digitalWrite(LED_PIN, LOW);
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    analogReadResolution(12); // Lee de 0 a 4095

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return;
    esp_now_register_send_cb(OnDataSent);

    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    esp_now_add_peer(&peerInfo);
}

void loop() {
    int crudo_vel = analogRead(PIN_JOYSTICK_VELOCIDAD);
    int crudo_ang = analogRead(PIN_JOYSTICK_ANGULO);
    
    // 1. PROCESAR VELOCIDAD
    if (crudo_vel > CENTRO_MAX) {
        myData.velocidad = map(crudo_vel, CENTRO_MAX, 4095, 0, 255);
    } else if (crudo_vel < CENTRO_MIN) {
        myData.velocidad = map(crudo_vel, CENTRO_MIN, 0, 0, -255);
    } else {
        myData.velocidad = 0; // Zona Muerta
    }

    // 2. PROCESAR ÁNGULO
    if (crudo_ang > CENTRO_MAX) {
        myData.angulo = map(crudo_ang, CENTRO_MAX, 4095, 90, 130);
    } else if (crudo_ang < CENTRO_MIN) {
        myData.angulo = map(crudo_ang, CENTRO_MIN, 0, 90, 50);
    } else {
        myData.angulo = 90; // Zona Muerta
    }
    
    Serial.printf("Lectura Cruda: Vel=%d Ang=%d >>> Enviando: Vel: %d | Ang: %d\n", 
                  crudo_vel, crudo_ang, myData.velocidad, myData.angulo);
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    delay(50); 
}
