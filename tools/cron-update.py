#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Communicates with the butler project's nvcron subsystem to add or remove cron entries over mqtt.
"""

import paho.mqtt.client as mqtt
import struct, argparse, csv, threading

lineFmt = "<BBBB"
oFmt = "{0:>4}, {1:>4}, {2:>4}, {3:>4}"
rdSem = threading.Semaphore(0)
wrSem = threading.Semaphore(0)

def decode_nvCron(msg):
    pend = msg
    size = struct.calcsize(lineFmt)
    print("Payload size (Bytes) = ", len(pend))
    print(oFmt.format("mins", "hrs", "func", "arg"))
    while(len(pend) >= size):
        mins, hrs, func, arg = struct.unpack(lineFmt, pend[:size])
        print(oFmt.format(mins, hrs, func, arg))
        pend = pend[size:]
    
def readCronFile(fileName):
    fp = open(fileName)
    buf = b''
    for line in csv.reader(fp):
        print(line)
        buf += struct.pack(lineFmt, int(line[0]), int(line[1]), int(line[2]), int(line[3]))
    fp.close()
    return buf
    
def on_publish(self, userdata, msg):
    wrSem.release()

def on_message(self, userdata, msg):
    if(msg.topic == prefix+"nvCron/rdData"):
        decode_nvCron(msg.payload)
    else:
        print("Unknown topic")
        print(msg.topic, str(msg.payload))
    rdSem.release()
    

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description = "Update Cron entries in an nvCron_mqtt based system over mqtt")
    parser.add_argument("csv", nargs="?" , help="[Optional] csv file of cron entries that is to be written to remote client. Each line corresponds to (min, hr, func, arg)")
    parser.add_argument('-d', '--device_id', default=None, help = "Optional device id") 
    parser.add_argument('-b', '--broker', default='iothub.local', help='The broker url to which to connect')
    
    args = parser.parse_args()

    prefix= ''
    if args.device_id is not None:
        prefix = args.device_id + '/'
    
    client = mqtt.Client("cron-PC");
    client.on_message = on_message
    client.on_publish = on_publish
    client.connect(args.broker)
    client.loop_start()
    client.subscribe(prefix+"nvCron/rdData")
    client.publish(prefix+"nvCron/rd")
    wrSem.acquire()
    rdSem.acquire()
    
    if args.csv:
        print(args.csv, "file provided for writing")
        payload = readCronFile(args.csv)
        client.publish(prefix+"nvCron/wr", payload)
        wrSem.acquire()
    #client.loop_forever()
    client.loop_stop()
