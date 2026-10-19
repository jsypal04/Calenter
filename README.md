# Calenter

Calenter is a TUI frontend for calendar.txt inspired by [Impala](https://github.com/pythops/impala)
and [Bluetui](https://github.com/pythops/bluetui).

## Building from Source

Run the following:
```bash
git clone https://github.com/jsypal04/Calenter.git
make
```

This command produces three artifacts: `build/calenter`, `build/calenter-headless`, `build/calenter-tests`,
and `build/libcalenter.so`. The first artifact is the app binary. The seconds artifact is a headless version of the app that is basically an entrypoint for 
libcalenter (for debugging purposes). The third artifact is the test suite. 
The fourth artifact is a shared library containing the backend code (e.g., reading/writing to calendar.txt,
parsing ics files, a custom array type, etc.).

To run the app without installing it use the following command.
```bash
make run
```
This sets the library paths properly before starting the app which prevents missing library issues.

## Configuration

You may create a config file at `~/.config/calenter/config.ini`.

Currently, there are only options to add a remote calendar and to configure notification settings.
You can add any number of remote urls and calendar.txt will be synced with all of them.
An example config is included below.

```
remote_url = <your gcal url>
remote_url = <your other gcal url>
enable_notifications = true
notify_time = 10
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

## Test Suite

All tests are located in the `tests` directory. The one top level source file, 
`tests.c` collects and executes the test, handling stdout capture and (eventually)
stdout restoration when tests fail. Currently, the stdout for each test is saved
in `logs/test_run.log`. In order to run the test suite, execute the following command:

```bash
make test
```

### Adding tests

To add a new test module follow the steps below:

1. Add a C or C++ file under a suitable directory in the tests source tree.
2. Write your test functions in that file. All test functions should accept no
   arguments and return an int with 0 indicating success and non zero indicating
   failure.
3. Write a function to that returns the testing structure for that module. I have
   been using the nameing convention `make_<module name>_tests`. This function
   should accept no arguments and should return a `struct tests` type. To make
   the tests structure, use the `make_tests` and `add_test` helper functions defined
   in `tests.h`.
4. Add a forward declaration for your make tests function. Call the functions to
   make the tests, run the tests, and destroy the tests in main.

> [!NOTE]
> If the test module is a C++ file, make sure to wrap the `make_<module name>_tests`
> function in a `extern "C"` block.

## Feature List

This is a list of features I want to add.

- [x] Ability to add all day events
- [ ] Repeat rules
    - [x] Store event date-time data as a `struct tm`
    - [x] Enable creating events with repeat rules.
    - [ ] Write algorithm to process BYxxx rules
- [ ] Multi-day all day events
- [x] Native ics parser
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
