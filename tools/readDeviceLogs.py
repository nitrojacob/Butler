#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Request persistent logs from a Butler device over MQTT using the stateProbe endpoint "plogRd".

Behaviour (matching nvLogRing.c):
- Subscribe to <prefix>plogRdData to receive probe responses.
- Publish an empty message to <prefix>plogRd to request one log entry. Each request advances the
  device-side read pointer.
- Repeat up to NVLOGRING_MAX_ENTRIES+1 times (default 32+1) to retrieve the entire ring buffer.
- The device will return a single entry per request. A blank entry is indicated by a payload
  starting with '-' (single dash + NUL). Use its position to reorder collected messages so that
  output is oldest -> newest.

Example:
  ./readDeviceLogs.py -d butler0011223344556677/ -b iothub.local
"""

import argparse
import threading
import time
import sys

import paho.mqtt.client as mqtt


DEFAULT_BROKER = 'iothub.local'
# Default NVLOGRING_MAX_ENTRIES from main/board_cfg.h
DEFAULT_MAX_ENTRIES = 32


def reorder_messages(msgs):
    """Reorder messages according to nvLogRing read semantics.

    msgs: list of strings (payloads). One of them should be '-' indicating the blank slot.
    Returns list of messages oldest->newest (skips '-' and '?' placeholders).
    """
    L = len(msgs)
    if L == 0:
        return []

    # Find index of blank ('-') if present
    blank_idx = None
    for i, m in enumerate(msgs):
        if m == '-' or (len(m) == 1 and m == '-'):
            blank_idx = i
            break

    # If blank not found, fall back to returning msgs in the order starting at 0
    if blank_idx is None:
        print("Unable to find Start Marker")
        # Filter out placeholders and return in the order received (best-effort)
        return [m for m in msgs if m not in ('-', '?')]

    # Build ordered list starting after blank_idx and wrap until before blank_idx
    ordered = []
    for offset in range(1, L):
        idx = (blank_idx + offset) % L
        m = msgs[idx]
        if m in ('-', '?'):
            continue
        ordered.append(m)

    return ordered


def main():
    parser = argparse.ArgumentParser(description='Read device persistent logs over MQTT (nvLogRing).')
    parser.add_argument('-d', '--device_id', default=None, help='Optional device id prefix (include trailing slash if you want). Example: "butler0011.../"')
    parser.add_argument('-b', '--broker', default=DEFAULT_BROKER, help='MQTT broker hostname')
    parser.add_argument('-n', '--max_entries', type=int, default=DEFAULT_MAX_ENTRIES, help='NVLOGRING_MAX_ENTRIES on the device (default 32)')
    parser.add_argument('-t', '--timeout', type=float, default=2.0, help='Per-request timeout in seconds')
    args = parser.parse_args()

    prefix = ''
    if args.device_id:
        # ensure prefix ends with a slash like device MQTT name does in firmware
        prefix = args.device_id
        if not prefix.endswith('/'):
            prefix = prefix + '/'

    topic_req = prefix + 'plogRd'
    topic_data = prefix + 'plogRdData'

    received = []
    recv_cv = threading.Condition()


    def on_connect(client, userdata, flags, rc):
        # Subscribe to data topic on connect
        client.subscribe(topic_data)

    def on_message(client, userdata, msg):
        if msg.topic == topic_data:
            # payload may contain embedded NUL; keep as decoded str up to first NUL
            payload = msg.payload.split(b'\x00', 1)[0].decode('utf-8', errors='replace')
            with recv_cv:
                received.append(payload)
                recv_cv.notify()

    client = mqtt.Client('readDeviceLogs')
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.broker)
    client.loop_start()

    # Give client time to connect and subscribe
    time.sleep(0.2)

    total_requests = args.max_entries + 1

    for i in range(total_requests):
        # Publish empty payload to request one entry
        client.publish(topic_req, payload=None)

        # Wait for a response or timeout
        with recv_cv:
            got = recv_cv.wait(timeout=args.timeout)
            if not got:
                # No message received for this request
                received.append('?')
            # else on_message already appended

    client.loop_stop()
    client.disconnect()

    # Reorder and print
    ordered = reorder_messages(received)

    if not ordered:
        print('No logs found.')
        # For debugging, print raw received
        print('Raw received (in read order):')
        for i, m in enumerate(received):
            print('{:02d}: {!r}'.format(i, m))
        sys.exit(0)

    # Print ordered logs oldest -> newest
    for line in ordered:
        print(line)


if __name__ == '__main__':
    main()
