#include <TimeLib.h>
#include <SHA256.h>

#include "pulse.hpp"
#include "globals.hpp"


void Pulse::send_pulse()
{
    // Times
    time_t time_s = now();
    unsigned long time_ms = millis();

    // Array lengths
    size_t times_length = sizeof(time_s) + sizeof(time_ms);
    size_t message_length = times_length + 32;
    
    // array of bytes for the message
    char times[times_length];
    //memset(times, 0, times_length); TODO: do we need this ?
    // add times to the times to hash
    memcpy(times, &time_s, sizeof(time_s));
    memcpy(times+sizeof(time_s), &time_ms, sizeof(time_ms));
    // hash the times into _packet_buffer
    hmac(times, times_length, _packet_buffer, 32);

    // append the times to _packet_buffer, for the recipient to check the pulse
    memcpy(_packet_buffer+32, &times, times_length);
    
    // Send a packet to the ROS emergency stop gateway
    int status = 0;
    status = _udp.beginPacket(_conf.recipient_ip, _conf.recipient_port);
    if (status == 0)
        Serial.println("Problen in IP or port for packet preparation");
    status = _udp.write(_packet_buffer, message_length);
    //  Serial.print("Amount of data sent: ");
    //  Serial.println(status);

    status = _udp.endPacket();
    if (status == 0) {
        Serial.print("Could not send a UDP packet to IP ");
        Serial.print(_conf.recipient_ip);
        Serial.print(" and port ");
        Serial.println(_conf.recipient_port);
    }
}

void Pulse::hmac(const char* message, const size_t message_size, char* hmac,
    const size_t hmac_size)
{
    SHA256 hash;
    // initialize hmac algorithm
    hash.resetHMAC(_conf.key, _conf.key_size);
    // add the message
    hash.update(message, message_size);
    // copy the hash to the output
    hash.finalizeHMAC(_conf.key, _conf.key_size, hmac, hmac_size);
}