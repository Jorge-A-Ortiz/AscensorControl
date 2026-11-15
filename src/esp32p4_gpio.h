#ifndef ESP32P4_GPIO_H
#define ESP32P4_GPIO_H

/* Asignacion prevista para ESP-IDF. No se usa en el simulador de PC. */
#include "driver/gpio.h"

/* Salidas hacia reles o actuadores de las llamadas de piso. */
#define GPIO_OUT_1 GPIO_NUM_50
#define GPIO_OUT_2 GPIO_NUM_51
#define GPIO_OUT_3 GPIO_NUM_52

/* Salida para la bocina. */
#define GPIO_OUT_HORN GPIO_NUM_5

/* Entradas para indicadores/senales de los pisos. */
#define GPIO_IN_1 GPIO_NUM_25
#define GPIO_IN_2 GPIO_NUM_27
#define GPIO_IN_3 GPIO_NUM_28

#endif
