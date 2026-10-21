#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

const int LED_PIN = 2;

// MAC del receptor_RX
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9F, 0x19, 0x60};

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el RX)
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = ruedas rectas)
} MensajeDatos;

// Telemetría que llega desde el auto (RX)
const int CLAVE_TELEMETRIA = 0xCD11;

typedef struct TelemetriaDatos {
    int clave;       // CLAVE_TELEMETRIA
    int bateria_mV;  // voltaje de la batería en milivoltios
} TelemetriaDatos;

volatile int bateriaAuto_mV = -1;  // -1 = aún no llega telemetría

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

int leerPromedio(int pin, int muestras, int esperaMs) {
    long suma = 0;
    for (int i = 0; i < muestras; i++) {
        suma += analogRead(pin);
        if (esperaMs > 0) delay(esperaMs);
    }
    return suma / muestras;
}

// Si el centro medido está muy lejos de la mitad, el joystick se estaba tocando
// o está mal conectado: se usa 2048 para no "calibrar" un movimiento.
int validarCentro(int centro, const char *nombre) {
    if (centro < 1200 || centro > 2900) {
        Serial.printf("⚠️ Centro de %s fuera de rango (%d). Revisar cableado. Se usa 2048.\n",
                      nombre, centro);
        return 2048;
    }
    return centro;
}

// --- SENSIBILIDAD (EXPO) ---
// Curva tipo control RC: cerca del centro la respuesta es suave y al final del
// recorrido llega igual al máximo. 0.0 = lineal, 1.0 = muy suave al centro.
const float EXPO_VELOCIDAD = 0.6;
const float EXPO_DIRECCION = 0.5;

// Convierte la lectura cruda (0-4095) a un valor entre -limite y +limite,
// con 0 dentro de la zona muerta alrededor del centro medido
int mapearEje(int crudo, int centro, int limite, float expo) {
    float n; // posición normalizada fuera de la zona muerta: -1.0 a 1.0
    if (crudo > centro + ZONA_MUERTA) {
        n = (float)(crudo - centro - ZONA_MUERTA) / (4095 - centro - ZONA_MUERTA);
    } else if (crudo < centro - ZONA_MUERTA) {
        n = -(float)(centro - ZONA_MUERTA - crudo) / (centro - ZONA_MUERTA);
    } else {
        return 0; // Zona Muerta
    }
    n = constrain(n, -1.0f, 1.0f);
    float curva = expo * n * n * n + (1.0f - expo) * n;
    return (int)lroundf(curva * limite);
}

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (status == ESP_NOW_SEND_SUCCESS) {
        digitalWrite(LED_PIN, HIGH);
        delay(5); 
        digitalWrite(LED_PIN, LOW);
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
    if (len != sizeof(TelemetriaDatos)) return;
    TelemetriaDatos t;
    memcpy(&t, datos, sizeof(t));
    if (t.clave != CLAVE_TELEMETRIA) return;
    bateriaAuto_mV = t.bateria_mV;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    analogReadResolution(12); // Lee de 0 a 4095

    // Calibración del centro de los joysticks (deben estar sueltos al encender)
    centroVel = leerPromedio(PIN_JOYSTICK_VELOCIDAD, 64, 2);
    centroAng = leerPromedio(PIN_JOYSTICK_ANGULO, 64, 2);

    WiFi.mode(WIFI_STA);
    
    Serial.println("\n==========================================");
    Serial.println("       TRANSMISOR (CONTROL - TX)          ");
    Serial.print  (">> MAC Transmisor: ");
    Serial.println(WiFi.macAddress());
    Serial.printf(">> Centro joystick velocidad: %d | dirección: %d\n", centroVel, centroAng);
    Serial.println("==========================================\n");
    centroVel = validarCentro(centroVel, "velocidad");
    centroAng = validarCentro(centroAng, "dirección");
    myData.clave = CLAVE_CDIO;

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_send_cb(OnDataSent);
    esp_now_register_recv_cb(OnDataRecv);

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
    // Promedio de varias lecturas: el ADC del ESP32 es ruidoso (sobre todo con WiFi)
    int crudo_vel = leerPromedio(PIN_JOYSTICK_VELOCIDAD, 16, 0);
    int crudo_ang = leerPromedio(PIN_JOYSTICK_ANGULO, 16, 0);
    
    // 1. PROCESAR VELOCIDAD (-255 a 255, 0 = Detenido)
    myData.velocidad = mapearEje(crudo_vel, centroVel, 255, EXPO_VELOCIDAD);

    // 2. PROCESAR ÁNGULO DE DIRECCIÓN (-40° a 40°, 0° = Recto)
    myData.angulo = mapearEje(crudo_ang, centroAng, 40, EXPO_DIRECCION);
    
    // Mostramos lecturas crudas, velocidad y ángulo relativo (-40° a +40°)
    int bat_mV = bateriaAuto_mV;
    if (bat_mV >= 0) {
        Serial.printf("Crudo vel: %4d ang: %4d -> Vel: %4d | Ang: %3d° | Batería auto: %.2f V\n",
                      crudo_vel, crudo_ang, myData.velocidad, myData.angulo, bat_mV / 1000.0f);
    } else {
        Serial.printf("Crudo vel: %4d ang: %4d -> Vel: %4d | Ang: %3d° | Batería auto: --\n",
                      crudo_vel, crudo_ang, myData.velocidad, myData.angulo);
    }
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    delay(50); 
}
