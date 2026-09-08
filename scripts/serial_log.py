#!/usr/bin/env python3
"""Capture Apollo's USB serial output for the local monitor."""

import os
import time
from pathlib import Path

import serial


DEVICE = os.environ.get('APOLLO_SERIAL_DEVICE', '/dev/cu.usbmodem1101')
LOG = Path(os.environ.get('APOLLO_MONITOR_LOG', '/tmp/apollo_live.log'))


def main():
    LOG.parent.mkdir(parents=True, exist_ok=True)
    with LOG.open('a', buffering=1) as log_file:
        deadline = time.time() + 86_400
        connection = None
        while time.time() < deadline:
            if connection is None:
                try:
                    connection = serial.Serial(DEVICE, 115200, timeout=1)
                    log_file.write('--- reader attached ---\n')
                except Exception:
                    time.sleep(2)
                    continue
            try:
                line = connection.readline().decode('utf-8', 'replace').rstrip()
            except Exception:
                try:
                    connection.close()
                except Exception:
                    pass
                connection = None
                time.sleep(2)
                continue
            if line:
                log_file.write(line + '\n')


if __name__ == '__main__':
    main()
