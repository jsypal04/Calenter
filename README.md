# Calenter

Calenter is a TUI frontend for calendar.txt inspired by [Impala](https://github.com/pythops/impala)
and [Bluetui](https://github.com/pythops/bluetui).

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

Run the following command (don't do this):
```bash
curl -L "https://github.com/jsypal04/Calenter/releases/download/v0.1.0/install.sh" | bash
```

## Feature List

This is a list of features I want to add.

1. Ability to add all day events
2. Repeat rules
3. Multi-day all day events
4. Native ics parser
5. Google Calendar Integrations (read and write)
6. Ability for configuration (need to flesh this out)
    1. Colors
    2. Layout

## Bug List

This is a list of known bugs.

1. Trim whitespace off of event descriptions when adding/editing an event.
