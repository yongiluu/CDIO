#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 2;

// MAC del receptor_RX ya configurada
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9F, 0x19, 0x60};

typedef struct struct_message {
    int id_mensaje;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

int contador = 1;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("Entrega OK! 💡");
        
        digitalWrite(LED_PIN, HIGH);
        delay(250);
        digitalWrite(LED_PIN, LOW);
    } else {
        Serial.println("Fallo al entregar (¿El carrito está apagado?)");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

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
    
    // Aumenté el tiempo de espera a 1.5s para que se vea claro que el led se apaga y se vuelve a prender.
    delay(1500); 
}
