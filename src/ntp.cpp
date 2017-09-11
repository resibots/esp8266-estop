#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <TimeLib.h>

// Global variables and settings
#include "globals.hpp"

void sendNTPpacket();

unsigned int NTPPort = 8888;  // local port to listen for UDP packets

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

time_t getNtpTime()
{
    const int time_zone = 0; // Set the time zone offset;
    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    Serial.println("Transmit NTP Request");
    sendNTPpacket();
    uint32_t beginWait = millis();
    while (millis() - beginWait < 1500) {
        int size = Udp.parsePacket();
        if (size >= NTP_PACKET_SIZE) {
        Serial.println("Receive NTP Response");
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        return secsSince1900 - 2208988800UL + time_zone * SECS_PER_HOUR;
        }
    }
    Serial.println("No NTP Response :-(");
    return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket()
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:         
  
  // Using http://www.pool.ntp.org/zone/fr
  Udp.beginPacket("pool.ntp.org", 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}

// // These methods are used to display the time. We comment them to use less
// // programm memory, since they are only usefull to debugging.
// void digitalClockDisplay()
// {
//     // digital clock display of the time
//     Serial.print(hour());
//     printDigits(minute());
//     printDigits(second());
//     Serial.print(" ");
//     Serial.print(day());
//     Serial.print(".");
//     Serial.print(month());
//     Serial.print(".");
//     Serial.print(year()); 
//     Serial.println(); 
// }

// void printDigits(int digits)
// {
//     // utility for digital clock display: prints preceding colon and leading 0
//     Serial.print(":");
//     if(digits < 10)
//         Serial.print('0');
//     Serial.print(digits);
// }