#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

// ¡RECUERDA CAMBIAR ESTO POR LA MAC DEL RECEPTOR!
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Misma estructura sencilla
typedef struct struct_message {
    int id_mensaje;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

int contador = 1;

// Callback al enviar datos
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("Entrega OK!");
        
        // Parpadear el LED porque se entregó con éxito
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
        digitalWrite(LED_BUILTIN, LOW);
    } else {
        Serial.println("Fallo al entregar (¿El carrito está apagado?)");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    // Configurar LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    WiFi.mode(WIFI_STA);
    
    Serial.println("\n--- CONTROL REMOTO (EMISOR) ---");

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }

    esp_now_register_send_cb(OnDataSent);

    memset(&peerInfo, 0, sizeof(peerInfo));
    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Fallo al emparejar");
        return;
    }
}

void loop() {
    myData.id_mensaje = contador;
    
    Serial.print("Enviando mensaje #");
    Serial.println(contador);
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    contador++;
    
    // Enviar un mensaje cada 1 segundo (1000ms) para que se vea claro el parpadeo
    delay(1000);
}
