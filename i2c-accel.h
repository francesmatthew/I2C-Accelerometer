#ifndef I2C_ACCEL_H
#define I2C_ACCEL_H

#include <linux/i2c-dev.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <errno.h>
#include "pi-gpio.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define RECORD_INTERVAL_USEC 10000
#define MAX_GPIO_ATTEMPTS 5

uint32_t ADAPTER_NO = 1;
uint8_t DEV_ADDR = 0x18;
// sensitivity determined based on range and mode
float SENSITIVITY = 0.004;
uint8_t XYZ_DATA_REG = 0x28;
uint32_t XYZ_DATA_LEN = 6;
typedef enum {RECORDING, PROCESSING, READY} led_state_t;

#define SWITCH 4
#define GREEN_LED 18
#define BLUE_LED 15
#define RED_LED 14
#define OUTPUT_FILENAME_TEMPLATE "data/%4d-%02d-%02d:%02d-%02d-%02d.csv"

bool devWrite(int fd, uint8_t reg, uint8_t val);
bool devRead(int fd, char* buffer, uint32_t reg, uint32_t size);
int openI2CBus(uint32_t adapterNo, uint8_t addr);
bool writeAccelConfig(int i2c_fd);
int recordDataLoop(int i2c_fd);
int changeLEDState(led_state_t state);

#endif