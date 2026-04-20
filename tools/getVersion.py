#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Request version info from a Butler device over MQTT using the stateProbe endpoint "version".

Behavior:
- Subscribe to <prefix>versionData to receive probe responses.
- Publish an empty message to <prefix>version to request a snapshot.
- Print the returned payload.

Example:
  ./getVersion.py -d butler0011223344556677/ -b iothub.local
"""

import argparse
import threading
import time
import sys

import paho.mqtt.client as mqtt

DEFAULT_BROKER = 'iothub.local'


def main():
    parser = argparse.ArgumentParser(description='Request version from device over MQTT (stateProbe).')
    parser.add_argument('-d', '--device_id', default=None, help='Optional device id prefix (include trailing slash if you want). Example: "butler0011.../"')
    parser.add_argument('-b', '--broker', default=DEFAULT_BROKER, help='MQTT broker hostname')
    parser.add_argument('-t', '--timeout', type=float, default=2.0, help='Response timeout in seconds')
    args = parser.parse_args()

    prefix = ''
    if args.device_id:
        prefix = args.device_id
        if not prefix.endswith('/'):
            prefix = prefix + '/'

    topic_req = prefix + 'version'
    topic_data = prefix + 'versionData'

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

    client = mqtt.Client('getVersion')
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

    # Print the payload as-is
    print(response_payload)


if __name__ == '__main__':
    main()
