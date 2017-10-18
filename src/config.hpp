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
    //  key for the heartbeats sent to the ROS node
    static constexpr char const *key = "16:40:35";
    static constexpr size_t key_size = 8;

    IPAddress recipient_ip; // IP address of the computer to which we send the heartbeats
    uint16_t recipient_port; // port to which the heartbeats are sent
    
    uint32_t heartbeat_period; // period of the heartbeat (ms)

    enum class CommunicationMode : uint8_t
    {
        serial,
        udp
    };

    Configuration(WiFiUDP& udp, TickerScheduler& scheduler, CommunicationMode com_mode)
        :_udp(udp),
        _scheduler(scheduler),
        _com_mode(com_mode),
        staticIP(152,81,70,17),
        gateway(152, 81, 70, 1),
        subnet(255, 255, 255, 0),
        dns_server_primary(152, 81, 1, 128),
        dns_server_secondary(152, 81, 1, 25),
        config_port(1043),
        recipient_ip(152, 81, 10, 184),
        recipient_port(1042),
        heartbeat_period(500) {
            _buffer_size = 512;
            _buffer = new char[_buffer_size];
        }
    void update();
    void load();
    void save();
    

protected:
    // These are global variables we are only getting handles to
    WiFiUDP& _udp;
    char* _buffer;
    size_t _buffer_size;
    TickerScheduler& _scheduler; // TODO: Do we need it ?
    CommunicationMode _com_mode;

    void help_message(bool full_message=false);
    size_t read(void);
    size_t write(void);
};

#endif