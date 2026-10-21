#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_system.h>
#include <ESP32Servo.h>

const int LED_PIN = 2;

const int PIN_SERVO_DIR = 13;
const uint8_t PIN_EN = 27;    // R_EN y L_EN
const uint8_t PIN_RPWM = 25;
const uint8_t PIN_LPWM = 26;

// --- BATERÍA ---
// Divisor de voltaje: Batería+ -> 50 kΩ -> GPIO 34 -> 10 kΩ -> GND
// Vpin = Vbat * 10k / (50k + 10k) = Vbat / 6  (3S llena 12,6 V -> 2,1 V en el pin)
const int PIN_BATERIA = 34;  // ADC1: funciona con el WiFi activo
const float FACTOR_DIVISOR = (50000.0f + 10000.0f) / 10000.0f;
const float VOLTAJE_MIN_CELDA = 3.5f;  // aviso de batería baja por celda
const unsigned long PERIODO_BATERIA_MS = 500;

Servo servoDireccion;

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el TX).
// Evita que otro ESP32 cercano que use ESP-NOW mueva el auto.
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = recto)
} MensajeDatos;

// Telemetría que el RX envía de vuelta al control (TX)
const int CLAVE_TELEMETRIA = 0xCD11;

typedef struct TelemetriaDatos {
    int clave;       // CLAVE_TELEMETRIA
    int bateria_mV;  // voltaje de la batería en milivoltios
} TelemetriaDatos;

uint8_t macControl[6];
bool macControlConocida = false;
bool controlEmparejado = false;

// Datos compartidos entre el callback de ESP-NOW (tarea WiFi) y el loop
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
MensajeDatos ultimoMensaje = {CLAVE_CDIO, 0, 0};
bool hayPaqueteNuevo = false;
unsigned long ultimoPaqueteMs = 0;

// --- FAILSAFE ---
// Si no llegan paquetes en este tiempo, se detiene el motor y se centra la dirección
const unsigned long TIEMPO_FAILSAFE_MS = 300;
bool enlaceActivo = false;

// --- DIRECCIÓN ---
// El TX envía -40 a +40; aquí se escala al recorrido real del servo.
// GIRO_MAX_SERVO: grados que gira el servo a cada lado del centro. Subirlo da
// más giro; si el servo zumba o se traba en los extremos, bajarlo.
// CENTRO_SERVO: ajuste fino para que las ruedas queden rectas.
const int GIRO_MAX_SERVO = 60;
const int CENTRO_SERVO = 90;

// --- RAMPA DE VELOCIDAD ---
// Limita el cambio de PWM por ciclo para evitar picos de corriente que
// bajan el voltaje y reinician el ESP32 (brownout). Frenar es inmediato.
const int PASO_RAMPA = 15;  // cada 10 ms -> 0 a 255 en ~170 ms
int velocidadObjetivo = 0;
int velocidadActual = 0;

void detenerMotor() {
    digitalWrite(PIN_EN, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
}

void aplicarMotor(int velocidad) {
    if (velocidad > 0) {
        // Avanzar: primero se fijan los PWM y después se habilita el puente H
        ledcWrite(1, 0);
        ledcWrite(0, velocidad);
        digitalWrite(PIN_EN, HIGH);
    } else if (velocidad < 0) {
        // Retroceder
        ledcWrite(0, 0);
        ledcWrite(1, -velocidad);
        digitalWrite(PIN_EN, HIGH);
    } else {
        // Detenido absoluto (corta la energía del motor)
        detenerMotor();
    }
}

void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
    // Ignora paquetes que no tengan el tamaño exacto de la estructura
    if (len != sizeof(MensajeDatos)) return;

    MensajeDatos m;
    memcpy(&m, datos, sizeof(m));
    // Ignora paquetes de otros dispositivos ESP-NOW
    if (m.clave != CLAVE_CDIO) return;

    // Solo se guardan los datos; los actuadores se controlan desde el loop
    portENTER_CRITICAL(&mux);
    ultimoMensaje = m;
    hayPaqueteNuevo = true;
    ultimoPaqueteMs = millis();
    if (!macControlConocida) {
        memcpy(macControl, mac, 6);  // se usa para enviar la telemetría de vuelta
        macControlConocida = true;
    }
    portEXIT_CRITICAL(&mux);
}

float leerBateria() {
    // analogReadMilliVolts usa la calibración de fábrica del ADC
    long suma = 0;
    for (int i = 0; i < 16; i++) suma += analogReadMilliVolts(PIN_BATERIA);
    return (suma / 16.0f) * FACTOR_DIVISOR / 1000.0f;
}

void enviarTelemetria(float voltaje) {
    bool conocida;
    portENTER_CRITICAL(&mux);
    conocida = macControlConocida;
    portEXIT_CRITICAL(&mux);
    if (!conocida) return;

    if (!controlEmparejado) {
        esp_now_peer_info_t peer;
        memset(&peer, 0, sizeof(peer));
        memcpy(peer.peer_addr, macControl, 6);
        peer.channel = 0;
        peer.encrypt = false;
        if (esp_now_add_peer(&peer) != ESP_OK) return;
        controlEmparejado = true;
    }

    TelemetriaDatos t = {CLAVE_TELEMETRIA, (int)(voltaje * 1000)};
    esp_now_send(macControl, (uint8_t *)&t, sizeof(t));
}

void setup() {
    // --- CONFIGURACIÓN SEGURA DEL MOTOR (lo PRIMERO al arrancar) ---
    // Hasta este punto los pines están flotando; mientras más rápido se fijen
    // en LOW, menos tiempo puede el BTS7960 activarse solo.
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW);

    ledcSetup(0, 5000, 8);
    ledcSetup(1, 5000, 8);
    ledcAttachPin(PIN_RPWM, 0);
    ledcAttachPin(PIN_LPWM, 1);

    ledcWrite(0, 0);
    ledcWrite(1, 0);
    // --------------------------------------

    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    analogSetPinAttenuation(PIN_BATERIA, ADC_11db);  // rango hasta ~3,1 V

    // Inicializar Servo (timer 2 -> no choca con los canales PWM 0 y 1 del motor)
    servoDireccion.setPeriodHertz(50);
    ESP32PWM::allocateTimer(2);
    servoDireccion.attach(PIN_SERVO_DIR, 500, 2500);
    servoDireccion.write(CENTRO_SERVO); // ruedas rectas

    WiFi.mode(WIFI_STA);

    Serial.println("\n==========================================");
    Serial.println("       RECEPTOR (AUTO - RX) ACTIVO        ");
    Serial.print  (">> MI MAC ADDRESS ES: ");
    Serial.println(WiFi.macAddress());
    Serial.printf (">> Causa del último reinicio: %d (9 = brownout / caída de voltaje)\n",
                   (int)esp_reset_reason());
    Serial.println(">> Esperando paquetes de control...");
    Serial.println("==========================================\n");

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicializando ESP-NOW");
        return;
    }

    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    MensajeDatos m;
    bool nuevo;
    unsigned long tPaquete;

    portENTER_CRITICAL(&mux);
    m = ultimoMensaje;
    nuevo = hayPaqueteNuevo;
    hayPaqueteNuevo = false;
    tPaquete = ultimoPaqueteMs;
    portEXIT_CRITICAL(&mux);

    if (nuevo) {
        enlaceActivo = true;
        digitalWrite(LED_PIN, HIGH);

        velocidadObjetivo = constrain(m.velocidad, -255, 255);

        // SERVO DE DIRECCIÓN
        // -40 -> CENTRO - GIRO_MAX | 0 -> CENTRO (recto) | +40 -> CENTRO + GIRO_MAX
        int angulo_dir = constrain(m.angulo, -40, 40);
        int angulo_servo = constrain(CENTRO_SERVO + map(angulo_dir, -40, 40, -GIRO_MAX_SERVO, GIRO_MAX_SERVO),
                                     0, 180);
        servoDireccion.write(angulo_servo);

        Serial.printf("Recibido -> Vel: %4d | Dirección: %3d° (Servo: %3d°)\n",
                      velocidadObjetivo, m.angulo, angulo_servo);

        digitalWrite(LED_PIN, LOW);
    }

    // FAILSAFE: si se pierde la señal del control, el auto se detiene
    if (enlaceActivo && millis() - tPaquete > TIEMPO_FAILSAFE_MS) {
        enlaceActivo = false;
        velocidadObjetivo = 0;
        servoDireccion.write(CENTRO_SERVO);
        Serial.println("⚠️ Sin señal del control: motor detenido (failsafe)");
    }

    // MOTOR DE TRACCIÓN con rampa (detenerse es inmediato)
    if (velocidadObjetivo == 0) {
        velocidadActual = 0;
    } else if (velocidadActual < velocidadObjetivo) {
        velocidadActual = min(velocidadActual + PASO_RAMPA, velocidadObjetivo);
    } else if (velocidadActual > velocidadObjetivo) {
        velocidadActual = max(velocidadActual - PASO_RAMPA, velocidadObjetivo);
    }
    aplicarMotor(velocidadActual);

    // BATERÍA: medir, mostrar y enviar al control cada PERIODO_BATERIA_MS
    static unsigned long ultimaBateriaMs = 0;
    if (millis() - ultimaBateriaMs >= PERIODO_BATERIA_MS) {
        ultimaBateriaMs = millis();
        float voltaje = leerBateria();
        int celdas = voltaje > 8.8f ? 3 : 2;  // 2S llena = 8,4 V | 3S descargada = 9,9 V
        Serial.printf("Batería: %.2f V (%dS, %.2f V/celda)%s\n", voltaje, celdas, voltaje / celdas,
                      voltaje / celdas < VOLTAJE_MIN_CELDA ? "  ⚠️ BATERÍA BAJA" : "");
        enviarTelemetria(voltaje);
    }

    delay(10);
}
