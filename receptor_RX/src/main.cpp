#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
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
// Divisor de voltaje: Batería+ -> R alta -> GPIO 34 -> 10 kΩ -> GND
// Calibrado con multímetro: con factor 6 (50k/10k) marcaba 10,29 V con la
// batería real en ~11,3 V. El factor medido es 6,6 (R alta de 56 kΩ):
// Vpin = Vbat * 10k / (56k + 10k) = Vbat / 6,6  (3S llena 12,6 V -> 1,9 V en el pin)
// Si vuelve a diferir del multímetro: FACTOR_nuevo = FACTOR * V_multímetro / V_mostrado
const int PIN_BATERIA = 34;  // ADC1: funciona con el WiFi activo
const float FACTOR_DIVISOR = 6.6f;
const unsigned long PERIODO_BATERIA_MS = 500;

Servo servoDireccion;

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el TX).
// Evita que otro ESP32 cercano que use ESP-NOW mueva el auto.
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // grados del servo respecto al centro: -38 a 38 (0 = recto)
    int freno;     // 1 = fast brake (click en el stick de potencia), 0 = normal
} MensajeDatos;

// Telemetría que el RX envía de vuelta al control (TX)
const int CLAVE_TELEMETRIA = 0xCD11;

typedef struct TelemetriaDatos {
    int clave;          // CLAVE_TELEMETRIA
    int bateria_mV;     // voltaje de la batería en milivoltios
    int causaReinicio;  // esp_reset_reason() del último arranque (9 = brownout)
    int encendido_ms;   // millis() del RX: si baja, el RX se reinició
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
// El TX envía el ángulo en grados reales del servo respecto al centro (-38 a 38);
// aquí solo se limita a GIRO_MAX_SERVO y se suma al centro.
// CENTRO_SERVO: posición del servo con las ruedas rectas. Con el servo montado
// en el auto y ajustado con el trim (+/-), las ruedas quedan rectas en 50°.
// GIRO_MAX_SERVO: grados que gira el servo a cada lado del centro. Con más de
// 38° la dirección puede topar con el chasis: si el servo zumba o se fuerza en
// los extremos, bajar este valor (y ANGULO_MAX en el TX). Antes estaba en 30°.
// Recorrido resultante: servo de 12° a 88°.
const int GIRO_MAX_SERVO = 38;
const int CENTRO_SERVO = 50;
// AJUSTE FINO DEL CENTRO (trim): con el monitor serie del RX abierto, cada '+'
// suma 1° al centro y cada '-' resta 1°, al instante. Cuando las ruedas queden
// rectas, copiar el valor que muestra el monitor en CENTRO_SERVO.
int centroServo = CENTRO_SERVO;
// El servo trabaja a 50 Hz (un pulso cada 20 ms): se le envía una orden por
// pulso, no al ritmo del lazo del motor (10 ms).
// INTERPOLACIÓN: PASO_SERVO son los grados que avanza por orden. El MG996R a
// fondo hace ~7° cada 20 ms y pide su pico máximo de corriente (> 2 A), lo que
// provocaba brownout. Con 2° va a ~1/3 de su velocidad y el pico de arranque
// baja: 0 -> 30° en ~300 ms. Más alto = gira más rápido pero pide más corriente.
const unsigned long PERIODO_SERVO_MS = 20;
const int PASO_SERVO = 2;
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

    TelemetriaDatos t = {CLAVE_TELEMETRIA, (int)(voltaje * 1000),
                         (int)esp_reset_reason(), (int)millis()};
    esp_now_send(macControl, (uint8_t *)&t, sizeof(t));
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
    delay(100);  // corto: si el auto se reinicia, vuelve a responder rápido

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    analogSetPinAttenuation(PIN_BATERIA, ADC_11db);  // rango hasta ~3,1 V

    // Inicializar Servo (timer 2 -> no choca con los canales PWM 0 y 1 del motor)
    servoDireccion.setPeriodHertz(50);
    ESP32PWM::allocateTimer(2);
    servoDireccion.attach(PIN_SERVO_DIR, 500, 2500);
    servoDireccion.write(centroServo); // ruedas rectas

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

        bool frenoPedido = (m.freno != 0);
        if (frenoPedido && !frenando) Serial.println("🛑 FAST BRAKE activado");
        if (!frenoPedido && frenando) Serial.println("✅ Fast brake liberado");
        frenando = frenoPedido;
        if (frenando) velocidadObjetivo = 0;

        // SERVO DE DIRECCIÓN
        // -38 -> CENTRO - 38 (12°) | 0 -> CENTRO (50°, recto) | +38 -> CENTRO + 38 (88°)
        int angulo_dir = constrain(m.angulo, -GIRO_MAX_SERVO, GIRO_MAX_SERVO);
        servoObjetivo = constrain(centroServo + angulo_dir, 0, 180);

        Serial.printf("Recibido -> Vel: %4d | Dirección: %3d° (Servo objetivo: %3d° | actual: %3d°)%s\n",
                      velocidadObjetivo, m.angulo, servoObjetivo, servoActual,
                      frenando ? " | FRENANDO" : "");

        digitalWrite(LED_PIN, LOW);
    }

    // AJUSTE FINO DEL CENTRO desde el monitor serie: '+' suma 1°, '-' resta 1°
    while (Serial.available() > 0) {
        char c = Serial.read();
        int delta = (c == '+') ? 1 : (c == '-') ? -1 : 0;
        if (delta != 0) {
            centroServo = constrain(centroServo + delta, GIRO_MAX_SERVO, 180 - GIRO_MAX_SERVO);
            servoObjetivo = constrain(servoObjetivo + delta, 0, 180);
            Serial.printf(">>> CENTRO DEL SERVO: %d°  (copiar este valor en CENTRO_SERVO)\n", centroServo);
        }
    }

    // FAILSAFE: si se pierde la señal del control, el auto se detiene
    if (enlaceActivo && millis() - tPaquete > TIEMPO_FAILSAFE_MS) {
        enlaceActivo = false;
        velocidadObjetivo = 0;
        frenando = false;
        servoObjetivo = centroServo;
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

    // SERVO a 50 Hz: una orden cada PERIODO_SERVO_MS, acercándose al objetivo
    // de a PASO_SERVO grados, y solo cuando el ángulo cambia
    static unsigned long ultimoServoMs = 0;
    if (servoActual != servoObjetivo && millis() - ultimoServoMs >= PERIODO_SERVO_MS) {
        ultimoServoMs = millis();
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
