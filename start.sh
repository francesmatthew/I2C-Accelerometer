PROG_DIR="/home/pi/I2C-Accelerometer"

# create pipe to eventually stop the program
mkfifo -m 0660 ${PROG_DIR}/in.fifo
sleep infinity > ${PROG_DIR}/in.fifo &
echo $! > ${PROG_DIR}/sleep.pid

cd ${PROG_DIR}
`${PROG_DIR}/i2c-accel < ${PROG_DIR}/in.fifo` &
echo $! > ${PROG_DIR}/prog.pid
exit 0
