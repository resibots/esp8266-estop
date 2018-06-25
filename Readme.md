# ESP8266-based emergency stop button

This is the code for a wifi emergency stop built with the esp8266 wifi + microcontroller (tested with an Adafruit Feather HUZZAH with ESP8266). The board we use has battery management included and allows to measure the battery voltage.

This device will send UDP packets over the wifi network, following a simple ad hoc protocol. The consumer of these packets expects them to arrive at a given frequency. If for some set time, no packet arrived, it would stop. As a result, if there is a problem in the network communication or in the emergency stop itself, the consumer would not miss a press on the emergency button.

Used pins on the board:

- It will read the current battery voltage (of our 3.7V, 1000mAh battery), using the only ADC (Analogue to Digital Conversion) pin available.
- It will blink a led wired to the pin 5 of the board
- The pin EN (enable) is pulled down when the emergency stop is triggered, switching off the device.

## Configuring the device

There are two sets of parameters. The first ones are hard-coded in the source code, like the signature key (which should hence never be committed). The second set of parameters can be changed through the serial interface (type `h` and Enter to see the options). For more details, have a look at `config.hpp` and `config.cpp`.

## Dependency

We rely on [TickerScheduler] for a basic time-based scheduling of the different tasks which are 1. blinking the led (three tasks) 2. handling the configuration 3. sending heartbeats.

## Remarks

One of the fields of the heartbeat messages is the battery charge level. This information is based on the voltage on the battery and is very noisy. It is apparently mainly due to the ESP8266 Wifi module that disrupts analogue readings.