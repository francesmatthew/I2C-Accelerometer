#!/bin/sh
PROG_DIR="/home/pi/I2C-Accelerometer"

echo stop > ${PROG_DIR}/in.fifo

WAITSECS="0"
MAXWAITSECS="30"
pgrepTest()
{
 pgrep -F ${PROG_DIR}/prog.pid > /dev/null 2>&1
 PGREPEXIT=$?
}
# wait until the process ends
pgrepTest
until [ $PGREPEXIT -ne "0" ]
do
 if [ $WAITSECS -ge $MAXWAITSECS ]
 then
 # forcible kill the process
 kill -9 =`cat ${PROG_DIR}/prog.pid` > /dev/null 2>&1
 fi
 sleep 1
 pgrepTest
 WAITSECS=`expr $WAITSECS + 1`
done

# remove the process keeping the fifo open
kill -9 `cat ${PROG_DIR}/sleep.pid` >  /dev/null 2>&1
# clean up
rm -f ${PROG_DIR}/in.fifo
rm -f prog.pid
rm -f sleep.pid
exit 0
