# AscensorControl

Simulador de una interfaz de ascensor de tres pisos para Windows, desarrollado en C con LVGL y SDL2. La ventana representa una pantalla de 1024 × 600 píxeles.

![Vista del simulador de AscensorControl](docs/simulador.png)

El simulador permite seleccionar pisos, mostrar el piso y la dirección, animar las puertas, escuchar un aviso al llegar y activar o desactivar la emergencia. Simula las llegadas por tiempo y admite solicitudes pendientes durante un viaje.

> **Alcance actual:** esta versión se ejecuta en la computadora. No se conecta al PLC, no lee sensores físicos y no genera firmware para el ESP32-P4. La pantalla física, el PLC y los controladores del dispositivo no son necesarios para probarla.

## Requisitos

- Windows 10 u 11 de 64 bits y conexión a internet para la instalación y la primera compilación.
- Una cuenta de GitHub con acceso al repositorio: actualmente es privado.
- [Git para Windows](https://git-scm.com/install/windows), instalado en la ruta predeterminada `C:\Program Files\Git`.
- [MSYS2](https://www.msys2.org/), instalado en la ruta predeterminada `C:\msys64`.
- GCC, CMake y Ninja instalados dentro de MSYS2 UCRT64, como se explica abajo.
- [Visual Studio Code](https://code.visualstudio.com/download), recomendado para abrir el proyecto y ejecutar la tarea incluida; también se puede usar PowerShell sin instalarlo.

No es necesario descargar SDL2 ni LVGL por separado: CMake descarga las versiones configuradas durante la primera ejecución. No se necesita SDL3 para este proyecto.

## Instalación desde cero

### 1. Instalar Git

1. Descarga el instalador desde [Git para Windows](https://git-scm.com/install/windows).
2. Ejecútalo y sigue el asistente. Conserva la ruta predeterminada `C:\Program Files\Git`.
3. Deja habilitado Git Credential Manager si el instalador lo ofrece: facilita iniciar sesión en GitHub al clonar el repositorio privado.

### 2. Instalar MSYS2 y las herramientas de C

1. Descarga e instala [MSYS2](https://www.msys2.org/) en `C:\msys64`.
2. Abre **MSYS2 UCRT64** desde el menú Inicio. Para pegar en esta terminal puedes usar **Shift + Insert** o el menú contextual.
3. Actualiza los paquetes:

   ```bash
   pacman -Syu
   ```

4. Si MSYS2 indica que debe cerrarse para completar la actualización, confirma, vuelve a abrir **MSYS2 UCRT64** y ejecuta `pacman -Syu` nuevamente hasta completar la actualización.
5. Instala el compilador y las herramientas:

   ```bash
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
   ```

6. Cuando pregunte si deseas continuar, escribe `Y` y pulsa Enter. Espera a que termine la instalación.

La guía oficial de [actualización de MSYS2](https://www.msys2.org/docs/updating/) explica el reinicio de la terminal cuando se actualizan componentes esenciales.

### 3. Descargar el proyecto

Abre **PowerShell** o una terminal de VS Code y ejecuta:

```powershell
git clone https://github.com/Jorge-A-Ortiz/AscensorControl.git
cd AscensorControl
```

Si Git solicita iniciar sesión, utiliza una cuenta de GitHub que tenga acceso al repositorio. También puedes descargar el ZIP desde GitHub y extraerlo, siempre que tengas acceso; Git sigue siendo necesario para descargar las dependencias durante la primera configuración.

### 4. Compilar y ejecutar

**Desde VS Code:**

1. Instala y abre [Visual Studio Code](https://code.visualstudio.com/download).
2. Selecciona **Archivo → Abrir carpeta** y abre la carpeta `AscensorControl` descargada.
3. Si aparece la pregunta de confianza, acepta la carpeta para habilitar sus tareas.
4. Pulsa **Ctrl + Shift + P**, busca **Tasks: Run Task** o **Tareas: Ejecutar tarea** y selecciónalo.
5. Elige **Iniciar simulador**.

**Desde PowerShell**, situado en la carpeta `AscensorControl`:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\iniciar.ps1
```

En la primera ejecución, el script configura el proyecto, descarga SDL2 y LVGL y compila sus dependencias. Puede tardar varios minutos. Al terminar se abrirá la ventana **LVGL Simulator**. Las ejecuciones posteriores reutilizan la compilación.

## Uso

- Pulsa **1**, **2** o **3** para solicitar un piso. Las solicitudes aceptadas quedan resaltadas.
- El indicador muestra el piso simulado y la dirección. Las puertas se animan al salir y llegar.
- **Emergencia** permanece activada hasta pulsarla otra vez; durante ese tiempo se detiene la simulación y se bloquean nuevas solicitudes.
- Cierra la ventana del simulador antes de recompilar o volver a ejecutar la tarea.

La espera previa al movimiento configurada en el simulador es de 7,5 segundos. Antes de conectar el sistema físico debe comprobarse que coincida con el temporizador del programa del PLC.

## Solución de problemas

- **`cmake`, `ninja` o el compilador no se encuentran:** confirma que MSYS2 esté en `C:\msys64` y que los tres paquetes UCRT64 del paso 2 terminaron de instalarse.
- **No se puede descargar o clonar:** comprueba la conexión a internet y que Git esté en `C:\Program Files\Git`. Para clonar este repositorio privado, verifica también que la cuenta autenticada tenga acceso.
- **`Permission denied` al compilar `simulador-emergencia-enclavada.exe`:** cierra cualquier ventana del simulador que siga abierta y vuelve a ejecutar la tarea.
- **La ventana abre sin sonido:** comprueba la salida y el volumen de audio de Windows. El archivo de sonido está en `assets/llegada-ascensor.wav`.
- **La configuración de `build` quedó dañada o pertenece a otra ubicación:** cierra el simulador, elimina únicamente la carpeta `build` del proyecto y vuelve a ejecutar el script para configurarla de nuevo.

## Estructura

- `src/ui.c`: interfaz y comportamiento simulado.
- `src/platform_audio.c`: reproducción de audio en Windows mediante SDL2.
- `src/esp32p4_gpio.h`: asignación de GPIO prevista para una futura versión ESP-IDF; no está conectada a la simulación.
- `assets/`: fondo y sonido utilizados.
- `docs/simulador.png`: captura del simulador.
- `CMakeLists.txt` y `lv_conf.h`: compilación y configuración de LVGL.
