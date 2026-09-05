#!/usr/bin/env python3
"""hid_dump_joystick.py - raw HID input report dumper for USB joysticks.

Host-side analysis helper used while developing the OPL 'hidpad' joystick
profiles. It is NOT part of the OPL runtime and implements no HID parsing;
it only prints the raw input reports received from the device so their layout
can be inspected.

Requires the Python 'hid' bindings (hidapi):

    pip install hid

Usage:

    python hid_dump_joystick.py [VID] [PID]

VID/PID are hex numbers; they default to 0x0079 0x0006 (DragonRise USB
gamepad). A leading '0x' is optional.

While it runs, press buttons and move the sticks/d-pad; every HID input
report received is printed. Press Ctrl+C to stop.
"""

import sys
import time

import hid

DEFAULT_VID = 0x0079
DEFAULT_PID = 0x0006


def parse_hex(value, default):
    if value is None:
        return default
    try:
        return int(value, 16)
    except ValueError:
        print("error: '%s' is not a valid hex number" % value, file=sys.stderr)
        sys.exit(2)


def main(argv):
    vid = parse_hex(argv[1], DEFAULT_VID) if len(argv) > 1 else DEFAULT_VID
    pid = parse_hex(argv[2], DEFAULT_PID) if len(argv) > 2 else DEFAULT_PID

    dev = hid.device()
    try:
        dev.open(vid, pid)
    except IOError as exc:
        print("error: could not open VID=%04X PID=%04X (%s)" % (vid, pid, exc),
              file=sys.stderr)
        sys.exit(1)

    dev.set_nonblocking(True)

    print("HID dump VID=%04X PID=%04X - press buttons / move sticks, Ctrl+C to stop"
          % (vid, pid))

    try:
        while True:
            data = dev.read(64, timeout_ms=200)
            if data:
                print(list(data), flush=True)
            time.sleep(0.05)
    except KeyboardInterrupt:
        pass
    finally:
        dev.close()


if __name__ == "__main__":
    main(sys.argv)