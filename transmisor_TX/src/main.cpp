#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_idf_version.h>

// Compatible con el core Arduino-ESP32 2.x y 3.x (PlatformIO espressif32 >= 7).
// En ESP-IDF 5.0+ cambió el callback de recepción y en 5.5+ el de envío.
#define ESPNOW_RECV_V5 (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0))
#define ESPNOW_SEND_V55 (ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0))

const int LED_PIN = 2;

// MAC del receptor_RX
uint8_t broadcastAddress[] = {0x68, 0x09, 0x47, 0x9F, 0x19, 0x60};

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el RX)
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // grados del servo respecto al centro: -38 a 38 (0 = ruedas rectas)
    int freno;     // 1 = fast brake (click en el stick de potencia), 0 = normal
} MensajeDatos;

// Telemetría que llega desde el auto (RX)
const int CLAVE_TELEMETRIA = 0xCD11;

typedef struct TelemetriaDatos {
    int clave;          // CLAVE_TELEMETRIA
    int bateria_mV;     // voltaje de la batería en milivoltios
    int causaReinicio;  // esp_reset_reason() del último arranque del RX (9 = brownout)
    int encendido_ms;   // millis() del RX: si baja, el RX se reinició
} TelemetriaDatos;

volatile int bateriaAuto_mV = -1;  // -1 = aún no llega telemetría
volatile unsigned long ultimaTelemetriaMs = 0;
volatile int ultimoEncendidoRx = -1;
volatile int reinicioRxCausa = -1;  // >= 0: se detectó un reinicio del RX con esa causa
const float VOLTAJE_MIN_CELDA = 3.5f;  // aviso de batería baja por celda

MensajeDatos myData;
esp_now_peer_info_t peerInfo;

const int PIN_JOYSTICK_VELOCIDAD = 34; // Y
const int PIN_JOYSTICK_ANGULO = 35;    // X

// --- FAST BRAKE ---
// Botón rojo de freno conectado en PULL-DOWN: un lado del botón a GPIO 25 y el
// otro a 3,3 V. Se usa la resistencia pull-down interna (sirve igual si además
// hay una externa a GND), así que suelto lee LOW y presionado lee HIGH.
// Un click activa el freno y queda activo hasta que pasen FRENO_MIN_MS, se suelte
// el botón y el stick de potencia vuelva al centro. Así un click corto igual frena
// a fondo y el auto no vuelve a acelerar solo al soltar el botón.
const int PIN_BOTON_FRENO = 25;
const unsigned long FRENO_MIN_MS = 500;
bool frenoActivo = false;
unsigned long inicioFrenoMs = 0;

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
// o está mal conectado. En ese caso el control queda BLOQUEADO (envía siempre
// velocidad 0 y dirección recta) hasta reiniciarlo con los joysticks bien.
bool joysticksOk = true;

bool validarCentro(int centro, const char *nombre) {
    if (centro < 1200 || centro > 2900) {
        Serial.printf("❌ Centro de %s fuera de rango (%d). Revisar cableado del joystick.\n",
                      nombre, centro);
        return false;
    }
    return true;
}

// --- SENSIBILIDAD (EXPO) ---
// Curva tipo control RC: cerca del centro la respuesta es suave y al final del
// recorrido llega igual al máximo. 0.0 = lineal, 1.0 = muy suave al centro.
// Velocidad LINEAL: con expo 0.6 casi todo el aumento quedaba en el último
// tramo del stick y la velocidad saltaba de lenta a máxima. Ahora cada parte
// del recorrido suma lo mismo (stick a la mitad = media velocidad).
// La aceleración gradual la hace la rampa del RX (PASO_ACELERAR).
const float EXPO_VELOCIDAD = 0.0;
const float EXPO_DIRECCION = 0.5;

// --- DIRECCIÓN ---
// Giro máximo del servo a cada lado del centro, en grados reales. El RX lo
// limita además con su propio GIRO_MAX_SERVO.
const int ANGULO_MAX = 38;

// --- EXTREMOS DEL JOYSTICK ---
// Con el ESP32 a 3,3 V los joysticks no siempre llegan a 0 / 4095 a fondo.
// El máximo se alcanza MARGEN_EXTREMO cuentas antes del final del recorrido,
// así el stick a fondo siempre entrega el 100 % (dirección = 38° en el servo).
const int MARGEN_EXTREMO = 400;

// Convierte la lectura cruda (0-4095) a un valor entre -limite y +limite,
// con 0 dentro de la zona muerta alrededor del centro medido
int mapearEje(int crudo, int centro, int limite, float expo) {
    float n; // posición normalizada fuera de la zona muerta: -1.0 a 1.0
    const int tope_alto = 4095 - MARGEN_EXTREMO;
    const int tope_bajo = MARGEN_EXTREMO;
    if (crudo > centro + ZONA_MUERTA) {
        n = (float)(crudo - centro - ZONA_MUERTA) / (tope_alto - centro - ZONA_MUERTA);
    } else if (crudo < centro - ZONA_MUERTA) {
        n = -(float)(centro - ZONA_MUERTA - crudo) / (centro - ZONA_MUERTA - tope_bajo);
    } else {
        return 0; // Zona Muerta
    }
    n = constrain(n, -1.0f, 1.0f);
    float curva = expo * n * n * n + (1.0f - expo) * n;
    return (int)lroundf(curva * limite);
}

#if ESPNOW_SEND_V55
void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
#else
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
#endif
    // Sin delay: este callback corre en la tarea del WiFi y no se debe bloquear.
    // El LED queda encendido mientras los envíos llegan bien.
    digitalWrite(LED_PIN, status == ESP_NOW_SEND_SUCCESS ? HIGH : LOW);
}

#if ESPNOW_RECV_V5
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *datos, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
#endif
    if (len != sizeof(TelemetriaDatos)) return;
    TelemetriaDatos t;
    memcpy(&t, datos, sizeof(t));
    if (t.clave != CLAVE_TELEMETRIA) return;
    bateriaAuto_mV = t.bateria_mV;
    ultimaTelemetriaMs = millis();
    // El tiempo encendido del RX volvió a empezar: el auto se reinició
    if (ultimoEncendidoRx >= 0 && t.encendido_ms < ultimoEncendidoRx) {
        reinicioRxCausa = t.causaReinicio;
    }
    ultimoEncendidoRx = t.encendido_ms;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);
    analogReadResolution(12); // Lee de 0 a 4095
    pinMode(PIN_BOTON_FRENO, INPUT_PULLDOWN);

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
    bool velOk = validarCentro(centroVel, "velocidad");
    bool angOk = validarCentro(centroAng, "dirección");
    joysticksOk = velOk && angOk;
    if (!joysticksOk) {
        Serial.println("❌ CONTROL BLOQUEADO: se envía Vel 0 / Ang 0. Revisar joysticks y reiniciar.");
    }
    myData.clave = CLAVE_CDIO;
    myData.freno = 0;

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

    // 2. PROCESAR ÁNGULO DE DIRECCIÓN en grados reales del servo (0° = Recto)
    myData.angulo = mapearEje(crudo_ang, centroAng, ANGULO_MAX, EXPO_DIRECCION);

    // Con la calibración fallida nunca se envía movimiento
    if (!joysticksOk) {
        myData.velocidad = 0;
        myData.angulo = 0;
        Serial.print("[BLOQUEADO] ");
    }

    // 3. FAST BRAKE con el click del stick de potencia
    // (se lee cada 50 ms, lo que ya filtra el rebote del botón)
    bool botonPresionado = (digitalRead(PIN_BOTON_FRENO) == HIGH);
    if (botonPresionado && !frenoActivo) {
        frenoActivo = true;
        inicioFrenoMs = millis();
    } else if (frenoActivo && !botonPresionado && myData.velocidad == 0 &&
               millis() - inicioFrenoMs >= FRENO_MIN_MS) {
        frenoActivo = false;  // botón suelto y stick al centro: se libera el freno
    }
    myData.freno = frenoActivo ? 1 : 0;
    if (frenoActivo) {
        myData.velocidad = 0;  // mientras frena nunca se pide tracción
    }
    
    // AVISOS DEL AUTO: reinicio detectado o telemetría cortada
    int causa = reinicioRxCausa;
    if (causa >= 0) {
        reinicioRxCausa = -1;
        Serial.printf("\n⚠️⚠️ EL AUTO (RX) SE REINICIÓ. Causa: %d%s\n\n", causa,
                      causa == 9 ? " = BROWNOUT (caída de voltaje: revisar alimentación del servo)" : "");
    }
    if (ultimaTelemetriaMs > 0 && millis() - ultimaTelemetriaMs > 1500) {
        Serial.print("[SIN TELEMETRÍA DEL AUTO] ");
    }

    // Monitor: velocidad, ángulo del servo (grados reales), botón, freno y batería
    Serial.printf("Vel: %4d | Ang: %3d° | Botón: %-7s | Freno: %-9s | ",
                  myData.velocidad, myData.angulo,
                  botonPresionado ? "APRETADO" : "suelto",
                  frenoActivo ? "ACTIVO 🛑" : "no");
    int bat_mV = bateriaAuto_mV;
    if (bat_mV >= 0) {
        float voltaje = bat_mV / 1000.0f;
        int celdas = voltaje > 8.8f ? 3 : 2;  // 2S llena = 8,4 V | 3S descargada = 9,9 V
        Serial.printf("Batería auto: %.2f V (%dS, %.2f V/celda)%s\n",
                      voltaje, celdas, voltaje / celdas,
                      voltaje / celdas < VOLTAJE_MIN_CELDA ? "  ⚠️ BATERÍA BAJA" : "");
    } else {
        Serial.println("Batería auto: --");
    }
    
    esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));
    
    delay(50); 
}
