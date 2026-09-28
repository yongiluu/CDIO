# Proyecto CDIO - Receptor (RX) y Transmisor (TX)

Este repositorio contiene el codigo base para la comunicacion entre dos placas ESP32 clasicas utilizando el protocolo ESP-NOW. El proyecto esta dividido en dos partes principales:

- receptor_RX/: Contiene el codigo del receptor. Este va en el ESP32 que se encarga de controlar los motores (o recibir los comandos).
- transmisor_TX/: Contiene el codigo del emisor. Este va en el ESP32 que se usara como control.

## Instrucciones de uso

Para que la comunicacion funcione, es necesario configurar la direccion MAC correcta. Sigue estos pasos:

1. Abre la carpeta del receptor_RX en PlatformIO (VS Code), compila y sube el codigo a la primera placa.
2. Abre el Monitor Serie (baud rate: 115200). La placa imprimira su direccion MAC en la consola.
3. Copia esa direccion MAC.
4. Abre la carpeta del transmisor_TX en PlatformIO.
5. Ve al archivo src/main.cpp y reemplaza el valor de la variable broadcastAddress con la MAC que copiaste.
6. Sube este codigo a la segunda placa.

## Sobre el codigo actual

Ambas placas comparten la misma estructura de datos (struct_message) para enviarse informacion, la cual incluye los ejes X e Y para el joystick, y dos botones de accion. 

Actualmente, para fines de prueba, el transmisor envia un simple contador y enciende el LED integrado de la placa cada vez que la comunicacion es exitosa, mientras que el receptor parpadea su LED al recibir el mensaje.
