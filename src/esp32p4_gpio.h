#ifndef ESP32P4_GPIO_H
#define ESP32P4_GPIO_H

#include "driver/gpio.h"

/* Salidas de la pantalla hacia las entradas del PLC. */
#define GPIO_OUT_1         GPIO_NUM_52 /* X000: solicitud de piso 1 */
#define GPIO_OUT_2         GPIO_NUM_51 /* X001: solicitud de piso 2 */
#define GPIO_OUT_3         GPIO_NUM_50 /* X002: solicitud de piso 3 */
#define GPIO_OUT_EMERGENCY GPIO_NUM_49 /* X006: emergencia */

/* Entradas de la pantalla para los indicadores de piso. */
#define GPIO_IN_1 GPIO_NUM_2
#define GPIO_IN_2 GPIO_NUM_3
#define GPIO_IN_3 GPIO_NUM_4

/* Asignacion anterior de bocina, pendiente de confirmar en el cableado. */
#define GPIO_OUT_HORN GPIO_NUM_5

#endif
