#!/bin/bash

if [[ ! -d "$HOME/.calendar" ]]; then
    mkdir -p "$HOME"/.calendar
fi

curl "https://terokarvinen.com/2021/calendar-txt/calendar-txt-until-2033.txt" -o "$HOME/.calendar/calendar.txt"

curl "https://github.com/jsypal04/Calenter/releases/download/v0.1.0/calenter-linux-x86_64" -o "$HOME/.local/bin/calenter"

