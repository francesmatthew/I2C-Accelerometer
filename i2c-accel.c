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
      if (getSwitchState()) { usleep(RECORD_INTERVAL_USEC); }
      else { break; }
    }
    close(output_fd);
    return 0;
}

bool getSwitchState() {
  return 1;
}

int main() {

  int i2c_fd = openI2CBus(ADAPTER_NO, DEV_ADDR);
  if (i2c_fd < 0) {
    printf("Failed to acquire bus access\n");
    return 1;
  }
  // setup control registers
  if (writeAccelConfig(i2c_fd)) {
    printf("Failed to configure device\n");
    close(i2c_fd);
    return 1;
  }

  // setup GPIO


  // poll input
  while (1) {
    // poll if switch is turned on
    if (getSwitchState()) {
      // de-bounce the switch, wait 10ms
      usleep(10000);
      if (getSwitchState()) {
        // enter a loop of recording data
        if (recordDataLoop(i2c_fd) < 0) {
          close(i2c_fd);
          return 1;
        }
      }
    }
  }

  close(i2c_fd);
  return 0;
}
