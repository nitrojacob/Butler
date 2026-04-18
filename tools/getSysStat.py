#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Request sysStat from a Butler device over MQTT using the stateProbe endpoint "sysStat".

Behavior:
- Subscribe to <prefix>sysStatData to receive probe responses.
- Publish an empty message to <prefix>sysStat to request a snapshot.
- Print the returned payload (expected lines TaskName:HighWaterMark).

Example:
  ./getSysStat.py -d butler0011223344556677/ -b iothub.local
"""

import argparse
import threading
import time
import sys

import paho.mqtt.client as mqtt

DEFAULT_BROKER = 'iothub.local'


def main():
    parser = argparse.ArgumentParser(description='Request sysStat from device over MQTT (stateProbe).')
    parser.add_argument('-d', '--device_id', default=None, help='Optional device id prefix (include trailing slash if you want). Example: "butler0011.../"')
    parser.add_argument('-b', '--broker', default=DEFAULT_BROKER, help='MQTT broker hostname')
    parser.add_argument('-t', '--timeout', type=float, default=2.0, help='Response timeout in seconds')
    args = parser.parse_args()

    prefix = ''
    if args.device_id:
        prefix = args.device_id
        if not prefix.endswith('/'):
            prefix = prefix + '/'

    topic_req = prefix + 'sysStat'
    topic_data = prefix + 'sysStatData'

    recv_cv = threading.Condition()
    response_payload = None

    def on_connect(client, userdata, flags, rc):
        client.subscribe(topic_data)

    def on_message(client, userdata, msg):
        nonlocal response_payload
        if msg.topic == topic_data:
            # payload may contain embedded NUL; keep up to first NUL
            response_payload = msg.payload.split(b'\x00', 1)[0].decode('utf-8', errors='replace')
            with recv_cv:
                recv_cv.notify()

    client = mqtt.Client('getSysStat')
    client.on_connect = on_connect
    client.on_message = on_message
    client.connect(args.broker)
    client.loop_start()

    # small delay to ensure subscription
    time.sleep(0.2)

    # Publish request
    client.publish(topic_req, payload=None)

    # Wait for response
    with recv_cv:
        got = recv_cv.wait(timeout=args.timeout)

    client.loop_stop()
    client.disconnect()

    if not got or response_payload is None:
        print('No response received (timeout {}).'.format(args.timeout))
        sys.exit(2)

    # Print the payload as-is (sysStat returns lines Task:mark)
    # Trim trailing NUL if present
    print(response_payload)


if __name__ == '__main__':
    main()
