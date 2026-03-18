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

## Installation

### From source

Run the following commands:
```bash
git clone https://github.com/jsypal04/Calenter.git
make install
```

Download a calendar.txt template and place it in `~/.calendar/calendar.txt`

### Pre-built

Run the following command:
```bash
curl "https://raw.githubusercontent.com/jsypal04/Calenter/refs/heads/main/install.sh" | bash
```
