#include <esp_now.h>
#include <WiFi.h>
#include <ESP32Servo.h>
#include <BTS7960.h>

Servo servoDireccion;
const int PIN_SERVO_DIR = 13; // asigna el pin 13 al servo
const uint8_t PIN_EN = 27; //para R_EN y L_EN
const uint8_t PIN_RPWM = 25;
const uint8_t PIN_LPWM = 26;

// Estructura de datos ESP-NOW (Debe ser idéntica en el emisor)
struct MensajeDatos { int velocidad; int angulo;
};
MensajeDatos datosEntrantes;

// Función Callback al recibir datos
void AlRecibirDatos(const uint8_t *mac, const uint8_t *datos, int len) {
// Copiar los datos binarios entrantes a nuestra estructura
memcpy(&datosEntrantes, datos, sizeof(datosEntrantes));
int velocidad.leida= map(datosEntrantes.velocidad, 0 ,4095, -255, 255)
int angulo_destino= map(datosEntrantes.angulo, 0, 4095, 50, 130)
Serial . printf ( " Datos recibidos -> Velocidad : %d | Ángulo : %d \ n " ,
                  velocidad_leida , angulo_DESTINO );
int angulo_actual= serovoDireccion.read();
if (angulo_destino > angulo_actual) {
 for (int a = angulo_actual; a <= angulo_destino; a++) {
  servoDireccion.write(a);
  delay(10);
 }
}
else {
  for (int a = angulo_actual; a >= angulo_destino; a--) {
  servoDireccion.write(a);
  delay(10);
}
}
if velocidad_leida >1 {
  motorTraccion.turnRight(velocidad_leida);
}
if velocidad_leida <-1 {
  motorTraccion.turnLeft(velocidad_leida);
}
}

void setup() {
Serial.begin(115200);
motorTraccion.begin();
motorTraccion.enable();

// Inicializar Servo a 50Hz
servoDireccion.setPeriodHertz(50);
ESP32PWM::allocateTimer(0);
servoDireccion.attach(PIN_SERVO_DIR, 500, 2500);
servoDireccion.write(90)

// Inicializar ESP-NOW en modo Station
WiFi.mode(WIFI_STA);
esp_now_init();

// Registra la función que actuará al recibir señal
esp_now_register_recv_cb(AlRecibirDatos);
}

void loop() {

}
