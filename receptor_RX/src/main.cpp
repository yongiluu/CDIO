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

Servo servoDireccion;

// Clave que identifica los paquetes de NUESTRO control (debe ser igual en el TX).
// Evita que otro ESP32 cercano que use ESP-NOW mueva el auto.
const int CLAVE_CDIO = 0xCD10;

typedef struct MensajeDatos {
    int clave;     // CLAVE_CDIO
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = recto)
} MensajeDatos;

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
    portEXIT_CRITICAL(&mux);
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

    delay(10);
}
