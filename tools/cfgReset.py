#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Mon March 2, 2026
Reset the nvs flash configurations

@author: jacob
"""

import paho.mqtt.client as mqtt
import threading
import argparse

rdSem = threading.Semaphore(0)
wrSem = threading.Semaphore(0)

def on_publish(self, userdata, msg):
    wrSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Reads the time from butler")
    parser.add_argument('-d', '--device_id', default=None, help = "Optional device id") 
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    opt = parser.parse_args()

    prefix= ''
    if opt.device_id is not None:
        prefix = opt.device_id + '/'
    
    client = mqtt.Client("crfReset.py-PC")
    client.on_publish = on_publish
    client.connect(opt.broker)
    
    client.loop_start()
    client.publish(prefix+"cfgReset")
    wrSem.acquire()
    client.loop_stop()
