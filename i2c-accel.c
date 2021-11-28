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

int writeToDevice(int fd, int reg, int val) {
  // fd: file descriptor of i2c device
  // reg: address of register to write to
  // val: value to write into that address
  // returns 0 on success, 1 on failure
   char buf[2];

   buf[0]=reg; buf[1]=val;

   return (write(fd, buf, 2) != 2);
}

int readDevice(int fd, char* buffer, int reg, int size) {
  // fd: file descriptor of i2c device
  // buffer: byte array that relevant data will be read to
  //   **buffer size **must** be larger than 'size'
  // reg: address of register to start reading data at
  // size: number of bytes to read
  // returns 0 on success, 1 on failure

  // or with 0x80 to enable address incrementing after each read
  buffer[0] = (reg | 0x80);

  // write register address onto bus
  if (write(fd, buffer, 1) != 1) {
    return 1;
  }

  return (read(fd, buffer, size) != size);
}


int main() {
  int file_desc;
  int adapter_nr = 1;
  char filename[20];

  // open i2c bus file
  snprintf(filename, 19, "/dev/i2c-%d", adapter_nr);
  file_desc = open(filename, O_RDWR);
  if (file_desc < 0) {
    printf("Failed to open i2c bus %d\n", adapter_nr);
    return 1;
  }

  // specifially address a certain i2c device
  int addr = 0x18;
  if (ioctl(file_desc, I2C_SLAVE, addr) < 0) {
    printf("Failed to acquire bus access\n");
    return 1;
  }

  // setup control registers
  bool config_fail = 0;
  config_fail = config_fail | writeToDevice(file_desc, 0x20, 0b01000111); // 50 Hz sample rate
  config_fail = config_fail | writeToDevice(file_desc, 0x21, 0b00000000);
  config_fail = config_fail | writeToDevice(file_desc, 0x22, 0b00000000);
  config_fail = config_fail | writeToDevice(file_desc, 0x23, 0b00101000); // high-resolution, 8g sensitivity
  config_fail = config_fail | writeToDevice(file_desc, 0x24, 0b00000000);
  config_fail = config_fail | writeToDevice(file_desc, 0x25, 0b00000000);

  if (config_fail) {
    printf("Failed to configure device\n");
    return 1;
  }

  // open a file to output all data to
  time_t start_time = time(NULL);
  struct tm time_struct = *localtime(&start_time);
  char output_filename[30] = {0};
  snprintf(output_filename, 30, "data/%4d-%02d-%02d:%02d-%02d-%02d.csv", \
    time_struct.tm_year+1900, time_struct.tm_mon+1, time_struct.tm_mday, \
    time_struct.tm_hour, time_struct.tm_min, time_struct.tm_sec);

  unsigned int output_file_desc = open(output_filename, O_CREAT | O_TRUNC | O_WRONLY , 00666);
  if (output_file_desc < 0) {
    printf("Failed to open output file, errno: %d\n", errno);
    return 1;
  }
  // file header
  write(output_file_desc, "time,x_acceleration,y_acceleration,z_acceleration\n", 50);

  unsigned char buffer[10]; // used to send/recieve data across i2c bus
  float sensitivity = 0.004; // determined based on range and mode
  int16_t x_accel_raw;
  int16_t y_accel_raw;
  int16_t z_accel_raw;
  float x_accel;
  float y_accel;
  float z_accel;
  char output_line[100];
  // initialize time structures
  struct timeval tval_before, tval_after, tval_result;
  gettimeofday(&tval_before, NULL);
  while(1) {

    if (readDevice(file_desc, buffer, 0x28, 6)) {
      printf("Failed to read from i2c device at address %x\r\n", addr);
      return 1;
    }

      // buffer contains the read data

      x_accel_raw = (buffer[1]<<8) + buffer[0];
      x_accel_raw = x_accel_raw>>4;
      x_accel = (x_accel_raw * sensitivity);

      y_accel_raw = (buffer[3]<<8) + buffer[2];
      y_accel_raw = y_accel_raw>>4;
      y_accel = (y_accel_raw * sensitivity);

      z_accel_raw = (buffer[5]<<8) + buffer[4];
      z_accel_raw = z_accel_raw>>4;
      z_accel = (z_accel_raw * sensitivity);


      //printf("x: %.2f\ty: %.2f\tz: %.2f\n", x_accel, y_accel, z_accel);
      memset(output_line, (uint8_t) 0, sizeof output_line); // clear output line
      gettimeofday(&tval_after, NULL);
      timersub(&tval_after, &tval_before, &tval_result);

      snprintf(output_line, 99, "%ld.%06ld,%.2f,%.2f,%.2f\r\n", \
        (long int)tval_result.tv_sec, (long int)tval_result.tv_usec, \
        x_accel, y_accel, z_accel);
      write(output_file_desc, output_line, strlen(output_line));

      if ((time(NULL)-start_time) > 10) {
        break;
      }
      
      usleep(10000);
    }

  close(output_file_desc);
  close(file_desc);
  return 0;
}
