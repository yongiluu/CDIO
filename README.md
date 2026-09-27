# Proyecto CDIO - Carrito y Control Remoto

Este repositorio contiene el codigo base para la comunicacion entre dos placas ESP32-S3 utilizando el protocolo ESP-NOW. El proyecto esta dividido en dos partes principales:

- carrito/: Contiene el codigo del receptor. Este va en el ESP32 que se encarga de controlar los motores del carrito.
- control_remoto/: Contiene el codigo del emisor. Este va en el ESP32 que se usara como control.

## Instrucciones de uso

Para que la comunicacion funcione, es necesario configurar la direccion MAC correcta. Sigue estos pasos:

1. Abre la carpeta del carrito en PlatformIO (VS Code), compila y sube el codigo a la primera placa.
2. Abre el Monitor Serie (baud rate: 115200). La placa imprimira su direccion MAC en la consola.
3. Copia esa direccion MAC.
4. Abre la carpeta del control_remoto en PlatformIO.
5. Ve al archivo src/main.cpp y reemplaza el valor de la variable broadcastAddress con la MAC que copiaste.
6. Sube este codigo a la segunda placa.

## Sobre el codigo actual

Ambas placas comparten la misma estructura de datos (struct_message) para enviarse informacion, la cual incluye los ejes X e Y para el joystick, y dos botones de accion. 

Actualmente, el codigo del control remoto esta configurado para mandar datos simulados de forma aleatoria. Esto es para poder probar que la conexion inalambrica ESP-NOW sea estable antes de conectar toda la electronica. Una vez validada la conexion, solo hay que cambiar esos datos por las lecturas reales de los pines (analogRead / digitalRead).
