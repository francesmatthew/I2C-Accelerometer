#ifndef PI_GPIO_H
#define PI_GPIO_H

#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>

typedef char* gpio_dir;
#define OUT "out"
#define IN "in"
typedef char gpio_val;
#define HIGH '1'
#define LOW '0'

int exportGPIO(uint32_t pin);
int setPinDirection(uint32_t pin, gpio_dir dir);
int setPinValue(uint32_t pin, gpio_val val);
int getPinValue(uint32_t pin, char* buf);
int unexportGPIO(uint32_t pin);

#endif