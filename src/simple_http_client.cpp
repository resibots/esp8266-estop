#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <TickerScheduler.h>

// Class in charge of blinking a led
#include "Led.hpp"

// Functions getting current time from an NTP server
#include "ntp.hpp"

// Global variables and settings
#include "globals.hpp"

// Global settings
//  Wifi configuration
const char* ssid = "LarsenRobots";
const char* password = "Httl3)7GZZA3/V\\M-dm4sfjLSaH2vfb";
//  Network configuration
const IPAddress staticIP(152, 81, 70, 17);
const IPAddress gateway(152, 81, 70, 1);
const IPAddress subnet(255, 255, 255, 0);
const IPAddress dns_server_primary(152, 81, 1, 128);
const IPAddress dns_server_secondary(152, 81, 1, 25);
//  Seed for the pulses sent to the ROS node
uint32_t pulse_seed;
uint32_t suivant = 1;
//  Global settings
const uint16_t config_port = 1043; // local port on which we listen for configuration
IPAddress recipient_ip(152, 81, 10, 184); // IP address of the computer to which we
                                          // send the pulses
uint16_t recipient_port = 1042; // port to which the pulses are sent
uint32_t pulse_period = 500; // period of the pulse (ms)
//  Object for UDP communication
WiFiUDP Udp;
char packet_buffer[512]; // buffer for incoming data;
//  Instance of the task scheduler
TickerScheduler scheduler(2 + 3 + 1);

// Function prototypes
void send_pulse();
void update_configuration();
void help_message(char* buffer);
void save_settings();
void read_settings();
void read_battery_voltage();

time_t prevDisplay = 0; // when the digital clock was displayed

void printDigits(int digits);
void digitalClockDisplay();

void setup(void)
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println('\n');

    // Initialize EEPROM
    static const uint32_t byte_space = 4 + 2 + 4; // memory space used by the
    // settings: 4 for the IP, 2 for the port, 4 for the pulse period
    EEPROM.begin(byte_space);
    read_settings();

    // Connect to the Wifi network
    // We only leave the loop when connected. In the meantime the status is
    // printed to the serial connection.
    WiFi.begin(ssid, password);
    WiFi.config(staticIP, gateway, subnet, dns_server_primary, dns_server_secondary);
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
    Udp.begin(config_port);
    
    
        // Get the time from NTP server
        // Serial.println("Starting UDP for NTP");
        // Udp.begin(NTPPort);
        Serial.println("waiting for sync");
        setSyncProvider(getNtpTime);

    // Setup the scheduler
    // if (!scheduler.add(0, pulse_period, send_pulse, true))
    //     Serial.println("ERROR: Could not create the pulse task");
    // if (!scheduler.add(1, 1000, update_configuration, true))
    //     Serial.println("ERROR: Could not create the configuration task");
    // // CAUTION : Tasks 2 to 4 are reserved for the led blinking class
    // if (!scheduler.add(5, 300, read_battery_voltage, true))
    //     Serial.println("ERROR: Could not create the battery monitoring task");

    // // initialisation of the led blinking class
    // // pin for the led: 4
    // blinkLed.init(4, &scheduler);

    // // initialise the pulse counter with a very basic number
    // unsigned int voltage = analogRead(A0);
    // unsigned long milliseconds = millis();
    // pulse_seed = (voltage * milliseconds) % 4096;
    // Serial.printf("Seed for the pulses : %u\n", pulse_seed);
}

void digitalClockDisplay()
{
    // digital clock display of the time
    Serial.print(hour());
    printDigits(minute());
    printDigits(second());
    Serial.print(" ");
    Serial.print(day());
    Serial.print(".");
    Serial.print(month());
    Serial.print(".");
    Serial.print(year()); 
    Serial.println(); 
}

void printDigits(int digits)
{
    // utility for digital clock display: prints preceding colon and leading 0
    Serial.print(":");
    if(digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void loop()
{
    if (timeStatus() != timeNotSet) {
        if (now() != prevDisplay) { //update the display only if time has changed
            prevDisplay = now();
            digitalClockDisplay();  
        }
    }
    scheduler.update();
    blinkLed.update();

    delay(10);
}

void send_pulse()
{
    char message[126];

    // pulse_seed = (pulse_seed + 1) % 65536;
    suivant = suivant * 1103515245 + 12345;
    pulse_seed = ((unsigned)(suivant/65536) % 32768);
    sprintf(message, "tick %u", pulse_seed);
    // randomSeed(0);
    // long randNumber = random(4096);
    // Serial.printf("aleatoire : %u\n", randNumber);

    // Send a packet to the ROS emergency stop gateway
    int status = 0;
    status = Udp.beginPacket(recipient_ip, recipient_port);
    if (status == 0)
        Serial.println("Problen in IP or port for packet preparation");
    status = Udp.write(message, sizeof(message));
    //  Serial.print("Amount of data sent: ");
    //  Serial.println(status);

    status = Udp.endPacket();
    if (status == 0) {
        Serial.print("Could not send an UDP packet to IP ");
        Serial.print(recipient_ip);
        Serial.print(" and port ");
        Serial.println(recipient_port);
    }
    // Serial.println("Sending the message");
    // Serial.println(message);
    // Serial.print("To IP ");
    // Serial.print(recipient_ip);
    // Serial.print(" and port ");
    // Serial.println(recipient_port);
}

void update_configuration()
{
    // If there is an incoming packet, read it
    int packet_size = Udp.parsePacket();

    if (packet_size) {

        // Read the packet into packet_buffer
        int len = Udp.read(packet_buffer, 255);
        if (len > 0) {
            packet_buffer[len] = 0;

            // Special case: use wants to [r]ead the current settings
            if (packet_buffer[0] == 'r') {
                sprintf(packet_buffer,
                    "Recipient IP   %s\n"
                    "Recipient port %u\n"
                    "Pulse period   %u\n",
                    recipient_ip.toString().c_str(),
                    recipient_port,
                    pulse_period);
            }
            else if (len > 2 && packet_buffer[1] != ' ') {
                // Store the parameters in a String and remove empty characters
                String parameter(packet_buffer + 2);
                parameter.trim(); // remove front and trailing spaces but above
                // all newline character

                // tell whether the setting change was successful
                bool success = false;

                // Select the command to execute based on first received
                // character
                switch (packet_buffer[0]) {
                case 't': // update pulse period
                {
                    // retrieve the desired period
                    uint32_t new_pulse_period = labs(parameter.toInt());

                    // Change the period of the pulse task (change value and
                    // update task)
                    success = scheduler.changePeriod(0, new_pulse_period)
                        & scheduler.disable(0) // required for the change to be
                        & scheduler.enable(0); // effective
                    if (!success)
                        sprintf(packet_buffer, "ERROR: Could not change the pulse period\n");
                    else {
                        pulse_period = new_pulse_period;
                        sprintf(packet_buffer, "New period : %u ms\n", pulse_period);
                    }

                    break;
                }
                case 'a': // change the recipient address
                {
                    // attempt to update the recipient IP ( andchecking if the
                    // IP is valid)
                    if (!(success = recipient_ip.fromString(parameter))) {
                        sprintf(packet_buffer, "ERROR: Could not change the "
                                               "recipient IP from string %s\n"
                                               "Current address %s\n",
                            parameter.c_str(), recipient_ip.toString().c_str());
                    }
                    else {
                        sprintf(packet_buffer, "New recipient IP : %s\n",
                            recipient_ip.toString().c_str());
                    }

                    break;
                }
                case 'p': // change the recipient port
                {
                    // retrieve the desired port number
                    uint16_t new_port = labs(parameter.toInt());

                    // check that the port is in the allowed range 0..65535
                    // and use it
                    if (new_port > 65535) {
                        sprintf(packet_buffer, "ERROR: Could not change the "
                                               "recipient port to%u\n",
                            new_port);
                        success = true;
                    }
                    else {
                        recipient_port = new_port;
                        sprintf(packet_buffer, "New recipient port : %u\n",
                            recipient_port);
                    }

                    break;
                }
                case 'd': // change the led's duty cycle
                {
                    uint8_t new_duty_cycle = labs(parameter.toInt());

                    if (new_duty_cycle > 99 || new_duty_cycle < 1) {
                        sprintf(packet_buffer, "ERROR: requested duty cycle %u% "
                                               "is out of range of [1, 99].\n",
                            new_duty_cycle);
                    }
                    else {
                        blinkLed.set_duty_cycle(new_duty_cycle);
                        sprintf(packet_buffer, "Duty cycle changed to %u%\n",
                            new_duty_cycle);
                    }
                    break;
                }
                case 'e': // change the led's blink period
                {
                    uint16_t new_period = labs(parameter.toInt());

                    if (!blinkLed.set_period(new_period)) {
                        sprintf(packet_buffer, "ERROR: failed to set blinking "
                                               "period to %u (probably due to "
                                               "TickerScheduler)\n",
                            new_period);
                    }
                    else {
                        sprintf(packet_buffer, "Blink period changed to %u\n",
                            new_period);
                    }

                    break;
                }
                case 'h': // TODO: print a help message with all accepted commands
                default:
                    help_message(packet_buffer);
                }

                // save the settings in EEPRO if the change was successful
                if (success)
                    save_settings();
            }
            else {
                help_message(packet_buffer);
            }
        }

        // send a reply, to the IP address and port that sent us the packet wee received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(packet_buffer);
        Udp.endPacket();

        // Print the same reply to the serial line
        Serial.print(packet_buffer);
    }
}

void help_message(char* buffer)
{
    sprintf(buffer,
        "Commands are single-character. Accepted ones are\n"
        "\tt change pulse period (time), in ms\n"
        "\ta change recipient ip [a]ddress\n"
        "\tp change recipient [p]ort\n"
        "\tr read settings currently used.\n"
        "Examples:\n"
        "\tt 500           set pulse period to 500 ms\n"
        "\ta 152.81.70.3   set recipient IP\n"
        "\tp 1042          set recipient port to 1042\n"
        "\nThe wifi SSID and password and network settings of the emergency stop"
        " are hard-coded in the firmware.\n");
}

void save_settings()
{
    EEPROM.put(0, (uint32_t)recipient_ip);
    EEPROM.put(4, recipient_port);
    EEPROM.put(6, pulse_period);
    EEPROM.commit();
}

void read_settings()
{
    uint32_t temp_ip;
    EEPROM.get(0, temp_ip);
    recipient_ip = temp_ip;
    EEPROM.get(4, recipient_port);
    EEPROM.get(6, pulse_period);
    Serial.printf(
        "Recipient IP   %s\n"
        "Recipient port %u\n"
        "Pulse period   %u\n",
        recipient_ip.toString().c_str(),
        recipient_port,
        pulse_period);
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