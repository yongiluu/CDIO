#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// En la mayoría de ESP32 clásicos, el LED azul está directamente en el pin 2
const int LED_PIN = 2;

typedef struct struct_message {
    int id_mensaje; 
} struct_message;

struct_message myData;

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));
    
    Serial.print("RECIBIDO - Mensaje #");
    Serial.println(myData.id_mensaje);

    // Hacer parpadear el LED (más tiempo para que se note)
    digitalWrite(LED_PIN, HIGH);
    delay(250); 
    digitalWrite(LED_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    delay(2000); 
    
    // Configurar explícitamente el pin 2
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    WiFi.mode(WIFI_STA);

    Serial.println("\n===========================");
    Serial.println("   RECEPTOR (CARRITO)      ");
    Serial.print(">> MI MAC ADDRESS ES: ");
    Serial.println(WiFi.macAddress());
    Serial.println("===========================\n");

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
    Serial.println("Esperando mensajes del control...");
}

void loop() {
    delay(100);
}
