# I2C-Accelerometer
This respository contains the source code to retrieve LIS3DH accelerometer data from the I2C bus on a Raspberry Pi.

Dependencies (should be included with Raspian; may need to be enabled through raspi-config):
-i2c dev-interface linux kernel module (linux/i2c-dev.h)
-sysfs interface for RPi GPIO