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
deviceList = [""]

def on_message(self, userdata, msg):
    knownTopic = False
    for device in deviceList:
        if(msg.topic == device+"log"):
            ascii = msg.payload.decode('ascii')
            print(time.asctime(), ":", device, ":", ascii)
            knownTopic =  True
    if not knownTopic:
        print(time.asctime(), ":", "Unknown topic:", msg.topic, str(msg.payload))
    rdSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Reads the time from butler")
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    opt = parser.parse_args()

    client = mqtt.Client("butlerLog.py")
    client.on_message = on_message
    client.connect(opt.broker)

    print(time.asctime(), ": ------------------Starting logger------------------")
    
    client.loop_start()
    for device in deviceList:
        client.subscribe(device+"log")

    try:
        while True:
            rdSem.acquire()
    finally:
        print(time.asctime(), ": ------------------Stopping logger------------------")
        client.loop_stop()
