#ifndef ESTOP_CONFIG_HPP
#define ESTOP_CONFIG_HPP

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <TickerScheduler.h>

#include "Led.hpp"

/** Configuration server

    This class handles two main tasks:
    - storing and providing global settings
    - handling requests (through UDP packets) to update the configuration

    For the latter, the update() method must be called periodically, to process incoming packets
**/
class Configuration
{
public:
    // Global settings
    //  Wifi configuration
    const char* ssid = "LarsenRobots";
    const char* password = "Httl3)7GZZA3/V\\M-dm4sfjLSaH2vfb";
    //  Network configuration
    const IPAddress staticIP;
    const IPAddress gateway;
    const IPAddress subnet;
    const IPAddress dns_server_primary;
    const IPAddress dns_server_secondary;
    const uint16_t config_port; // local port on which we listen for configuration
    //  key for the pulses sent to the ROS node
    char *key;
    size_t key_size;

    IPAddress recipient_ip; // IP address of the computer to which we send the pulses
    uint16_t recipient_port; // port to which the pulses are sent
    
    uint32_t pulse_period; // period of the pulse (ms)

    Configuration(WiFiUDP& udp, char* packet_buffer, TickerScheduler& scheduler)
        :_udp(udp),
        _packet_buffer(packet_buffer),
        _scheduler(scheduler),
        staticIP(152,81,70,17),
        gateway(152, 81, 70, 1),
        subnet(255, 255, 255, 0),
        dns_server_primary(152, 81, 1, 128),
        dns_server_secondary(152, 81, 1, 25),
        config_port(1043),
        recipient_ip(152, 81, 10, 184),
        recipient_port(1042),
        pulse_period(500)
        {
            char secret_key[] = "16:40:35";
            memcpy(key, secret_key, strlen(secret_key));
            key_size = strlen(secret_key);
        }
    void update();
    void load();
    void save();
    

protected:
    // These are global variables we are only getting handles to
    WiFiUDP& _udp;
    char* _packet_buffer;
    TickerScheduler& _scheduler; // TODO: Do we need it ?

    void help_message(char* buffer);
};

#endif