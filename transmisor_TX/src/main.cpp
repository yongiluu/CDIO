#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 2;

// MAC del receptor_RX
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9F, 0x19, 0x60};

typedef struct MensajeDatos {
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = ruedas rectas)
} MensajeDatos;

MensajeDatos myData;
esp_now_peer_info_t peerInfo;

const int PIN_JOYSTICK_VELOCIDAD = 34; // Y
const int PIN_JOYSTICK_ANGULO = 35;    // X

// --- ZONA MUERTA (DEADZONE) ---
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
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    analogReadResolution(12); // Lee de 0 a 4095

    WiFi.mode(WIFI_STA);
    
    Serial.println("\n==========================================");
    Serial.println("       TRANSMISOR (CONTROL - TX)          ");
    Serial.print  (">> MAC Transmisor: ");
    Serial.println(WiFi.macAddress());
    Serial.println("==========================================\n");

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_send_cb(OnDataSent);

    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("❌ Fallo al emparejar con el receptor");
        return;
    }

    Serial.println(">> Transmitiendo comandos...");
}

void loop() {
    int crudo_vel = analogRead(PIN_JOYSTICK_VELOCIDAD);
    int crudo_ang = analogRead(PIN_JOYSTICK_ANGULO);
    
    // 1. PROCESAR VELOCIDAD (-255 a 255, 0 = Detenido)
    if (crudo_vel > CENTRO_MAX) {
        myData.velocidad = map(crudo_vel, CENTRO_MAX, 4095, 0, 255);
    } else if (crudo_vel < CENTRO_MIN) {
        myData.velocidad = map(crudo_vel, CENTRO_MIN, 0, 0, -255);
    } else {
        myData.velocidad = 0; // Zona Muerta
    }

    // 2. PROCESAR ÁNGULO DE DIRECCIÓN (-40° a 40°, 0° = Recto)
    if (crudo_ang > CENTRO_MAX) {
        myData.angulo = map(crudo_ang, CENTRO_MAX, 4095, 0, 40);
    } else if (crudo_ang < CENTRO_MIN) {
        myData.angulo = map(crudo_ang, CENTRO_MIN, 0, 0, -40);
    } else {
        myData.angulo = 0; // Zona Muerta: 0° (Centrado)
    }
    
    // Mostramos velocidad y ángulo relativo (-40° a +40°)
    Serial.printf("Enviando -> Vel: %4d | Ang: %3d°\n", myData.velocidad, myData.angulo);
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    delay(50); 
}
