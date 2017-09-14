#include <functional>

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <TickerScheduler.h>
#include <SHA256.h>

// Class in charge of blinking a led
#include "Led.hpp"

// Functions getting current time from an NTP server
#include "ntp.hpp"

// Pulse generator and sender for heartbeat
#include "pulse.hpp"

// Global variables and settings
#include "globals.hpp"

// Configuration server
#include "config.hpp"

//  Object for UDP communication
WiFiUDP Udp;
char packet_buffer[512]; // buffer for incoming and outgoing data

//  Instance of the task scheduler
TickerScheduler scheduler(2 + 3 + 1);

// Manage and expose the configuration of the emergency stop
// It also offers a configuration interface either through UDP or serial
// communication.
Configuration conf(Udp, scheduler, Configuration::CommunicationMode::serial);

// Object sending heartbeat pulses through the network
Pulse pulse(conf, Udp, packet_buffer);

// Function prototypes
void read_battery_voltage();

void setup(void)
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println('\n');

    // Initialize EEPROM
    static const uint32_t byte_space = 4 + 2 + 4; // memory space used by the
    // settings: 4 for the IP, 2 for the port, 4 for the pulse period
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
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n",
        macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    // Setup of UDP communication with the port on which to listen
    Udp.begin(conf.config_port);
    
    // Get the time from NTP server
    Serial.println("waiting for Network Time Protocol sync");
    setSyncProvider(getNtpTime);
    // Tells how often the time is synchronised
    setSyncInterval(3600); // in seconds
    Serial.print("Time status: ");
    switch(timeStatus()){
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

    Serial.print("Listening on UDP port "); Serial.println(conf.config_port);

    // Setup the scheduler
    std::function<void(void)> send_pulse = std::bind(&Pulse::send_pulse, pulse);
    if (!scheduler.add(0, conf.pulse_period, send_pulse, true))
        Serial.println("ERROR: Could not create the pulse task");
    std::function<void(void)> update_configuration = std::bind(&Configuration::update, conf);
    if (!scheduler.add(1, 1000, update_configuration, true))
        Serial.println("ERROR: Could not create the configuration task");
    // CAUTION : Tasks 2 to 4 are reserved for the led blinking class
    if (!scheduler.add(5, 300, read_battery_voltage, true))
        Serial.println("ERROR: Could not create the battery monitoring task");

    // initialisation of the led blinking class
    // pin for the led: 4
    blinkLed.init(4, &scheduler);
}

void loop()
{
    scheduler.update();
    blinkLed.update();

    delay(10);
}

void read_battery_voltage()
{
    // The analog to digital converter has 10 bits, so ranges from 0 to 1023, for a
    // voltage in the range 0V - 1V.
    int level = analogRead(A0);

    // We use a resistor-based voltage divider with R1 and R2 (R2 being the resistor
    // closest to the ground). Values: R1 = 1000 kOhm; R2 = 220 kOhm
    // Battery max voltage is 4.2V corresponding to 774 in digital
    //         min            3.14V                 579
    // Hence, we remap from digital values to percentage of charge
    double percent_charge = (level - 579) * 100.0 / (774 - 579);
    // Serial.print("Battery charge level is ");
    // Serial.println(percent_charge);
}