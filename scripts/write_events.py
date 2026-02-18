#!/usr/bin/python3
'''
Writes events parsed from a .ics file to calendar.txt
'''

from datetime import date, datetime
import sys
import os
import re
from parse_ics import parse_ics

HOME_DIR = os.environ["HOME"]
CALENDAR_PATH = f"{HOME_DIR}/.calendar/calendar.txt"

def write_events(events):
    with open(CALENDAR_PATH, 'r') as calendar_txt:
        calendar = calendar_txt.readlines()

    for id in events:
        if events[id]["RRULE"] is not None:
            handle_repeate_rule(events[id], calendar)
            continue

        start = events[id]["DTSTART"]
        summary = events[id]["SUMMARY"]

        year = start[0:4]
        month = start[4:6]
        day = start[6:8]
        hour = None
        minute = None

        if "T" in start:
            time = start[start.find("T") + 1:-1]
            hour = time[0:2]
            minute = time[2:4]

            # Time is in UTC timezone (Greenwich) need to convert to NYC
            if "Z" in start and int(hour) >= 5:
                hour = str((int(hour) - 5) % 24)
            elif "Z" in start and int(hour) < 5:
                hour = str((int(hour) - 5) % 24)
                day = int(day) - 1

        pattern = fr"{year}-{month}-{day}"
        matches = [(i, s) for i, s in enumerate(calendar) if re.search(pattern, s)]

        assert len(matches) <= 2 and len(matches) > 0

        match = matches[0] if len(matches) == 1 else matches[1]
        updated_match = add_event(match[1], hour, minute, summary)

        calendar[match[0]] = updated_match

    with open(CALENDAR_PATH, 'w') as calendar_txt:
        calendar_txt.writelines(calendar)


def handle_repeate_rule(event, calendar):
    start = event["DTSTART"]
    year = start[0:4]
    month = start[4:6]
    day = start[6:8]

    matches = None

    match event["RRULE"]["FREQ"]:
        case "YEARLY":
            pattern = fr"-{month}-{day}"
            matches = [(i, s) for i, s in enumerate(calendar) if re.search(pattern, s)]
        case "WEEKLY":
            print(event["RRULE"])
            repeat_days = event["RRULE"].get("BYDAY")

            repeat_days_pattern = None
            if repeat_days is None:
                repeat_days_pattern = map_weekday(date(int(year), int(month), int(day)).weekday())
            else:
                repeat_days_pattern = "|".join(list(map(lambda d: f"({map_weekday(d)})", repeat_days.split(","))))

            pattern = fr'{repeat_days_pattern}'
            prelim_matches = [(i, s) for i, s in enumerate(calendar) if re.search(pattern, s)]

            matches = []
            for match in prelim_matches:
                dstart = datetime(int(year), int(month), int(day))
                dmatch = datetime(int(match[1][0:4]), int(match[1][5:7]), int(match[1][8:10]))

                if "UNTIL" in event["RRULE"].keys():
                    dend = datetime(
                        int(event["RRULE"]["UNTIL"][0:4]),
                        int(event["RRULE"]["UNTIL"][4:6]),
                        int(event["RRULE"]["UNTIL"][6:8])
                    )

                    if dstart <= dmatch <= dend:
                        matches.append(match)
                else:
                    if dstart <= dmatch:
                        matches.append(match)
        case _:
            print("Unkown repeat rule frequency")
            return

    assert matches is not None

    hour = None
    minute = None

    if "T" in start:
        time = start[start.find("T") + 1:-1]
        hour = time[0:2]
        minute = time[2:4]

        # Time is in UTC timezone (Greenwich) need to convert to NYC
        if "Z" in start and int(hour) >= 5:
            hour = str((int(hour) - 5) % 24)
        elif "Z" in start and int(hour) < 5:
            hour = str((int(hour) - 5) % 24)
            day = int(day) - 1

    for match in matches:
        if len(match[1][0:18].strip()) < 18:
            continue
        updated_match = add_event(match[1], hour, minute, event["SUMMARY"])
        calendar[match[0]] = updated_match


def add_event(date, hour, minute, summary):
    date_prefix = date[0:18]
    days_events = list(
        filter(
            lambda e: len(e) > 0,
            map(lambda e: e.strip(), date[18:].split(","))
        )
    )

    if hour is None or minute is None:
        new_event = f"ALL DAY - {summary}"
        for e in days_events:
            hyphen_index = e.find("-")
            if e[hyphen_index + 1:].strip().lower() == summary.strip().lower():
                print(f"Cannot add event {new_event}. There is another ALL DAY event with the same summary.")
                return date_prefix + '  ' + ','.join(days_events) + '\n'
        days_events.insert(0, new_event)

    else:
        new_event = f"{hour}:{minute} - {summary}"

        if len(days_events) == 0:
            days_events.append(new_event)
        else:
            insertion_index = None
            for i, e in enumerate(days_events):
                e_time = extract_time(e)

                if compare_time((int(hour), int(minute)), e_time) == 1:
                    continue
                elif compare_time((int(hour), int(minute)), e_time) == 0:
                    print(f"Cannot add event {new_event}. There is another event at this time.")
                    return date_prefix + '  ' + ','.join(days_events) + '\n'

                insertion_index = i
                break

            if insertion_index is not None:
                days_events.insert(insertion_index, new_event)
            else:
                days_events.append(new_event)

    return date_prefix + '  ' + ','.join(days_events) + '\n'

def map_weekday(weekday):
    if type(weekday) is int:
        match weekday:
            case 0:
                return "Mon"
            case 1:
                return "Tue"
            case 2:
                return "Wed"
            case 3:
                return "Thu"
            case 4:
                return "Fri"
            case 5:
                return "Sat"
            case 6:
                return "Sun"

    match weekday:
        case "MO":
            return "Mon"
        case "TU":
            return "Tue"
        case "WE":
            return "Wed"
        case "TH":
            return "Thu"
        case "FR":
            return "Fri"
        case "SA": # May not be the right one for saturday
            return "Sat"
        case "SU":
            return "Sun"


def extract_time(calendar_event: str) -> tuple[int, int]:
    if calendar_event[0] == "A":
        return (0, 0)

    hour = calendar_event[0:2]
    minute = calendar_event[3:5]

    return (int(hour), int(minute))


def compare_time(time1, time2):
    '''
    Returns 0 if they are the same time, 1 if time 1 is later,
    and 2 if time 2 is later

    The times are the following format: (hour, minute)
    '''

    if time1[0] > time2[0]:
        return 1
    elif time1[0] < time2[0]:
        return 2

    if time1[1] > time2[1]:
        return 1
    elif time1[1] < time2[1]:
        return 2

    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Expected a path to a .ics file")
        exit(1)

    ics_file_path = sys.argv[1]

    events = parse_ics(ics_file_path)
    write_events(events)
