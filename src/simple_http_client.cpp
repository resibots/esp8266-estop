#include <functional>

#include <ESP8266WiFi.h>
// #include <EEPROM.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <TickerScheduler.h>
#include <SHA256.h>

// Class in charge of blinking a led
#include "Led.hpp"

// Functions getting current time from an NTP server
#include "ntp.hpp"

// Global variables and settings
#include "globals.hpp"

// Configuration server
#include "config.hpp"

//  Object for UDP communication
WiFiUDP Udp;
char packet_buffer[512]; // buffer for incoming data;
//  Instance of the task scheduler
TickerScheduler scheduler(2 + 3 + 1);

Configuration conf(Udp, packet_buffer, scheduler);

// Function prototypes
void send_pulse();
void read_battery_voltage();

void hmac(const char* message, const size_t message_size, char* hmac, const size_t hmac_size){
    SHA256 hash;
    char key[] = "16:40:35";
    hash.resetHMAC(key, sizeof(key));
    Serial.print("Message size: "); Serial.println ((int)message_size);
    hash.update(message, message_size);
    hash.finalizeHMAC(key, sizeof(key), hmac, hmac_size);
    // Serial.print("Hmac output: ");
    // for (uint8_t i=0; i<sizeof(hmac); ++i)
    // {
    //     Serial.println((int)hmac[i], HEX);
    // }
}

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

    // Setup the scheduler
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

    // // initialise the pulse counter with a very basic number
    // unsigned int voltage = analogRead(A0);
    // unsigned long milliseconds = millis();
    // pulse_seed = (voltage * milliseconds) % 4096;
    // Serial.printf("Seed for the pulses : %u\n", pulse_seed);

    Serial.print("now has ");
    Serial.print((int)sizeof(time_t));
    Serial.println(" bytes");
    Serial.print("millis has ");
    Serial.print((int)sizeof(unsigned long));
    Serial.println(" bytes");
}

void loop()
{
    scheduler.update();
    blinkLed.update();

    delay(10);
}

void send_pulse()
{
    // pulse_seed = (pulse_seed + 1) % 65536;
    
    // TODO: this was the current code : pseudo-random number
    // conf.suivant = conf.suivant * 1103515245 + 12345;
    // conf.pulse_seed = ((unsigned)(conf.suivant/65536) % 32768);
    // sprintf(packet_buffer, "tick %u", conf.pulse_seed);
    time_t time_s = now();
    unsigned long time_ms = millis();
    size_t times_length = sizeof(time_s) + sizeof(time_ms);
    size_t message_length = times_length + 32;
    //        type of now      type of millis
    char times[times_length];
    memset(times, 0, times_length);

    // add times to the times to hash
    memcpy(times, &time_s, sizeof(time_s));
    memcpy(times+sizeof(time_s), &time_ms, sizeof(time_ms));
    // hash the times into packet_buffer
    hmac(times, times_length, packet_buffer, 32);
    // append the times to packet_buffer, for the recipient to check
    memcpy(packet_buffer+32, &times, times_length);

    // Pretty-print the result of the message preparation
    char read_hash[32];
    memcpy(read_hash, packet_buffer, 32);
    time_t read_s;
    memcpy(&read_s, packet_buffer+32, sizeof(time_t));
    unsigned long read_ms;
    memcpy(&read_ms, packet_buffer+32+sizeof(time_t), sizeof(unsigned long));
    // char read_time[sizeof(double)];
    // memcpy(read_time, packet_buffer+32, sizeof(double));
    // // Print time bytes
    // for (uint8_t i=0; i<sizeof(double); ++i)
    // {
    //     Serial.print((int)read_time[i], HEX);
    // }
    // Serial.print('\n');
    // Print hash bytes
    for (uint8_t i=0; i<32; ++i)
    {
        Serial.print((int)read_hash[i], HEX);
    }
    Serial.print("; ");
    for (uint8_t i=32; i<32+times_length; ++i)
    {
        Serial.print((int)read_hash[i], HEX);
    }
    Serial.print("; ");
    Serial.print(read_s, DEC);
    Serial.print(" (s)\t");
    Serial.print(read_ms, DEC);
    Serial.println(" (ms)");
    
    // Serial.print("Current timestamp: ");
    // Serial.println(now());
    // Serial.println((double)now() + (double)millis()*1e-6);
    // Serial.println((double)millis()*1e-6);

    // randomSeed(0);
    // long randNumber = random(4096);
    // Serial.printf("aleatoire : %u\n", randNumber);

    // Send a packet to the ROS emergency stop gateway
    int status = 0;
    status = Udp.beginPacket(conf.recipient_ip, conf.recipient_port);
    if (status == 0)
        Serial.println("Problen in IP or port for packet preparation");
    status = Udp.write(packet_buffer, message_length);
    //  Serial.print("Amount of data sent: ");
    //  Serial.println(status);

    status = Udp.endPacket();
    if (status == 0) {
        Serial.print("Could not send an UDP packet to IP ");
        Serial.print(conf.recipient_ip);
        Serial.print(" and port ");
        Serial.println(conf.recipient_port);
    }
    // Serial.println("Sending the message");
    // Serial.println(message);
    // Serial.print("To IP ");
    // Serial.print(recipient_ip);
    // Serial.print(" and port ");
    // Serial.println(recipient_port);
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