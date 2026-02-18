#!/bin/bash

CALENDAR_DIR=$HOME/.calendar

start=$(date +%s)
timeout=10

mkdir $CALENDAR_DIR/downloads

result=1
duration=$(($(date +%s) - $start))
while [ $duration -le $timeout ]; do
    curl -o $CALENDAR_DIR/downloads/gcal.ics "https://calendar.google.com/calendar/ical/sypalj%40cua.edu/private-d2b9ed591858075349ddec67354adcaa/basic.ics"
    if [ $? -eq 0 ]; then
        result=0
        break
    fi

    sleep 5
    duration=$(($(date +%s) - $start))
done

if [[ $result != 0 ]]; then
    return=$?
    rm -rf $CALENDAR_DIR/downloads
    notify-send --urgency=critical "calendar.txt" "Failed to download Google Calendar ICS file"

    exit $return
fi

python3 $CALENDAR_DIR/scripts/write_events.py $CALENDAR_DIR/downloads/gcal.ics > /dev/null

if [[ $? != 0 ]]; then
    return=$?
    rm -rf $CALENDAR_DIR/downloads
    notify-send --urgency=critical "calendar.txt" "Failed to write Google Calendar event to calendar.txt"

    exit $return
fi

rm -rf $CALENDAR_DIR/downloads

notify-send --urgency=normal "calendar.txt" "Google Calendar sync successful"
