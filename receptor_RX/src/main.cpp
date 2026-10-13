#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <ESP32Servo.h>

const int LED_PIN = 2;

const int PIN_SERVO_DIR = 13;
const uint8_t PIN_EN = 27;    // R_EN y L_EN
const uint8_t PIN_RPWM = 25;  
const uint8_t PIN_LPWM = 26;  

Servo servoDireccion;

typedef struct MensajeDatos {
    int velocidad; // -255 a 255 (0 = detenido)
    int angulo;    // -40 a 40 (0 = recto)
} MensajeDatos;

MensajeDatos datosEntrantes;

// --- FAILSAFE ---
// Si no llegan paquetes en este tiempo, se detiene el motor y se centra la dirección
const unsigned long TIEMPO_FAILSAFE_MS = 300;
volatile unsigned long ultimoPaqueteMs = 0;
volatile bool enlaceActivo = false;

void detenerMotor() {
    digitalWrite(PIN_EN, LOW);
    ledcWrite(0, 0);
    ledcWrite(1, 0);
}

void OnDataRecv(const uint8_t *mac, const uint8_t *datos, int len) {
    // Ignora paquetes que no tengan el tamaño exacto de la estructura
    if (len != sizeof(MensajeDatos)) return;

    digitalWrite(LED_PIN, HIGH);
    memcpy(&datosEntrantes, datos, sizeof(datosEntrantes));
    ultimoPaqueteMs = millis();
    enlaceActivo = true;

    int velocidad_final = constrain(datosEntrantes.velocidad, -255, 255);
    int angulo_relativo = datosEntrantes.angulo; // Rango de -40° a +40°

    // 1. SERVO DE DIRECCIÓN
    // El servo físico entiende de 0° a 180° (donde 90° es el centro).
    // Con 90 + angulo_relativo:
    // -40° -> 50° (Giro a un lado)
    //   0° -> 90° (Ruedas perfectamente rectas)
    // +40° -> 130° (Giro al otro lado)
    int angulo_servo = constrain(90 + angulo_relativo, 50, 130);
    servoDireccion.write(angulo_servo);

    Serial.printf("Recibido -> Vel: %4d | Dirección: %3d° (Servo: %3d°)\n", 
                  velocidad_final, angulo_relativo, angulo_servo);

    // 2. MOTOR DE TRACCIÓN (Control por hardware a prueba de fallos)
    if (velocidad_final > 0) {
        // Avanzar
        digitalWrite(PIN_EN, HIGH);      // Habilita el puente H SOLO cuando nos movemos
        ledcWrite(1, 0);                 
        ledcWrite(0, velocidad_final);   
    } else if (velocidad_final < 0) {
        // Retroceder
        digitalWrite(PIN_EN, HIGH);      // Habilita el puente H SOLO cuando nos movemos
        ledcWrite(0, 0);                 
        ledcWrite(1, abs(velocidad_final)); 
    } else {
        // Detenido absoluto (corta la energía del motor)
        detenerMotor();
    }

    digitalWrite(LED_PIN, LOW);
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, LOW);

    // --- CONFIGURACIÓN SEGURA DEL MOTOR ---
    pinMode(PIN_EN, OUTPUT);
    digitalWrite(PIN_EN, LOW); // Iniciar con el motor apagado

    ledcSetup(0, 5000, 8); 
    ledcSetup(1, 5000, 8); 
    ledcAttachPin(PIN_RPWM, 0);
    ledcAttachPin(PIN_LPWM, 1);

    ledcWrite(0, 0);
    ledcWrite(1, 0);
    // --------------------------------------

    // Inicializar Servo
    servoDireccion.setPeriodHertz(50);
    ESP32PWM::allocateTimer(2); 
    servoDireccion.attach(PIN_SERVO_DIR, 500, 2500);
    servoDireccion.write(90); // 90° físicos = 0° de dirección (recto)

    WiFi.mode(WIFI_STA);
    
    Serial.println("\n==========================================");
    Serial.println("       RECEPTOR (AUTO - RX) ACTIVO        ");
    Serial.print  (">> MI MAC ADDRESS ES: ");
    Serial.println(WiFi.macAddress());
    Serial.println(">> Esperando paquetes de control...");
    Serial.println("==========================================\n");

    if (esp_now_init() != ESP_OK) {
        Serial.println("❌ Error inicializando ESP-NOW");
        return;
    }
    
    esp_now_register_recv_cb(OnDataRecv);
}

void loop() {
    // FAILSAFE: si se pierde la señal del control, el auto se detiene
    if (enlaceActivo && millis() - ultimoPaqueteMs > TIEMPO_FAILSAFE_MS) {
        enlaceActivo = false;
        detenerMotor();
        servoDireccion.write(90);
        Serial.println("⚠️ Sin señal del control: motor detenido (failsafe)");
    }
    delay(10);
}
