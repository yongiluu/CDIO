#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#if __has_include(<esp_mac.h>)
#include <esp_mac.h>   // esp_read_mac (ESP-IDF 5 / core 3.x)
#else
#include <esp_system.h>
#endif
#include <esp_system.h>
#include <ESP32Servo.h>

// Compatible con el core Arduino-ESP32 2.x y 3.x (PlatformIO espressif32 >= 7).
// En el 3.x cambió la API de PWM (ledc) y el callback de recepción de ESP-NOW.
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
#define CORE_ESP32_V3 1
#else
#define CORE_ESP32_V3 0
#endif

const int LED_PIN = 2;

const int PIN_SERVO_DIR = 13;
const uint8_t PIN_EN = 27;    // R_EN y L_EN
const uint8_t PIN_RPWM = 25;
const uint8_t PIN_LPWM = 26;

// Canales PWM del motor (timer 0). El servo usa el timer 2 (canal 4), así no chocan.
const uint8_t CANAL_RPWM = 0;
const uint8_t CANAL_LPWM = 1;
const uint32_t FRECUENCIA_PWM = 5000;  // Hz
const uint8_t RESOLUCION_PWM = 8;      // 0 a 255

// --- BATERÍA ---
// Divisor de voltaje: Batería+ -> 50 kΩ -> GPIO 34 -> 10 kΩ -> GND
// Vpin = Vbat * 10k / (50k + 10k) = Vbat / 6  (3S llena 12,6 V -> 2,1 V en el pin)
const int PIN_BATERIA = 34;  // ADC1: funciona con el WiFi activo
const float FACTOR_DIVISOR = (50000.0f + 10000.0f) / 10000.0f;
const unsigned long PERIODO_BATERIA_MS = 500;

Servo servoDireccion;

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el TX).
// Evita que otro ESP32 cercano que use ESP-NOW mueva el auto.
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = recto)
    int freno;     // 1 = fast brake (click en el stick de potencia), 0 = normal
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
MensajeDatos ultimoMensaje = {CLAVE_CDIO, 0, 0, 0};
bool hayPaqueteNuevo = false;
unsigned long ultimoPaqueteMs = 0;

// --- FAILSAFE ---
// Si no llegan paquetes en este tiempo, se detiene el motor y se centra la dirección
const unsigned long TIEMPO_FAILSAFE_MS = 300;
bool enlaceActivo = false;

// --- FAST BRAKE ---
// true mientras el control pide frenar (click en el stick de potencia)
bool frenando = false;

// --- DIRECCIÓN ---
// El TX envía -40 a +40; aquí se escala al recorrido real del servo.
// GIRO_MAX_SERVO: grados que gira el servo a cada lado del centro. Subirlo da
// más giro; si el servo zumba o se traba en los extremos, bajarlo.
// CENTRO_SERVO: ajuste fino para que las ruedas queden rectas.
// IMPORTANTE: el servo no debe llegar al tope mecánico de la dirección. Si choca,
// se traba, consume mucha corriente (> 2 A) y deja de responder.
const int GIRO_MAX_SERVO = 45;
const int CENTRO_SERVO = 90;
// Velocidad máxima del servo: grados por ciclo de 10 ms (5 -> 45° en ~90 ms).
// Evita los saltos bruscos que generan picos de corriente.
const int PASO_SERVO = 5;
int servoObjetivo = CENTRO_SERVO;
int servoActual = CENTRO_SERVO;

// --- RAMPA DE VELOCIDAD ---
// Limita el cambio de PWM por ciclo para evitar picos de corriente que
// bajan el voltaje y reinician el ESP32 (brownout). Frenar es inmediato.
const int PASO_RAMPA = 15;  // cada 10 ms -> 0 a 255 en ~170 ms
int velocidadObjetivo = 0;
int velocidadActual = 0;

// Escribe el PWM de un pin del motor (en el core 3.x ledcWrite recibe el pin,
// en el 2.x recibe el canal)
void pwmMotor(uint8_t pin, uint8_t canal, uint32_t duty) {
#if CORE_ESP32_V3
    (void)canal;
    ledcWrite(pin, duty);
#else
    (void)pin;
    ledcWrite(canal, duty);
#endif
}

void configurarPwmMotor() {
#if CORE_ESP32_V3
    ledcAttachChannel(PIN_RPWM, FRECUENCIA_PWM, RESOLUCION_PWM, CANAL_RPWM);
    ledcAttachChannel(PIN_LPWM, FRECUENCIA_PWM, RESOLUCION_PWM, CANAL_LPWM);
#else
    ledcSetup(CANAL_RPWM, FRECUENCIA_PWM, RESOLUCION_PWM);
    ledcSetup(CANAL_LPWM, FRECUENCIA_PWM, RESOLUCION_PWM);
    ledcAttachPin(PIN_RPWM, CANAL_RPWM);
    ledcAttachPin(PIN_LPWM, CANAL_LPWM);
#endif
    pwmMotor(PIN_RPWM, CANAL_RPWM, 0);
    pwmMotor(PIN_LPWM, CANAL_LPWM, 0);
}

// Rueda libre: puente H deshabilitado, el motor se detiene solo por roce
void detenerMotor() {
    digitalWrite(PIN_EN, LOW);
    pwmMotor(PIN_RPWM, CANAL_RPWM, 0);
    pwmMotor(PIN_LPWM, CANAL_LPWM, 0);
}

// FAST BRAKE (freno dinámico): RPWM = LPWM = 0 con el puente H habilitado.
// Los dos MOSFET inferiores del BTS7960 quedan conduciendo y ponen en corto
// los bornes del motor (M+ y M- a B-). La fuerza contraelectromotriz genera
// una corriente que se opone al giro y el motor se detiene mucho más rápido
// que en rueda libre.
void frenarMotor() {
    pwmMotor(PIN_RPWM, CANAL_RPWM, 0);
    pwmMotor(PIN_LPWM, CANAL_LPWM, 0);
    digitalWrite(PIN_EN, HIGH);
}

void aplicarMotor(int velocidad) {
    if (velocidad > 0) {
        // Avanzar: primero se fijan los PWM y después se habilita el puente H
        pwmMotor(PIN_LPWM, CANAL_LPWM, 0);
        pwmMotor(PIN_RPWM, CANAL_RPWM, velocidad);
        digitalWrite(PIN_EN, HIGH);
    } else if (velocidad < 0) {
        // Retroceder
        pwmMotor(PIN_RPWM, CANAL_RPWM, 0);
        pwmMotor(PIN_LPWM, CANAL_LPWM, -velocidad);
        digitalWrite(PIN_EN, HIGH);
    } else {
        // Detenido absoluto (corta la energía del motor)
        detenerMotor();
    }
}

#if CORE_ESP32_V3
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *datos, int len) {
    const uint8_t *mac = info->src_addr;
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
#endif
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

// Imprime la MAC WiFi (STA) leída directo del eFuse. En el core 3.x
// WiFi.macAddress() devuelve 00:00:00:00:00:00 si el WiFi aún no terminó de iniciar.
void imprimirMac() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void setup() {
    // --- CONFIGURACIÓN SEGURA DEL MOTOR (lo PRIMERO al arrancar) ---
    // Hasta este punto los pines están flotando; mientras más rápido se fijen
    // en LOW, menos tiempo puede el BTS7960 activarse solo.
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW);

    configurarPwmMotor();
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
    imprimirMac();
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

        bool frenoPedido = (m.freno != 0);
        if (frenoPedido && !frenando) Serial.println("🛑 FAST BRAKE activado");
        if (!frenoPedido && frenando) Serial.println("✅ Fast brake liberado");
        frenando = frenoPedido;
        if (frenando) velocidadObjetivo = 0;

        // SERVO DE DIRECCIÓN
        // -40 -> CENTRO - GIRO_MAX | 0 -> CENTRO (recto) | +40 -> CENTRO + GIRO_MAX
        int angulo_dir = constrain(m.angulo, -40, 40);
        servoObjetivo = constrain(CENTRO_SERVO + map(angulo_dir, -40, 40, -GIRO_MAX_SERVO, GIRO_MAX_SERVO),
                                  0, 180);

        Serial.printf("Recibido -> Vel: %4d | Dirección: %3d° (Servo objetivo: %3d° | actual: %3d°)%s\n",
                      velocidadObjetivo, m.angulo, servoObjetivo, servoActual,
                      frenando ? " | FRENANDO" : "");

        digitalWrite(LED_PIN, LOW);
    }

    // FAILSAFE: si se pierde la señal del control, el auto se detiene
    if (enlaceActivo && millis() - tPaquete > TIEMPO_FAILSAFE_MS) {
        enlaceActivo = false;
        velocidadObjetivo = 0;
        frenando = false;
        servoObjetivo = CENTRO_SERVO;
        Serial.println("⚠️ Sin señal del control: motor detenido (failsafe)");
    }

    // MOTOR DE TRACCIÓN con rampa (detenerse y frenar es inmediato)
    if (frenando) {
        velocidadActual = 0;
        frenarMotor();
    } else {
        if (velocidadObjetivo == 0) {
            velocidadActual = 0;
        } else if (velocidadActual < velocidadObjetivo) {
            velocidadActual = min(velocidadActual + PASO_RAMPA, velocidadObjetivo);
        } else if (velocidadActual > velocidadObjetivo) {
            velocidadActual = max(velocidadActual - PASO_RAMPA, velocidadObjetivo);
        }
        aplicarMotor(velocidadActual);
    }

    // SERVO con rampa: se acerca al objetivo de a PASO_SERVO grados y solo
    // se actualiza cuando el ángulo cambia
    if (servoActual != servoObjetivo) {
        if (servoActual < servoObjetivo) {
            servoActual = min(servoActual + PASO_SERVO, servoObjetivo);
        } else {
            servoActual = max(servoActual - PASO_SERVO, servoObjetivo);
        }
        servoDireccion.write(servoActual);
    }

    // BATERÍA: medir y enviar al control cada PERIODO_BATERIA_MS
    // (el voltaje y el aviso de batería baja se muestran en el monitor del TX)
    static unsigned long ultimaBateriaMs = 0;
    if (millis() - ultimaBateriaMs >= PERIODO_BATERIA_MS) {
        ultimaBateriaMs = millis();
        enviarTelemetria(leerBateria());
    }

    delay(10);
}
