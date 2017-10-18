#include <TimeLib.h>
#include <SHA256.h>

#include "heartbeat.hpp"
#include "globals.hpp"


void Heartbeat::send_heartbeat(bool battery_level)
{
    size_t message_length = prepare_heartbeat();

    if (battery_level)
    {
        message_length += prepare_battery_voltage(message_length);
    }

    send_packet(message_length);
}

size_t Heartbeat::prepare_heartbeat()
{
    // Time and message counter
    time_t time_s = now();
    uint16_t message_counter = inc_since_last_sec(time_s);

    // Array lengths
    size_t times_length = sizeof(time_s) + sizeof(message_counter);
    size_t message_length = times_length + _hash_length;
    
    // array of bytes for the message
    char times[times_length];
    //memset(times, 0, times_length); TODO: do we need this ?
    // add times to the times to hash
    memcpy(times, &time_s, sizeof(time_s));
    memcpy(times+sizeof(time_s), &message_counter, sizeof(message_counter));

    // hash the times into _packet_buffer
    hmac(times, times_length, _packet_buffer, _hash_length);

    // append the times to _packet_buffer, for the recipient to check the heartbeat
    memcpy(_packet_buffer+_hash_length, &times, times_length);

    return message_length;
}

uint16_t Heartbeat::inc_since_last_sec(time_t s)
{
    static time_t last_second = 0;
    static uint16_t counter = 0;

    if (s > last_second)
    {
        last_second = s;
        counter = 0;
    }
    else
        ++counter;

    return counter;
}

void Heartbeat::hmac(const char* message, const size_t message_size, char* hmac,
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

size_t Heartbeat::prepare_battery_voltage(size_t offset)
{
    // The analog to digital converter has 10 bits, so ranges from 0 to 1023, for a
    // voltage in the range 0V - 1V.
    int level = analogRead(A0);
    
    // We use a resistor-based voltage divider with R1 and R2 (R2 being the resistor
    // closest to the ground). Values: R1 = 1000 kOhm; R2 = 220 kOhm
    // Battery max voltage is 4.2V corresponding to 774 in digital
    //         min            3.14V                 579
    // Hence, we remap from digital values to percentage of charge
    float percent_charge = (level - 579) * 100.0 / (774 - 579);

    memcpy(_packet_buffer+offset, &percent_charge, sizeof(percent_charge));
    return sizeof(percent_charge);
}

void Heartbeat::send_packet(size_t message_length)
{
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