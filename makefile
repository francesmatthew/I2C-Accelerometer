i2c-accel: i2c-accel.o pi-gpio.o
	gcc -o i2c-accel i2c-accel.o pi-gpio.o
i2c-accel.o: i2c-accel.c i2c-accel.h
	gcc -c i2c-accel.c
pi-gpio.o: pi-gpio.c pi-gpio.h
	gcc -c pi-gpio.c
gpio-test: pi-gpio.o gpio-test.c
	gcc -o gpio-test pi-gpio.o gpio-test.c
clean:
	rm -f *.o