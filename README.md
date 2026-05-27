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

## Feature List

This is a list of features I want to add.

- [x] Ability to add all day events
- [ ] Repeat rules
    - [ ] Store event date-time data as a `struct tm`
- [ ] Multi-day all day events
- [ ] Native ics parser
- [ ] Google Calendar Integrations (read and write)
- [ ] Ability for configuration (need to flesh this out)
    - [ ] Colors
    - [ ] Layout
- [ ] Add cursors to input fields

## Bug List

This is a list of known bugs.

1. If lines of an ICS file end in LF instead of CRLF it causes a seg fault
    - This is not really a bug since by the RFC, ICS file lines must end int CRLF
      but it would probably be good to add fault handling.
4. When creating events between 12 PM and 12:59 PM the time of the event gets pushed forward 12 hours.
