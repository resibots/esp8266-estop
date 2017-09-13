#ifndef ESTOP_PULSE_HPP
#define ESTOP_PULSE_HPP

#include "config.hpp"

class Pulse
{
public:
    Pulse(const Configuration& conf):
        _conf(conf){}
    
    /** Send a pulse, or heartbeat, to a given IP address.
        It is made of a message and the HMAC of this message, using a secret
        key. The message is the concatenation of Unix timestamp in seconds and
        milliseconds (UTC).

        The key is also known of the recipient that uses it to authenticate the
        pulse sender and (hopefully) prevent an attacker to impersonate the
        emergency stop button.

        We use time as the message, so that pulses that are too old are
        discarded (to limit the risk of relay attack).
    **/
    void send_pulse(void);
protected:
    void hmac(const char* message, const size_t message_size, char* hmac,
        const size_t hmac_size);

    const Configuration& _conf;
};

#endif