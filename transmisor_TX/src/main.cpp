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
// El centro real de cada joystick se mide al encender (no tocar los joysticks
// durante el primer segundo). La zona muerta es +-ZONA_MUERTA alrededor de ese centro.
const int ZONA_MUERTA = 300;
int centroVel = 2048;
int centroAng = 2048;

int leerPromedio(int pin) {
    long suma = 0;
    for (int i = 0; i < 64; i++) {
        suma += analogRead(pin);
        delay(2);
    }
    return suma / 64;
}

// Convierte la lectura cruda (0-4095) a un valor entre -limite y +limite,
// con 0 dentro de la zona muerta alrededor del centro medido
int mapearEje(int crudo, int centro, int limite) {
    if (crudo > centro + ZONA_MUERTA) {
        return constrain(map(crudo, centro + ZONA_MUERTA, 4095, 0, limite), 0, limite);
    } else if (crudo < centro - ZONA_MUERTA) {
        return constrain(map(crudo, centro - ZONA_MUERTA, 0, 0, -limite), -limite, 0);
    }
    return 0; // Zona Muerta
}

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

    // Calibración del centro de los joysticks (deben estar sueltos al encender)
    centroVel = leerPromedio(PIN_JOYSTICK_VELOCIDAD);
    centroAng = leerPromedio(PIN_JOYSTICK_ANGULO);

    WiFi.mode(WIFI_STA);
    
    Serial.println("\n==========================================");
    Serial.println("       TRANSMISOR (CONTROL - TX)          ");
    Serial.print  (">> MAC Transmisor: ");
    Serial.println(WiFi.macAddress());
    Serial.printf(">> Centro joystick velocidad: %d | dirección: %d\n", centroVel, centroAng);
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
    myData.velocidad = mapearEje(crudo_vel, centroVel, 255);

    // 2. PROCESAR ÁNGULO DE DIRECCIÓN (-40° a 40°, 0° = Recto)
    myData.angulo = mapearEje(crudo_ang, centroAng, 40);
    
    // Mostramos lecturas crudas, velocidad y ángulo relativo (-40° a +40°)
    Serial.printf("Crudo vel: %4d ang: %4d -> Vel: %4d | Ang: %3d°\n",
                  crudo_vel, crudo_ang, myData.velocidad, myData.angulo);
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    delay(50); 
}
