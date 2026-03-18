# PC/Python based tools to control and configure Butler

## Pre-requesites
pip3 install paho-mqtt

## Sample Cron Config
samples/cron_cfg.txt

### Format
min, hr, on/off, channel  
Followin Configuration turn on Channel/Relay 0 at 22:05 (10:05 PM) and turns off at 22:06  
05, 22, 1, 0  
06, 22, 0, 0  

