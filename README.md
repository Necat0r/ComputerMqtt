# ComputerMqtt

## What is does
Allows turning power off as well as turning the monitor on and off

* Power status is reported as either 'true' or 'false' to ** computer/power/status **
* Suspend the machine by publishing 'false' to ** computer/power/control **
* Turn on/off the monitor by publishing 'true'/'false' to ** computer/monitor/control **

## Building it
See https://github.com/Necat0r/BtMqtt for instructions

## Running
Setup environment to include mosquitto:
`PATH=%PATH%;$(MOSQUITTO_DIR)`

Command line:
`computermqtt <host> <port>`