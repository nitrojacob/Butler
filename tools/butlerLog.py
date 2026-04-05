#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep  1 22:04:51 2022

@author: jacob
"""

import paho.mqtt.client as mqtt
import threading
import argparse
import time

rdSem = threading.Semaphore(0)

# List of Device IDs to log
deviceList = []

def on_message(self, userdata, msg):
    knownTopic = False
    for device in deviceList:
        if(msg.topic == device+"/log"):
            ascii = msg.payload.decode('ascii')
            print(time.asctime(), ":", device, ":", ascii, flush=True)
            knownTopic =  True
    if not knownTopic:
        print(time.asctime(), ":", "Unknown topic:", msg.topic, str(msg.payload), flush=True)
    rdSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Reads the time from butler")
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    parser.add_argument('-l', '--device_list', help='Path to file containing device names, one per line')
    opt = parser.parse_args()

    # If a device list file was provided, read device names from it.
    if opt.device_list:
        try:
            with open(opt.device_list, 'r') as f:
                for raw in f:
                    name = raw.strip()
                    # ignore empty lines and comments
                    if not name or name.startswith('#'):
                        continue
                    deviceList.append(name)
        except Exception as e:
            parser.error(f"Unable to read device list file '{opt.device_list}': {e}")

    client = mqtt.Client("butlerLog.py")
    client.on_message = on_message
    client.connect(opt.broker)

    print(time.asctime(), ": ------------------Starting logger------------------", flush=True)
    
    client.loop_start()
    for device in deviceList:
        client.subscribe(device+"/log")

    try:
        while True:
            rdSem.acquire()
    finally:
        print(time.asctime(), ": ------------------Stopping logger------------------", flush=True)
        client.loop_stop()
