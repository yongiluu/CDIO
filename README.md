# Proyecto CDIO - Sistema de Mando y Control de Vehículo Autónomo / RC

Este repositorio contiene la arquitectura completa de hardware y software para el sistema de control remoto y telemetría de un prototipo vehicular desarrollado bajo la metodología **CDIO** (*Conceive, Design, Implement, Operate*).

El sistema se basa en una arquitectura maestro-esclavo (transmisor-receptor) bidireccional punto a punto (P2P) utilizando dos microcontroladores **ESP32** comunicados mediante el protocolo de baja latencia **ESP-NOW**.

---

## 🚀 Arquitectura General del Sistema

El proyecto está modularizado en dos bloques principales:

```
                  ┌──────────────────────┐
                  │   TRANSMISOR (TX)    │
                  │ (Estación / Control) │
                  └──────────┬───────────┘
                             │  ESP-NOW (WiFi P2P 2.4 GHz)
                             │  Baja Latencia (< 5 ms)
                             ▼
                  ┌──────────────────────┐
                  │    RECEPTOR (RX)     │
                  │   (Vehículo / Auto)  │
                  └──────────┬───────────┘
            ┌────────────────┴────────────────┐
            ▼                                 ▼
   ┌─────────────────┐               ┌─────────────────┐
   │ Servo Dirección │               │  Puente H 43A   │
   │  (Ángulo Turn)  │               │   (BTS7960)     │
   └─────────────────┘               └────────┬────────┘
                                              ▼
                                     ┌─────────────────┐
                                     │  Motor Tracción │
                                     └─────────────────┘
```

1. **`transmisor_TX/` (Estación de Mando / Control Remoto):**
   - Lectura analógica de Joysticks (resolución de 12 bits: `0 - 4095`).
   - Algoritmo de filtrado con **Zona Muerta (Deadzone)** para compensar deriva física (*drift*).
   - Empaquetado de datos en estructura `MensajeDatos` y despacho periódico a alta frecuencia (20 Hz / cada 50 ms).

2. **`receptor_RX/` (Vehículo / Actuadores):**
   - Recepción en tiempo real de paquetes vía callback asíncrono de ESP-NOW.
   - Control de ángulo para servomotor de dirección (`ESP32Servo`, Pin 13).
   - Control de velocidad y sentido de tracción mediante Puente H de alta potencia **BTS7960** (Pines 25, 26 y 27) utilizando canales PWM por hardware (`ledc`).
   - Sistema *Failsafe* (apagado del puente H en reposo para evitar consumo y movimientos involuntarios).

---

## 🛠️ Especificaciones de Hardware y Pines

### 1. Receptor (RX - Vehículo)
* **Microcontrolador:** ESP32-WROOM-32 (esp32dev).
* **Controlador de Tracción:** BTS7960 (43A H-Bridge).
  * `PIN_RPWM` -> **GPIO 25** (Canal PWM 0)
  * `PIN_LPWM` -> **GPIO 26** (Canal PWM 1)
  * `PIN_EN` (R_EN y L_EN unificados) -> **GPIO 27**
* **Servomotor de Dirección:** SG90 / MG996R (50 Hz).
  * `PIN_SERVO_DIR` -> **GPIO 13**
* **Sistema de Alimentación:**
  * Batería LiPo 2S/3S conectada a la etapa de potencia del BTS7960 (`B+` y `B-`).
  * Regulador Step-Down DC-DC **LM2596** reduciendo voltaje a 5.0V para alimentar el ESP32 (pin `VIN`/`5V`), el servo y la lógica del BTS7960.
  * Tierras comunes (**GND**) compartidas en un único punto de referencia.

### 2. Transmisor (TX - Control Remoto)
* **Microcontrolador:** ESP32-WROOM-32 (esp32dev).
* **Mando Analógico:** 2 Joysticks analógicos de dos ejes.
  * Eje Y (Velocidad / Tracción) -> **GPIO 34**
  * Eje X (Dirección / Ángulo) -> **GPIO 35**
* **Filtrado de Señal:**
  * Rango muerto calibrado entre `1400` y `2000` cuentas ADC para asegurar que el centro físico repose en `Velocidad = 0` y `Ángulo = 90°`.

---

## 📦 Estructura de Datos (ESP-NOW)

Ambas placas intercambian la estructura binaria exacta:

```cpp
typedef struct MensajeDatos {
    int velocidad; // Rango: -255 a 255 (0 = detenido, >0 = adelante, <0 = reversa)
    int angulo;    // Rango: 50° a 130° (90° = centrado)
} MensajeDatos;
```

---

## ⚙️ Compilación e Instalación

Este proyecto está configurado para **PlatformIO** (extensión recomendada para VS Code).

### Pasos de Carga:
1. **Receptor:**
   * Abrir la carpeta `receptor_RX/` en PlatformIO.
   * Compilar y subir el firmware al ESP32 del vehículo.
   * Abrir el Serial Monitor a **115200 baudios** para verificar que imprima su dirección MAC.
2. **Transmisor:**
   * Abrir la carpeta `transmisor_TX/` en PlatformIO.
   * En caso de requerir un cambio de placa, confirmar la MAC de destino en `broadcastAddress`.
   * Compilar y subir el firmware al ESP32 del control remoto.
   * Observar el LED indicador (GPIO 2) que parpadea con cada confirmación de entrega (*ACK*).

---

## 📄 Archivos Adicionales en el Repositorio
* `esquema_electronica.svg`: Esquema de bloques e interconexión de todo el subsistema electrónico y de telecomunicaciones.
* `conexiones esp32.pdf`: Diagrama esquemático de cableado pin a pin de los componentes reales.
* `guardar_como_png.html`: Utilidad para exportar el diagrama de arquitectura en alta resolución.
