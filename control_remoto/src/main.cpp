#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ¡CAMBIA ESTA DIRECCIÓN MAC!
// Reemplaza los números de abajo con la MAC Address que te dio el código del carrito.
// Ejemplo: Si el carrito dice 24:DC:C3:AE:B2:10, lo cambias a {0x24, 0xDC, 0xC3, 0xAE, 0xB2, 0x10}
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Estructura para enviar los comandos (Debe coincidir exactamente con la del carrito)
typedef struct struct_message {
    int x_axis;
    int y_axis;
    bool button_a;
    bool button_b;
} struct_message;

// Variable global para los datos a enviar
struct_message myData;
esp_now_peer_info_t peerInfo;

// Pines para tus componentes de control (Joystick o potenciómetros, y botones)
// #define PIN_JOY_X 4
// #define PIN_JOY_Y 5
// #define PIN_BTN_A 6
// #define PIN_BTN_B 7

// Callback que se ejecuta cuando se envían los datos para confirmar si llegaron o no
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.print("Estado del ultimo envio: ");
    if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.println("Entregado con Exito al Carrito");
    } else {
        Serial.println("Fallo en la entrega (Posiblemente fuera de rango o apagado)");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000); // Dar tiempo a que el monitor serie abra

    Serial.println("\n\n--- INICIANDO CONTROL REMOTO ---");

    // Configurar ESP32 en modo estación (Station)
    WiFi.mode(WIFI_STA);

    // Inicializar ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error fatal: No se pudo inicializar ESP-NOW");
        return;
    }

    // Registrar la función de callback para revisar el estado del envío
    esp_now_register_send_cb(OnDataSent);

    // Configurar el "Peer" (Emparejamiento con el carrito)
    memset(&peerInfo, 0, sizeof(peerInfo)); // Limpiar estructura por seguridad
    memcpy(peerInfo.peer_addr, broadcastAddress, 6); // Asignar la MAC del carrito
    peerInfo.channel = 0; // Usar el canal WiFi por defecto
    peerInfo.encrypt = false; // Sin encriptación por ahora para mayor velocidad y simplicidad

    // Añadir el peer a la lista de emparejamientos
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Fallo al emparejar con el carrito");
        return;
    }
    
    Serial.println("Control configurado. Listo para enviar.");
}

void loop() {
    // Aquí es donde leerías los valores de tu hardware de control físico.
    // Para probar el lunes sin tener todo armado, enviaremos datos simulados
    // para verificar que la comunicación inalámbrica es estable.

    // Descomentar cuando tengas el joystick conectado:
    // myData.x_axis = analogRead(PIN_JOY_X);
    // myData.y_axis = analogRead(PIN_JOY_Y);
    // myData.button_a = digitalRead(PIN_BTN_A);
    // myData.button_b = digitalRead(PIN_BTN_B);

    // --- DATOS SIMULADOS PARA PRUEBA INICIAL ---
    myData.x_axis = random(-100, 100);
    myData.y_axis = random(-100, 100);
    myData.button_a = random(0, 2);
    myData.button_b = random(0, 2);
    // -------------------------------------------

    Serial.println("\nEnviando datos al carrito...");
    
    // Enviar el mensaje vía ESP-NOW a la dirección MAC configurada
    esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &myData, sizeof(myData));

    if (result == ESP_OK) {
        Serial.println("Mensaje mandado correctamente a la red WiFi");
    } else {
        Serial.println("Hubo un error enviando el dato a la red WiFi");
    }

    // Retraso de envío. Un delay(100) envía 10 comandos por segundo.
    // Dependiendo de lo ágil que quieras que sea tu carrito, puedes bajarlo a 50 o subirlo a 200.
    delay(500); 
}
