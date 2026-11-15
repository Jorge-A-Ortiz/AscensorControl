# AscensorControl

Simulador de una interfaz de ascensor de tres pisos, desarrollado en C con LVGL y SDL2 para Windows. La pantalla simulada tiene una resolución de 1024 × 600.

El simulador reproduce la selección y cola de pisos, la animación decorativa de la puerta, el indicador de dirección, la emergencia y un sonido al llegar. La posición y las llegadas se simulan por tiempo: no hay conexión con el PLC ni lectura de sensores físicos.

## Ejecutar en Windows

Se necesitan Git para Windows y MSYS2 UCRT64 con los paquetes `mingw-w64-ucrt-x86_64-gcc`, `mingw-w64-ucrt-x86_64-cmake` y `mingw-w64-ucrt-x86_64-ninja`. La primera ejecución necesita acceso a internet para descargar SDL2 y LVGL.

Abre esta carpeta en VS Code, pulsa `Ctrl+Shift+P`, selecciona **Tasks: Run Task** y después **Iniciar simulador**. También puedes ejecutar desde PowerShell:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\iniciar.ps1
```

Cierra el simulador antes de recompilarlo. La primera compilación puede tardar varios minutos.

## PLC y pantalla

La lógica del simulador se basa en el Ladder compartido, pero no reemplaza su validación. La espera del simulador es de 7,5 segundos; debe coincidir con el temporizador configurado en el PLC. Los sensores X003–X005 y la comunicación física aún no están implementados.

La configuración de referencia utiliza ESP32-P4, pantalla MIPI-DSI, controlador EK79007 y táctil GT911. Este repositorio todavía no genera firmware ESP-IDF ni acciona hardware. Los pines previstos están documentados en `src/esp32p4_gpio.h`.

El botón de emergencia permanece activo hasta una segunda pulsación en el simulador. La implementación física y su correspondencia con X006 quedan pendientes.
