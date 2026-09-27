#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// Estructura para recibir los comandos (Debe coincidir exactamente con la del control remoto)
typedef struct struct_message {
    int x_axis;    // Eje X del joystick (ej. -100 a 100)
    int y_axis;    // Eje Y del joystick (ej. -100 a 100)
    bool button_a; // Botón de acción A (ej. encender luces, turbo, etc)
    bool button_b; // Botón de acción B
} struct_message;

// Variable global para almacenar los datos recibidos
struct_message myData;

// Pines sugeridos para control de motores (Puente H como L298N o TB6612FNG)
// #define MOTOR_LEFT_F 4
// #define MOTOR_LEFT_B 5
// #define MOTOR_RIGHT_F 6
// #define MOTOR_RIGHT_B 7

// Callback que se ejecuta automáticamente cuando se reciben datos
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    memcpy(&myData, incomingData, sizeof(myData));
    
    Serial.print("=== Datos Recibidos (");
    Serial.print(len);
    Serial.println(" bytes) ===");
    
    Serial.print("Eje X (Giro): "); Serial.println(myData.x_axis);
    Serial.print("Eje Y (Avance): "); Serial.println(myData.y_axis);
    Serial.print("Boton A: "); Serial.println(myData.button_a);
    Serial.print("Boton B: "); Serial.println(myData.button_b);
    Serial.println("===============================\n");

    // AQUÍ VA TU LÓGICA DE CONTROL (FASE DE IMPLEMENTACIÓN - CDIO)
    // Ejemplo de pseudocódigo:
    // si myData.y_axis > 50 -> Mover motores hacia adelante
    // si myData.y_axis < -50 -> Mover motores hacia atrás
    // si myData.x_axis > 50 -> Girar a la derecha
}

void setup() {
    // Inicializar monitor serie para ver los mensajes
    Serial.begin(115200);
    delay(2000); // Dar tiempo a que el monitor serie abra

    Serial.println("\n\n--- INICIANDO SISTEMA DEL CARRITO ---");

    // Configurar ESP32 en modo estación (Station) requerido para ESP-NOW
    WiFi.mode(WIFI_STA);

    // ¡PASO CRUCIAL PARA LA COMUNICACIÓN!
    // Imprimir la dirección MAC de este ESP32. 
    // Tienes que copiar esta dirección y pegarla en el código del control remoto.
    Serial.print("***********************************\n");
    Serial.print(">> MAC ADDRESS DE ESTE CARRITO: ");
    Serial.println(WiFi.macAddress());
    Serial.print("***********************************\n");

    // Inicializar el protocolo ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error fatal: No se pudo inicializar ESP-NOW");
        return;
    }

    // Registrar la función de callback para recibir datos
    esp_now_register_recv_cb(OnDataRecv);
    
    Serial.println("ESP-NOW Inicializado correctamente. Esperando comandos...");
}

void loop() {
    // En ESP-NOW, la recepción de datos trabaja en segundo plano mediante eventos (callbacks).
    // Este loop puede estar vacío, pero podrías incluir una lógica de seguridad o 'fail-safe'
    
    // Ejemplo: Si no recibimos datos en X milisegundos, detener los motores por seguridad
    
    delay(100); 
}
