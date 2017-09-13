// #include <TimeLib.h>
// #include <SHA256.h>

// #include "pulse.hpp"
// #include "globals.hpp"


// void Pulse::send_pulse()
// {
//     // Times
//     time_t time_s = now();
//     unsigned long time_ms = millis();

//     // Array lengths
//     size_t times_length = sizeof(time_s) + sizeof(time_ms);
//     size_t message_length = times_length + 32;
    
//     // array of bytes for the message
//     char times[times_length];
//     //memset(times, 0, times_length); TODO: do we need this ?
//     // add times to the times to hash
//     memcpy(times, &time_s, sizeof(time_s));
//     memcpy(times+sizeof(time_s), &time_ms, sizeof(time_ms));
//     // hash the times into packet_buffer
//     hmac(times, times_length, packet_buffer, 32);

//     // append the times to packet_buffer, for the recipient to check the pulse
//     memcpy(packet_buffer+32, &times, times_length);

//     // Pretty-print the result of the message preparation
//     // char read_hash[32];
//     // memcpy(read_hash, packet_buffer, 32);
//     // time_t read_s;
//     // memcpy(&read_s, packet_buffer+32, sizeof(time_t));
//     // unsigned long read_ms;
//     // memcpy(&read_ms, packet_buffer+32+sizeof(time_t), sizeof(unsigned long));
//     // // Print hash bytes
//     // for (uint8_t i=0; i<32; ++i)
//     // {
//     //     Serial.print((int)read_hash[i], HEX);
//     // }
//     // Serial.print("; ");
//     // for (uint8_t i=32; i<32+times_length; ++i)
//     // {
//     //     Serial.print((int)read_hash[i], HEX);
//     // }
//     // Serial.print("; ");
//     // Serial.print(read_s, DEC);
//     // Serial.print(" (s)\t");
//     // Serial.print(read_ms, DEC);
//     // Serial.println(" (ms)");
    
//     // Send a packet to the ROS emergency stop gateway
//     int status = 0;
//     status = Udp.beginPacket(_conf.recipient_ip, _conf.recipient_port);
//     if (status == 0)
//         Serial.println("Problen in IP or port for packet preparation");
//     status = Udp.write(packet_buffer, message_length);
//     //  Serial.print("Amount of data sent: ");
//     //  Serial.println(status);

//     status = Udp.endPacket();
//     if (status == 0) {
//         Serial.print("Could not send a UDP packet to IP ");
//         Serial.print(_conf.recipient_ip);
//         Serial.print(" and port ");
//         Serial.println(_conf.recipient_port);
//     }
//     // Serial.println("Sending the message");
//     // Serial.println(message);
//     // Serial.print("To IP ");
//     // Serial.print(recipient_ip);
//     // Serial.print(" and port ");
//     // Serial.println(recipient_port);
// }

// void Pulse::hmac(const char* message, const size_t message_size, char* hmac,
//     const size_t hmac_size)
// {
//     SHA256 hash;
//     // initialize hmac algorithm
//     hash.resetHMAC(_conf.key, _conf.key_size);
//     // add the message
//     hash.update(message, message_size);
//     // copy the hash to the output
//     hash.finalizeHMAC(_conf.key, _conf.key_size, hmac, hmac_size);

//     // Serial.print("Hmac output: ");
//     // for (uint8_t i=0; i<sizeof(hmac); ++i)
//     // {
//     //     Serial.println((int)hmac[i], HEX);
//     // }
// }