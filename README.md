# ESP32 Pantry-IO
WIP Pantry-IO software for ESP32. This is my attempt at tinkering with FreeRTOS and ESP32.

The pantry-IO is a project that's goal is to keep track of what food items you have
available in your kitchen cabinet.
The whole project also consists of an [API](https://github.com/salmmike/pantry-io-api) and an android app.

This repo contains the software that should run on an ESP32 that's placed in a kitchen cabinet.
[The development board used.](https://www.partco.fi/en/development-boards/esp/20179-esp32-wifible.html)

## Compiling:
Install the [ESP-IDF development environment](https://github.com/espressif/esp-idf)
compile with idf.py build

## Components:
http:
    Handles HTTPS requests

bluetooth:
    Handle bluetooth initialization and communication.

nvs\_init:
    Non-volatile storage operations

wifi:
    Establish WiFi connecion.

button-states:
    Handle button I/O events.

