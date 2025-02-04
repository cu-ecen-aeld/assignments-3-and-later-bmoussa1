#!/bin/bash

# Resolve the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
echo $SCRIPT_DIR

# Paths relative to the script directory
AESDSOCKET_BIN="$SCRIPT_DIR/aesdsocket"
PID_FILE="$SCRIPT_DIR/aesdsocket.pid"
LOG_FILE="$SCRIPT_DIR/aesdsocket.log"

start() {
    echo "Starting aesdsocket..."
    $AESDSOCKET_BIN -d 2>>$LOG_FILE &
    sleep 1  # Allow daemon to initialize
    pgrep -f "$AESDSOCKET_BIN -d" > $PID_FILE
    if [ -s $PID_FILE ]; then
        echo "aesdsocket started with PID $(cat $PID_FILE)."
    else
        echo "Failed to determine aesdsocket PID. Check logs."
    fi
}



stop() {
    echo "Stopping aesdsocket..."
    start-stop-daemon --stop --quiet --pidfile $PID_FILE
    if [ $? -eq 0 ]; then
        echo "aesdsocket stopped."
    else
        echo "Failed to stop aesdsocket. Check if it is running."
    fi
}

status() {
    if [ -f $PID_FILE ]; then
        PID=$(cat $PID_FILE)
        if ps -p $PID > /dev/null 2>&1; then
            echo "aesdsocket is running with PID $PID."
            return 0
        else
            echo "PID file exists, but no process is running."
            return 1
        fi
    else
        # Fallback: Check for the process by name
        if pgrep -x "aesdsocket" > /dev/null 2>&1; then
            echo "aesdsocket is running (detected by name)."
            return 0
        else
            echo "aesdsocket is not running."
            return 1
        fi
    fi
}


case "$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart)
        stop
        start
        ;;
    status)
        status
        ;;
    *)
        echo "Usage: $0 {start|stop|restart|status}"
        exit 1
        ;;
esac

