# iot-esp8266-serial-fwd

Allow to monitor(read) and interact(write) a TTL serial 3.3V over wifi.

<hr/>

<!-- TOC -->
* [features](#features)
* [wirings](#wirings)
* [flash the firmware](#flash-the-firmware)
* [configure wifis](#configure-wifis)
* [application example](#application-example)
* [how this project was built](#how-this-project-was-built)
* [references](#references)
<!-- TOCEND -->

<hr/>

![](doc/desktop.png)

![](doc/mobile.jpg)

## features

- default speed can be changed and saved
- send text ( one line ) can be autouppercased
- multiple wifi routers can be inserted in the [code](https://github.com/devel0/iot-esp8266-serial-fwd/blob/42cefa981820aebe436a02f99a68831d79f1686e/src/main.cpp#L148) and strongest will connected
- if no router available you can connect directly to esp in Access Point mode to its own network ( url http://192.168.4.1 )
- from desktop web browser mDNS allow to locate at http://espserial.local

## wirings

software serial on D5 (RX) and D6 (TX) used to avoid conflicting with GPIO3/GPIO1 used in flashing firmware.

![](doc/wirings.svg)

:warning: be careful to connect 5v serial to esp pins ; to be sure use a simple logic converter level like `level converter mh` based on `BSS138` chip:
- LV to 3.3V of esp
- LV1, LV2 to esp RX, TX
- HV to external serial level (eg. 5V)
- HV1, HV2 to external serial TX, RX

## flash the firmware

- open folder in vscode
- if platformio warn about arduino extension you can disable ( for workspace ) from extensions
- ctrl+shift+u to upload firmware
- `data` files can be uploaded to esp spiffs using platformio tasks/nodemcuv2/Platform/Upload Filesystem image

![](doc/platformio-spiffs.png)

## configure wifis

create your own `.h` file and set into [main.cpp](https://github.com/devel0/iot-esp8266-serial-fwd/blob/42cefa981820aebe436a02f99a68831d79f1686e/src/main.cpp#L19) right path then set them at [startWiFi()](https://github.com/devel0/iot-esp8266-serial-fwd/blob/42cefa981820aebe436a02f99a68831d79f1686e/src/main.cpp#L148) function.

```c
#ifndef _MY_WIFI_KEY_H_
#define _MY_WIFI_KEY_H_
#define WIFI_SSID "myssid"
#define WIFI_KEY "mysecretkey"
#endif
```

## application example

![](doc/example01b.jpg)

![](doc/example01a.jpg)

This box allow you to interact with serial of devices either 3v3 or 5v due to the level converter inside the box.

In the photo above an adrduino serial can be used in parallel either with usb or through wifi connecting to http://espserial.local after device power on.

Esp8266 powered through G, VIN pins by a 9V rechargable LiOn battery.
External red wire ( VREF ) is to be connected to 5V of arduino so that level converter bss138 can recognize the signal and convert to the 3v3 of esp8266.

Notes about wiring lvl converter:
- GND, LV, L1, L2 connects to esp8266 GND, 3V3, D5, D6
- GND, HV, H1, H2 are black, red (VREF), green (RX), blue (TX) external wires.
- green wire ( rx of esp8266 ) is connected to arduino TX.
- blue wire ( tx of esp8266 ) is connected to arduino RX.

## how this project was built

- install arduino
- install vscode
- install platformio
- from platformio / project / new ( board : nodemcu v1.0 )

## references

- [A Beginner's Guide to the ESP8266](https://tttapa.github.io/ESP8266/Chap01%20-%20ESP8266.html)