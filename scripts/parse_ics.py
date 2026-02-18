#!/usr/bin/python3
'''
Parses data from an ics file to be written to
calendar.txt

Reads the provided file (must be in ics format) and outputs a
dictionary containing the start time, summary, and repeat rule.
'''

import sys
from datetime import date as dt

def split_key_value_pair(pair):
    colon_index = pair.find(":")
    large_key = pair[:colon_index]
    large_value = pair[colon_index + 1:]

    key = None
    key_params = dict()
    for i, param in enumerate(large_key.split(";")):
        if i == 0 and "=" not in param:
            key = param
            continue
        equals_index = param.find("=")
        key_params[param[0:equals_index]] = param[equals_index + 1:]

    value = None
    value_params = dict()
    for i, param in enumerate(large_value.split(";")):
        if i == 0 and "=" not in param:
            value = param
            continue
        equals_index = param.find("=")
        value_params[param[0:equals_index]] = param[equals_index + 1:]

    return (key, value, key_params, value_params)


def parse_ics(ics_file_path):
    with open(ics_file_path, 'r') as ics_file:
        raw_events = ics_file.read().split("BEGIN:VEVENT")

    events = dict()
    for raw_event in raw_events:
        event_properties = raw_event.split("\n")

        properties = dict()
        for property in event_properties:
            key, value, key_params, value_params = split_key_value_pair(property)

            properties[key] = (value, key_params, value_params)

        id = properties.get("UID")
        start = properties.get("DTSTART")
        summary = properties.get("SUMMARY")
        repeat_rule = properties.get("RRULE")

        if start is None or summary is None or id is None:
            continue

        year = start[0][0:4]

        if int(year) < dt.today().year:
            continue

        event = {
            "DTSTART": start[0],
            "SUMMARY": summary[0],
            "RRULE": repeat_rule[2] if repeat_rule is not None else None
        }
        events[id[0]] = event

    return events


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Expected a path to a .ics file")
        exit(1)

    ics_file_path = sys.argv[1]

    events = parse_ics(ics_file_path)
    print(events)
