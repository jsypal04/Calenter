# Calenter

Calenter is a TUI frontend for calendar.txt written in C.

## Building from Source

Run the following:
```bash
git clone https://github.com/jsypal04/Calenter.git
make
```
The binary is `build/calenter`.

## Config File

You may create a config file at `~/.config/calenter/config`. It uses the
following basic syntax:
```
key=value
```
At the time of writing the only configurable option is to add your private Google Calendar
ICS url using the following line:
```
remote_url=<your gcal url>
```

## Bugs

This is a list of known bugs that I would like to get around to fixing at some point.

