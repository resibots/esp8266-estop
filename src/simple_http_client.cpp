#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TickerScheduler.h>

const char* ssid = "LarsenRobots";
const char* password = "Httl3)7GZZA3/V\\M-dm4sfjLSaH2vfb";
//const char* ssid = "Linksys00509";
//const char* password = "myveryunsafepassword";

IPAddress staticIP(152, 81, 70, 17);
IPAddress gateway(152, 81, 70, 1);
IPAddress subnet(255, 255, 255, 0);

// Create the class for UDP communication
WiFiUDP Udp;

unsigned int localPort = 1043;

char packet_buffer[255]; // buffer for incoming data;

// Configuration of the ROS gateway
IPAddress node_ip(152, 81, 10, 184);
unsigned int target_port = 1042;

void setup(void)
{
    Serial.begin(115200);
    Serial.println();

    Serial.printf("Connecting to %s\n", ssid);
    //WiFi.begin(ssid);
    Serial.printf("Password %s\n", password);
    WiFi.begin(ssid, password);
    WiFi.config(staticIP, gateway, subnet);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        //Serial.print(".");
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
    Serial.println();
    Serial.print("Connected, IP address: ");
    Serial.println(WiFi.localIP());
    uint8_t macAddr[6];
    WiFi.macAddress(macAddr);
    Serial.printf("Mac address: %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

    // Setup of UDP communication
    Udp.begin(localPort);
}

void loop()
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

    char message[] = "A nice message from emergencies";
    int status = 0;
    // Send a packet to the ROS emergency stop gateway
    status = Udp.beginPacket(node_ip, target_port);
    if (status == 0)
        Serial.println("Problen in IP or port for packet preparation");
    status = Udp.write(message, sizeof(message));
    //  Serial.print("Amount of data sent: ");
    //  Serial.println(status);

    status = Udp.endPacket();
    if (status == 1)
        Serial.println("The packet was sent successfully.");
    Serial.println("Sending the message");
    Serial.println(message);
    Serial.print("To IP ");
    Serial.print(node_ip);
    Serial.print(" and port ");
    Serial.println(target_port);
    delay(1000);
}
