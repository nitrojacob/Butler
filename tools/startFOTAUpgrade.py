#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Created on Thu Sep  1 22:04:51 2022

@author: jacob
"""

import paho.mqtt.client as mqtt
import threading, argparse

wrSem = threading.Semaphore(0)

def on_publish(self, userdata, msg):
    wrSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Initiates FOTA update of butler")
    parser.add_argument('-d', '--device_id', default=None, help = "Optional device id") 
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    opt = parser.parse_args()

    prefix= ''
    if opt.device_id is not None:
        prefix = opt.device_id + '/'
    client = mqtt.Client('startFOTAUpgrade.py')
    client.on_publish = on_publish
    client.connect(opt.broker)
    
    client.loop_start()
    client.publish(prefix+"butler/upgrade")
    wrSem.acquire()
    client.loop_stop()
