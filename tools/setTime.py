#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Set time of PC to butler

"""

import paho.mqtt.client as mqtt
import time
import threading
import argparse

wrSem = threading.Semaphore(0)

def on_publish(self, userdata, msg):
    wrSem.release()

if __name__ == "__main__":
    parser = argparse.ArgumentParser("Sets the time to butler")
    parser.add_argument('-d', '--device_id', default=None, help = "Optional device id") 
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    opt = parser.parse_args()

    prefix= ''
    if opt.device_id is not None:
        prefix = opt.device_id + '/'
    
    client = mqtt.Client("setTime.py");
    client.on_publish = on_publish
    client.connect(opt.broker)
    client.loop_start()
    now = time.asctime()
    client.publish(prefix+"time/set", now)
    wrSem.acquire()
    client.loop_stop()
