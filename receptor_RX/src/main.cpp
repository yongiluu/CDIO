#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Definimos el pin del LED (PlatformIO suele tener mapeado LED_BUILTIN, 
// pero en muchas ESP32-S3 suele ser el pin 2 o 48). 
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// Estructura super simple para esta prueba
typedef struct struct_message {
    int id_mensaje; // Un simple contador
} struct_message;

struct_message myData;

// Callback al recibir datos
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));
    
    Serial.print("RECIBIDO - Mensaje #");
    Serial.println(myData.id_mensaje);

    // Hacer parpadear el LED
    digitalWrite(LED_BUILTIN, HIGH);
    delay(100); // Espera de 100ms
    digitalWrite(LED_BUILTIN, LOW);
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Esperar que inicie el serial
    
    // Configurar LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW); // Apagado por defecto

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
    // No necesitamos nada en el loop para esta prueba
    delay(100);
}
