# Roller blinds and light controller

This project is based on a arduino mega V3 with ethernet shield.
The intention of this project is to control 3 rollerblinds and three led lights via MQTT.

IP address and MQTT broker address are received via DHCP
Base topic received dynamicly via openHab
openHab send the time every minute on topic "openhab/time"
if the time has not been received for 61 seconds we expect "standalone mode"

## Startup sequence

* read address from DS18B20
* use part of the DS18B20 address as ethernet mac address
* retrieve IP address via DHCP
* retrieve MQTT address via option 151 from the DHCP server (custom ethernet library)
* subscribe to MQTT topic "raw/mac/[mac address]"
* send mac address to topic "raw/mac"
* openHab responds to the topic and send the base Topic to use
* subscribe to all topics starting with the received base topic

## MQTT topics

* [baseTopic]/Status/