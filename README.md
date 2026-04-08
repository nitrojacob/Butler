# Project Butler
![ESP8266 Build](https://github.com/nitrojacob/Butler/workflows/ESP8266_BUILD/badge.svg)

An IoT device that adheres with my fail safe IoT architecture.
ie. If internet fails, the system operates completely within local network
If local network also fails, (eg. wifi jamming) the device tries best to continue operating

In the fully hardened case, devices should have no internet access. All access should be through a linux iothub.local machine.
If power and local network both fails, and if a local rtc chip is not connected, then
the device have no way of finding out current time and it fails.

Unless otherwise specified, we assume a linux ubuntu 24.04 development environment.

## Features
* Stateful Execution - Traditional IOT devices have an event based behaviour. Events happen and the devices respond. If device misses the event due to power failure or internet failure, the device will be in the wrong state. Here we try to calculate the expected state at each instant and the calculated state is effected.
* Cron based on NVS Flash - Cron schedules are persistent across power cycles.
* Two modes of operation:
  * Standalone Mode: Configuration and monitoring over Provisioning Interface.
  * Network Mode: Configuration and monitoring over MQTT
* OTA - over the air firmware update handled through fota.c/h
* Meta Actuators - Through actuator.c/h you can define more complex functions that eg. produce PWM output.

## How to Build and flash
### ESP8266
Depends on ESP_8266_RTOS_SDK master/latest.

In the root folder of this repo, you can
* Clean: rm -rf build
* Build: make
* Flash: make flash
* Monitoring the logs: make monitor
* Configuring the project: make menuconfig

### ESP32
Depends on v6.0 of esp-idf master branch.
In the root folder of this repo, you can
  * Clean: rm -rf build
  * Set Target: idf.py set-target esp32s2
  * Build: idf.py build
  * Flash: idf.py flash
  * Monitor: idf.py monitor
  * Configuring: idf.py menuconfig

## How to flash using OTA
1. Make your changes.
2. make
3. cd www
4. cp ../build/butler.bin .
5. Start https server in current dir using command
   openssl s_server -WWW -key ../server_certs/ca_key.pem -cert ../server_certs/ca_cert.pem -port 8070
6. Send mqtt message butler/upgrade to the client to start the ota. (mqtt broker is iothub.local)
7. In case you forgot the passphrase for the certificate you need to generate a new certificate and key:
 a. cd ../server_certs
 b. openssl req -x509 -newkey rsa:2048 -keyout ca_key.pem -out ca_cert.pem -days 3660

## Configuring the Device
* Standalone Mode: Android app for configuration [is available in my other repo](https://github.com/nitrojacob/ButlerManager).  
* Network Mode: A set of python tools for interaction through MQTT is in tools directory.  
  * A web-ui is avialable at [ButlerWeb](https://github.com/nitrojacob/ButlerWeb)  

## Common Problems
### MQTT not connecting.
Most likely a mDNS resolution issue. See if you can ping from your laptop to the MQTT address.
If the MQTT broker is hosted locally on laptop do
sudo systemctl restart avahi-daemon

#### Mosquitto versions
Starting v2.x the behaviour of mosquitto server has drastically changed.
For legacy behaviour of unauthorized access create /etc/mosquitto/conf.d/iot.conf with following cofiguration
listener 1883
allow_anonymous true
then invoke mosquitto with mosquitto -c /etc/mosquitto/mosquitto.conf


### Issue with tty
Depending upon board you have to run 'make menuconfig' > 'Serial Flasher Config' > 'Default Serial Port' and change the value to /dev/ttyUSB0 or /dev/ttyACM0


### Issues faced with Over The Air (OTA) updates.
Many attempts were done (over 2 days effort) to get the OTA up and running. Primary issue was with the partition table csv.
The one that comes with SDK for two app had only 0x4000 NVS, which was insufficient for our application.
Then there were issues with partition alignment etc once NVS was made large enough.
At last https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/api-guides/partition-tables.html came in handy,
and a correct partition table could be created as ./partition_two_ota_largeNVS.csv which solved all issues.
