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

#define RECORD_INTERVAL_USEC 10000

uint32_t ADAPTER_NO = 1;
uint8_t DEV_ADDR = 0x18;
// sensitivity determined based on range and mode
float SENSITIVITY = 0.004;
uint8_t XYZ_DATA_REG = 0x28;
uint32_t XYZ_DATA_LEN = 6;

bool devWrite(int fd, uint8_t reg, uint8_t val);
bool devRead(int fd, char* buffer, uint32_t reg, uint32_t size);
int openI2CBus(uint32_t adapterNo, uint8_t addr);
bool writeAccelConfig(int i2c_fd);
int recordDataLoop(int i2c_fd);
bool getSwitchState();

#endif