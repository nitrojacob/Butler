#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep  1 22:04:51 2022

@author: jacob
"""

import paho.mqtt.client as mqtt
import threading
import argparse

rdSem = threading.Semaphore(0)
wrSem = threading.Semaphore(0)

def on_publish(self, userdata, msg):
    wrSem.release()

def on_message(self, userdata, msg):
    if(msg.topic == prefix+"time/getData"):
        ascii = msg.payload.decode('ascii')
        print("Remote Time:", ascii)
    else:
        print("Unknown topic")
        print(msg.topic, str(msg.payload))
    rdSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Reads the time from butler")
    parser.add_argument('-d', '--device_id', default=None, help = "Optional device id") 
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    opt = parser.parse_args()

    prefix= ''
    if opt.device_id is not None:
        prefix = opt.device_id + '/'
    
    client = mqtt.Client("getTime.py-PC")
    client.on_message = on_message
    client.on_publish = on_publish
    client.connect(opt.broker)
    
    client.loop_start()
    client.subscribe(prefix+"time/getData")
    client.publish(prefix+"time/get")
    wrSem.acquire()
    rdSem.acquire()
    client.loop_stop()
