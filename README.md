# Calenter

Calenter is a TUI frontend for calendar.txt inspired by [Impala](https://github.com/pythops/impala)
and [Bluetui](https://github.com/pythops/bluetui).

## Building from Source

Run the following:
```bash
git clone https://github.com/jsypal04/Calenter.git
make
```

This command produces three artifacts: `build/calenter`, `build/calenter-notification-daemon`,
and `libcalenter.so`. The first artifact is the app binary. The seconds artifact is the daemon process that sends
notifications when the event gets close enough (configurable, see below). The third artifact is a shared library
containing the code shared between the two executables.

## Config File

You may create a config file at `~/.config/calenter/config`. It uses the
following basic syntax:
```
key=value
```

Currently, there are only options to add a remote calendar and to configure notification settings.
An example config is included below.

```
remote_url=<your gcal url>
enable_notifications=true
notify_time=10
```
Notes on config options:
- `remote_url` should be a permalink to a .ics file
- `enable_notifications` is the master toggle for the notification daemon; if it is false, then the daemon will be stopped.
- `notify_time` is the amount of time in minutes before an event that the daemon will send
a notification.

## Installation

Run the following commands:
```bash
git clone https://github.com/jsypal04/Calenter.git
sudo make install
```

## Feature List

This is a list of features I want to add.

- [x] Ability to add all day events
- [ ] Repeat rules
    - [ ] Store event date-time data as a `struct tm`
    - [ ] Write algorithm to process BYxxx rules
- [ ] Multi-day all day events
- [x] Native ics parser
- [ ] Google Calendar Integrations (read and write)
- [ ] Ability for configuration (need to flesh this out)
    - [x] Notifications
    - [ ] Colors
    - [ ] Layout
- [ ] Add cursors to input fields

## Bug List

This is a list of known bugs.

1. If lines of an ICS file end in LF instead of CRLF it causes a seg fault
    - This is not really a bug since by the RFC, ICS file lines must end int CRLF
      but it would probably be good to add fault handling.
2. When the app launches the daemon process the terminal stays open after the app is closed.
