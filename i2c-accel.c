#include "i2c-accel.h"

bool devWrite(int fd, uint8_t reg, uint8_t val) {
  // fd: file descriptor of i2c device
  // reg: address of register to write to
  // val: value to write into that address
  // returns 0 on success, 1 on failure
   char buf[2];

   buf[0]=reg; buf[1]=val;

   return (write(fd, buf, 2) != 2);
}

bool devRead(int fd, char* buffer, uint32_t reg, uint32_t size) {
  // fd: file descriptor of i2c device
  // buffer: byte array that relevant data will be read to
  //   **buffer size **must** be larger than 'size'
  // reg: address of register to start reading data at
  // size: number of bytes to read
  // returns 0 on success, 1 on failure

  // set the most significant bit to enable address incrementing after each read
  buffer[0] = (reg | 0x80);

  // write register address onto bus
  if (write(fd, buffer, 1) != 1) {
    return 1;
  }

  return (read(fd, buffer, size) != size);
}

int openI2CBus(uint32_t adapterNo, uint8_t addr) {
  int i2c_fd;
  char filename[20];
  // open i2c bus file
  snprintf(filename, 19, "/dev/i2c-%d", adapterNo);
  i2c_fd =  open(filename, O_RDWR);
  if (i2c_fd < 0) {
    return -1;
  }
  // specifically address a cetain i2c device
  if (ioctl(i2c_fd, I2C_SLAVE, addr) < 0) {
    return -1;
  }
  else {
    return i2c_fd;
  }

}

bool writeAccelConfig(int i2c_fd) {
  // setup control registers
  // takes the file descriptor returned from openI2CBus()
  int config_fail = 0;
  config_fail = config_fail | devWrite(i2c_fd, 0x20, 0b01000111); // 50 Hz sample rate
  config_fail = config_fail | devWrite(i2c_fd, 0x21, 0b00000000);
  config_fail = config_fail | devWrite(i2c_fd, 0x22, 0b00000000);
  config_fail = config_fail | devWrite(i2c_fd, 0x23, 0b00101000); // high-resolution, 8g sensitivity
  config_fail = config_fail | devWrite(i2c_fd, 0x24, 0b00000000);
  config_fail = config_fail | devWrite(i2c_fd, 0x25, 0b00000000);
  return config_fail;
}

int recordDataLoop(int i2c_fd) {
  // wait for the user to let off the button
  char sw_state = 0;
  while (1) {
    getPinValue(SWITCH, &sw_state);
    if (sw_state == HIGH) {
      // wait 10ms to debounce switch
      usleep(10000);
      getPinValue(SWITCH, &sw_state);
      if (sw_state == HIGH) {
        break;
      }
    }
    usleep(10000);
  }
  // open a file to output data
  // a time struct is required to track millisecond values
  time_t start_time = time(NULL);
  struct tm time_struct = *localtime(&start_time);
  // create a name for the output file based on the current time
  char output_filename[30] = {0};
  snprintf(output_filename, 30, "data/%4d-%02d-%02d:%02d-%02d-%02d.csv", \
    time_struct.tm_year+1900, time_struct.tm_mon+1, time_struct.tm_mday, \
    time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);

  unsigned int output_fd = open(output_filename, O_CREAT | O_TRUNC | O_WRONLY , 00666);
  if (output_fd < 0) {
    printf("Failed to open output file, errno: %d\n", errno);
    return -1;
  }
  // file header
  write(output_fd, "time,x_acceleration,y_acceleration,z_acceleration\r\n", 50);

  unsigned char buffer[10]; // used to send/recieve data across i2c bus
  int16_t x_accel_raw, y_accel_raw, z_accel_raw;
  float x_accel, y_accel, z_accel;
  char output_line[100];
  // initialize time structures
  struct timeval tval_before, tval_after, tval_result;
  gettimeofday(&tval_before, NULL);
  if (changeLEDState(RECORDING)) {
        perror("GPIO set pin value failed\n");
    }
  while(1) {

    if (devRead(i2c_fd, buffer, 0x28, 6)) {
      printf("Failed to read from device \n");
      close(output_fd);
      return -1;
    }

      // buffer contains the read data

      x_accel_raw = (buffer[1]<<8) + buffer[0];
      x_accel_raw = x_accel_raw>>4;
      x_accel = (x_accel_raw * SENSITIVITY);

      y_accel_raw = (buffer[3]<<8) + buffer[2];
      y_accel_raw = y_accel_raw>>4;
      y_accel = (y_accel_raw * SENSITIVITY);

      z_accel_raw = (buffer[5]<<8) + buffer[4];
      z_accel_raw = z_accel_raw>>4;
      z_accel = (z_accel_raw * SENSITIVITY);


      //printf("x: %.2f\ty: %.2f\tz: %.2f\n", x_accel, y_accel, z_accel);
      memset(output_line, (uint8_t) 0, sizeof output_line); // clear output line
      gettimeofday(&tval_after, NULL);
      timersub(&tval_after, &tval_before, &tval_result);

      snprintf(output_line, 99, "%ld.%06ld,%.2f,%.2f,%.2f\r\n", \
        (long int)tval_result.tv_sec, (long int)tval_result.tv_usec, \
        x_accel, y_accel, z_accel);
      write(output_fd, output_line, strlen(output_line));

      // poll input to check if continuing
      getPinValue(SWITCH, &sw_state);
      if (sw_state == LOW) {
        // wait 10ms to debounce switch
        usleep(10000);
        getPinValue(SWITCH, &sw_state);
        if (sw_state == LOW) {
          break;
        }
      }
      usleep(RECORD_INTERVAL_USEC);
    }
    if (changeLEDState(PROCESSING)) {
          perror("GPIO set pin value failed\n");
    }
    close(output_fd);
    // wait for the user to let off the button
    while (1) {
      getPinValue(SWITCH, &sw_state);
      if (sw_state == HIGH) {
        // wait 10ms to debounce switch
        usleep(10000);
        getPinValue(SWITCH, &sw_state);
        if (sw_state == HIGH) {
          break;
        }
      }
      usleep(10000);
    }
    return 0;
}

int changeLEDState(led_state_t state) {
  if (state == RECORDING) {
    return (setPinValue(GREEN_LED, LOW) | setPinValue(BLUE_LED, LOW) | setPinValue(RED_LED, HIGH));
  }
  else if (state == READY) {
    return (setPinValue(GREEN_LED, HIGH) | setPinValue(BLUE_LED, LOW) | setPinValue(RED_LED, LOW));
  }
  else {
    return (setPinValue(GREEN_LED, LOW) | setPinValue(BLUE_LED, HIGH) | setPinValue(RED_LED, LOW));
  }
}

int main() {

  int i2c_fd = openI2CBus(ADAPTER_NO, DEV_ADDR);
  if (i2c_fd < 0) {
    printf("Failed to acquire bus access\n");
    return -1;
  }
  // setup control registers
  if (writeAccelConfig(i2c_fd)) {
    printf("Failed to configure device\n");
    close(i2c_fd);
    return -1;
  }

  // setup GPIO
  // export pins
  if (exportGPIO(SWITCH) |
      exportGPIO(GREEN_LED) |
      exportGPIO(BLUE_LED) |
      exportGPIO(RED_LED)) {
          perror("GPIO export failed\n");
      }
  // set pin direction
  usleep(500000);
  if (setPinDirection(SWITCH, IN) |
      setPinDirection(GREEN_LED, OUT) |
      setPinDirection(BLUE_LED, OUT) |
      setPinDirection(RED_LED, OUT)) {
      perror("GPIO direction setting failed\n");
  }
  // inital pin settings
  if (changeLEDState(PROCESSING)) {
        perror("GPIO set pin value failed\n");
    }
  char sw_state = 0;
  char stdin_buf[5] = {0};
  pid_t c_pid = fork();

  if (c_pid == -1) {
      perror("failed to fork \n");
      return -1;
  }
  if (c_pid == 0) {
    // child thread
    // poll input
    if (changeLEDState(READY)) {
      perror("GPIO set pin value failed\n");
    }
    while (1) {
      // poll if switch is turned on
      getPinValue(SWITCH, &sw_state);
      if (sw_state == LOW) {
        // de-bounce the switch, wait 10ms
        usleep(10000);
        getPinValue(SWITCH, &sw_state);
        if (sw_state == LOW) {
          // register a button press
          if (changeLEDState(PROCESSING)) {
                perror("GPIO set pin value failed\n");
          }
          if (recordDataLoop(i2c_fd) < 0) {
            perror("error in recording data\n");
          }
          if (changeLEDState(READY)) {
            perror("GPIO set pin value failed\n");
          }
        }
      }
    }
  }
  else {
    // parent thread reads stdin
    if (read(0, stdin_buf, 5) == 5) {
      if (strcmp(stdin_buf, "stop\0")) {
          if (kill(c_pid, SIGTERM) == -1) {
            perror("error killing child process\n");
          }
          wait(0);
          if (unexportGPIO(SWITCH) |
              unexportGPIO(GREEN_LED) |
              unexportGPIO(BLUE_LED) |
              unexportGPIO(RED_LED)) {
            perror("GPIO unexport failed\n");
          }
          return 0;
        }
    }
  }

  close(i2c_fd);
  return 0;
}
