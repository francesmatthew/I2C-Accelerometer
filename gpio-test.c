#include "pi-gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define SWITCH 4
#define GREEN_LED 18
#define BLUE_LED 15
#define RED_LED 14

int toggle(bool* red_on) {
    setPinValue(BLUE_LED, HIGH);
    setPinValue(GREEN_LED, LOW);
    setPinValue(RED_LED, LOW);

    char sw_state = 0;
    if ( getPinValue(SWITCH, &sw_state) ) { perror("GPIO get pin value failed\n"); }
    while(sw_state == LOW) {
        usleep(10000);
        if ( getPinValue(SWITCH, &sw_state) ) { perror("GPIO get pin value failed\n"); }
    }

    *red_on = *red_on ^ 1;
    setPinValue(BLUE_LED, LOW);

    if (*red_on) { setPinValue(RED_LED, HIGH); }
    else { setPinValue(GREEN_LED, HIGH); }
    return 0;
}

int main() {
    // exportGPIO(SWITCH);
    // exportGPIO(GREEN_LED);
    // exportGPIO(BLUE_LED);
    // exportGPIO(RED_LED);
    if (exportGPIO(SWITCH) | exportGPIO(GREEN_LED) | exportGPIO(BLUE_LED) | exportGPIO(RED_LED)) {
        perror("GPIO export failed\n");
    }

    // setPinDirection(SWITCH, IN);
    // setPinDirection(GREEN_LED, OUT);
    // setPinDirection(BLUE_LED, OUT);
    // setPinDirection(RED_LED, OUT);
    usleep(500000);
    if (setPinDirection(SWITCH, IN) | setPinDirection(GREEN_LED, OUT) | setPinDirection(BLUE_LED, OUT) | setPinDirection(RED_LED, OUT)) {
        perror("GPIO direction setting failed\n");
    }

    if (setPinValue(GREEN_LED, LOW) | setPinValue(BLUE_LED, LOW) | setPinValue(RED_LED, HIGH)) {
        perror("GPIO set pin value failed\n");
    }
    bool red_on = 1;

    char stdin_buf[5] = {0};
    char sw_state = 0;

    pid_t cpid = fork();

    if (cpid == -1) {
        perror("fork error\n");
        return 1;
    }
    if (cpid == 0) {
        // child thread polls input switch
        while(1) {
            if (getPinValue(SWITCH, &sw_state)) { perror("GPIO get pin value failed\n"); }
            if (sw_state == LOW) {
                usleep(10000);
                if ( getPinValue(SWITCH, &sw_state) ) { perror("GPIO get pin value failed\n"); }
                if (sw_state == LOW) {
                    toggle(&red_on);
                }
            }
            usleep(10000);
        }
    }
    else {
        // parent thread reads stdin
        if (read(0, stdin_buf, 5) == 5) {
            if (strcmp(stdin_buf, "stop\0")) {
                if (kill(cpid, SIGTERM) == -1) {
                    perror("error killing child process\n");
                }
                wait(0);
                if (unexportGPIO(SWITCH) | unexportGPIO(GREEN_LED) | unexportGPIO(BLUE_LED) | unexportGPIO(RED_LED)) {
                    perror("GPIO unexport failed\n");
                }
                return 0;
            }
        }
    }


}