// Standard
#include <functional> // for `bind`

// Libraries
#include <ESP8266WiFi.h>
#include <SHA256.h>
#include <TickerScheduler.h>
#include <TimeLib.h>
#include <WiFiUdp.h>

// Project-specific classes, functions and variables
// Class in charge of blinking a led
#include "Led.hpp"
// Functions getting current time from an NTP server
#include "ntp.hpp"
// Heartbeat generator and sender for heartbeat
#include "heartbeat.hpp"
// Global variables and settings
#include "globals.hpp"
// Configuration server
#include "config.hpp"

#include "Arduino.h"

//  Object for UDP communication
WiFiUDP Udp;
char packet_buffer[512]; // buffer for incoming and outgoing data

//  Instance of the task scheduler
TickerScheduler scheduler(2 + 3);

// Manage and expose the configuration of the emergency stop
// It also offers a configuration interface either through UDP or serial
// communication.
Configuration conf(Udp, scheduler, Configuration::CommunicationMode::serial);

// Object sending heartbeat heartbeats through the network
Heartbeat heartbeat(conf, Udp, packet_buffer);

void setup(void)
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println('\n');

    // Print MAC address, in case we need it for wifi setup
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    // Initialize EEPROM
    static const uint32_t byte_space = 4 + 2 + 4; // memory space used by the
    // settings: 4 for the IP, 2 for the port, 4 for the heartbeat period
    EEPROM.begin(byte_space);
    conf.load();

    // Connect to the Wifi network
    // We only leave the loop when connected. In the meantime the status is
    // printed to the serial connection.
    WiFi.begin(conf.ssid, conf.password);
    WiFi.config(conf.staticIP, conf.gateway, conf.subnet, conf.dns_server_primary, conf.dns_server_secondary);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        String status = "";
        switch (WiFi.status()) {
        case WL_CONNECTED:
            status = "connected";
            break;
        case WL_NO_SSID_AVAIL:
            status = "no SSID available";
            break;
        case WL_CONNECT_FAILED:
            status = "connection failed";
            break;
        case WL_IDLE_STATUS:
            status = "idle";
            break;
        case WL_DISCONNECTED:
            status = "disconnected";
            break;
        default:
            status = "unknown status";
        }
        Serial.println(status);
    }
    // Once connected, print on the serial port the affected IP address, as well
    // as the MAC address
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());

    // Setup of UDP communication with the port on which to listen
    Udp.begin(conf.config_port); // used for NTP and configuration (if enabled)

    // Get the time from NTP server
    Serial.println("waiting for Network Time Protocol sync");
    setSyncProvider(getNtpTime);
    // Tells how often the time is synchronised
    // By default, synchronisation is done every five minutes
    // setSyncInterval(3600); // in seconds
    Serial.print("Time status: ");
    switch (timeStatus()) {
    case timeNotSet:
        Serial.println("not set");
        break;
    case timeSet:
        Serial.println("set");
        break;
    case timeNeedsSync:
        Serial.println("needs sync");
        break;
    }

    Serial.print("Listening on UDP port ");
    Serial.println(conf.config_port);

    // Setup the scheduler
    std::function<void(void)> send_heartbeat = std::bind(&Heartbeat::send_heartbeat, &heartbeat, true);
    if (!scheduler.add(0, conf.heartbeat_period, send_heartbeat, true))
        Serial.println("ERROR: Could not create the heartbeat task");
    std::function<void(void)> update_configuration = std::bind(&Configuration::update, &conf);
    if (!scheduler.add(1, 1000, update_configuration, true))
        Serial.println("ERROR: Could not create the configuration task");

    // initialisation of the led blinking class
    // pin for the led: 5
    blinkLed.init(5, &scheduler);
}

void loop()
{
    scheduler.update();
    blinkLed.update();

    delay(10);
}