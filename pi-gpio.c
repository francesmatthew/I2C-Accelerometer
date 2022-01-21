#include "pi-gpio.h"

int exportGPIO(uint32_t pin) {
    // Export the desired pin by writing to /sys/class/gpio/export
    int fd = open("/sys/class/gpio/export", O_WRONLY);
    if (fd == -1) {
        return -1;
    }
    // convert pin number to string
    char pinStr[3] = {0};
    int pinStrLen = snprintf(pinStr, 3, "%d", pin);
    // write pin to /sys/class/gpio/export
    if (write(fd, pinStr, pinStrLen) != pinStrLen) {
        return -1;
    }
    close(fd);
    return 0;
}

int setPinDirection(uint32_t pin, gpio_dir dir) {
    // set pin direction by writing "in" or "out" to /sys/class/gpio/gpio##/direction
    // assemble file path using specified pin number
    char filename[33] = {0};
    snprintf(filename, 33, "/sys/class/gpio/gpio%d/direction", pin);
    int fd = open(filename, O_WRONLY);
    if (fd == -1) {
        return -2;
    }
    // write to /sys/class/gpio/gpio##/direction
    if (write(fd, dir, strlen(dir)) != strlen(dir)) {
        return -1;
    }
    close(fd);
    return 0;
}

int setPinValue(uint32_t pin, gpio_val val) {
    // **Pin must be exported and defined as output!**
    // assemble file path using specified pin number
    char filename[29] = {0};
    snprintf(filename, 29, "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(filename, O_WRONLY);
    if (fd == -1) {
        return -1;
    }
    // write to /sys/class/gpio/gpio##/value
    if (write(fd, &val, 1) != 1) {
        return -1;
    }
    close(fd);
    return 0;
}

int getPinValue(uint32_t pin, char* buf) {
    // **Pin must be exported and defined as input!**
    // assemble file path using specified pin number
    char filename[29] = {0};
    snprintf(filename, 29, "/sys/class/gpio/gpio%d/value", pin);
    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
        return -1;
    }
    char buffer[3] = {0};
    // read from /sys/class/gpio/gpio##/value
    if (read(fd, buffer, 3) < 1) {
        return -1;
    }
    *buf = buffer[0];
    close(fd);
    return 0;
}

int unexportGPIO(uint32_t pin) {
    // Unxport the desired pin by writing to /sys/class/gpio/unexport
    int fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if (fd == -1) {
        return -1;
    }
    // convert pin number to string
    char pinStr[3] = {0};
    int pinStrLen = snprintf(pinStr, 3, "%d", pin);
    // write pin to /sys/class/gpio/unexport
    if (write(fd, pinStr, pinStrLen) != pinStrLen) {
        return -1;
    }
    close(fd);
    return 0;
}