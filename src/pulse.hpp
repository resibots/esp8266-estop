#ifndef ESTOP_PULSE_HPP
#define ESTOP_PULSE_HPP

#include "config.hpp"

class Pulse
{
public:
    Pulse(const Configuration& conf, WiFiUDP& Udp, char* packet_buffer):
        _conf(conf),
        _udp(Udp),
        _packet_buffer(packet_buffer) {}
    
    /** Send a pulse, or heartbeat, to a given IP address.
        It is made of a message and the HMAC of this message, using a secret
        key. The message is the concatenation of Unix timestamp in seconds and
        a counter since last second (UTC).

        The key is also known of the recipient that uses it to authenticate the
        pulse sender and (hopefully) prevent an attacker to impersonate the
        emergency stop button.

        We use time as the message, so that pulses that are too old are
        discarded (to limit the risk of relay attack).

        The data is sent as a bynary stream of 32+4+2 bytes. The 32 first bytes
        is the hash. Then come 4 bytes for the timestamp (unsigned long, in
        seconds) and 2 last bytes for the counter (uint16).
    **/
    void send_pulse(bool battery_level = false);
protected:
    size_t prepare_pulse();
    /** Give how many times it was called since last second.
     * For instance, if we calls this method three times within the same second,
     * it will return 0, 1 and 2 successively.
     * 
     * @param s current time in seconds
     * 
     * @return how many times this methods was called since last second
    **/
    uint16_t inc_since_last_sec(time_t s);
    void hmac(const char* message, const size_t message_size, char* hmac,
        const size_t hmac_size);

    size_t prepare_battery_voltage(size_t offset);
    
    void send_packet(size_t message_length);

    const Configuration& _conf;
    WiFiUDP& _udp;
    char* _packet_buffer;

    static constexpr uint8_t _hash_length = 32; // length of the hash in bytes > 256 bits
};

#endif