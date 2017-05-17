#!/bin/sh

PROG=/usr/bin/thermo_lm75
CFG=/etc/default/thermo_lm75.cfg

test -x ${PROG} || { echo "${PROG} not found"; exit 0; }

if test -f ${CFG}; then
    . ${CFG}
fi

if ! `lsmod | grep i2c_dev -q`; then
    echo "Trying to load i2c_dev module..."
    modprobe i2c_dev || exit 1
    echo Successfully loaded i2c_dev modules
fi

case "$1" in
    start)
        if ! test -d `dirname ${FIFO_SRV}`; then
            mkdir -p `dirname ${FIFO_SRV}`
        fi
        start-stop-daemon --start --exec ${PROG} -- ${THERMO_LM75_OPTS}
        ;;
    stop)
        start-stop-daemon --stop --exec ${PROG}
        if test -d `dirname ${FIFO_SRV}`; then
            rmdir `dirname ${FIFO_SRV}`
        fi
        ;;
    *)
        echo "Usage: $0 start|stop"
        ;;
esac

exit 0
