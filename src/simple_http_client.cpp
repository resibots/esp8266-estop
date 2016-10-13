#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TickerScheduler.h>

// Networking configuration
const char* ssid = "LarsenRobots";
const char* password = "Httl3)7GZZA3/V\\M-dm4sfjLSaH2vfb";

IPAddress staticIP(152, 81, 70, 17);
IPAddress gateway(152, 81, 70, 1);
IPAddress subnet(255, 255, 255, 0);

// Create the class for UDP communication
WiFiUDP Udp;
char packet_buffer[255]; // buffer for incoming data;

// Global settings
uint16_t config_port = 1043; // local port on which we listen for configuration
// update commands
IPAddress node_ip(152, 81, 10, 184); // IP address of the computer to which we
// send the pulses
uint16_t target_port = 1042; // port to which the pulses are sent
uint32_t pulse_period = 500; // period of the pulse

// Create an instance of the task scheduler
TickerScheduler scheduler(2);

// Function prototypes
void send_pulse();
void update_configuration();

void setup(void)
{
    // Initialize the serial port
    Serial.begin(115200);
    Serial.println();

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
    // if (!scheduler.add(1, 1000, update_configuration, true))
    //     Serial.println("ERROR: Could not create the configuration task");
}

void loop()
{
    scheduler.update();
    delay(10);
}

void send_pulse()
{
    char message[] = "tick";

    // randomSeed(0);
    long randNumber = random(4096);
    Serial.printf("rand Arduino : %u\n", randNumber);

    int status = 0;
    // Send a packet to the ROS emergency stop gateway
    status = Udp.beginPacket(node_ip, target_port);
    if (status == 0)
        Serial.println("Problen in IP or port for packet preparation");
    status = Udp.write(message, sizeof(message));
    //  Serial.print("Amount of data sent: ");
    //  Serial.println(status);

    status = Udp.endPacket();
    if (status == 0) {
        Serial.print("Could not send an UDP packet to IP ");
        Serial.print(node_ip);
        Serial.print(" and port ");
        Serial.println(target_port);
    }
    // Serial.println("Sending the message");
    // Serial.println(message);
    // Serial.print("To IP ");
    // Serial.print(node_ip);
    // Serial.print(" and port ");
    // Serial.println(target_port);
}

void update_configuration()
{
    // If there is an incoming packet, read it
    int packet_size = Udp.parsePacket();

    if (packet_size) {
        Serial.print("Received packet of size ");
        Serial.println(packet_size);
        Serial.print("From ");
        IPAddress remote_ip = Udp.remoteIP();
        Serial.print(remote_ip);
        Serial.print(", port ");
        Serial.println(Udp.remotePort());

        // Read the packet into packet_buffer
        int len = Udp.read(packet_buffer, 255);
        if (len > 0) {
            packet_buffer[len] = 0;
        }
        Serial.println("Contents:");
        Serial.println(packet_buffer);

        // send a reply, to the IP address and port that sent us the packet wee received
        Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
        Udp.write(packet_buffer);
        Udp.endPacket();
    }
}