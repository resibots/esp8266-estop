#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <WiFiUdp.h>
#include <TickerScheduler.h>

// Networking configuration
const char* ssid = "LarsenRobots";
const char* password = "Httl3)7GZZA3/V\\M-dm4sfjLSaH2vfb";

IPAddress staticIP(152, 81, 70, 17);
IPAddress gateway(152, 81, 70, 1);
IPAddress subnet(255, 255, 255, 0);

// Seed for the pulses sent to the ROS node
uint16_t pulse_seed;

// Create the class for UDP communication
WiFiUDP Udp;
char packet_buffer[512]; // buffer for incoming data;

// Global settings
uint16_t config_port = 1043; // local port on which we listen for configuration
// update commands
IPAddress recipient_ip(152, 81, 10, 184); // IP address of the computer to which we
// send the pulses
uint16_t recipient_port = 1042; // port to which the pulses are sent
uint32_t pulse_period = 500; // period of the pulse

// Create an instance of the task scheduler
TickerScheduler scheduler(2);

// Function prototypes
void send_pulse();
void update_configuration();
void help_message(char* buffer);
void save_settings();
void read_settings();

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
    WiFi.begin(ssid, password);
    WiFi.config(staticIP, gateway, subnet);
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

    // Setup the scheduler
    if (!scheduler.add(0, pulse_period, send_pulse, true))
        Serial.println("ERROR: Could not create the pulse task");
    if (!scheduler.add(1, 1000, update_configuration, true))
        Serial.println("ERROR: Could not create the configuration task");

    unsigned int voltage = analogRead(A0);
    unsigned long milliseconds = millis();
    pulse_seed = (voltage * milliseconds) % 4096;
    Serial.printf("Seed for the pulses : %u\n", pulse_seed);
}

void loop()
{
    scheduler.update();
    delay(10);
}

void send_pulse()
{
    char message[] = "tick";

    pulse_seed = (pulse_seed + 1) % 65536;
    sprintf(message, "%u", pulse_seed);
    // randomSeed(0);
    // long randNumber = random(4096);
    // Serial.printf("aleatoire : %u\n", randNumber);

    int status = 0;
    // Send a packet to the ROS emergency stop gateway
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
            else if (len > 2) {
                // Store the parameters in a String and remove empty characters
                String parameter(packet_buffer + 1);
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