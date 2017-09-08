#include "config.hpp"

void Configuration::update()
{
    // If there is an incoming packet, read it
    int packet_size = _udp.parsePacket();

    if (packet_size) {

        // Read the packet into _packet_buffer
        int len = _udp.read(_packet_buffer, 255);
        if (len > 0) {
            _packet_buffer[len] = 0;

            // Special case: use wants to [r]ead the current settings
            if (_packet_buffer[0] == 'r') {
                sprintf(_packet_buffer,
                    "Recipient IP   %s\n"
                    "Recipient port %u\n"
                    "Pulse period   %u\n",
                    recipient_ip.toString().c_str(),
                    recipient_port,
                    pulse_period);
            }
            else if (len > 2 && _packet_buffer[1] != ' ') {
                // Store the parameters in a String and remove empty characters
                String parameter(_packet_buffer + 2);
                parameter.trim(); // remove front and trailing spaces but above
                // all newline character

                // tell whether the setting change was successful
                bool success = false;

                // Select the command to execute based on first received
                // character
                switch (_packet_buffer[0]) {
                case 't': // update pulse period
                {
                    // retrieve the desired period
                    uint32_t new_pulse_period = labs(parameter.toInt());

                    // Change the period of the pulse task (change value and
                    // update task)
                    success = _scheduler.changePeriod(0, new_pulse_period)
                        & _scheduler.disable(0) // required for the change to be
                        & _scheduler.enable(0); // effective
                    if (!success)
                        sprintf(_packet_buffer, "ERROR: Could not change the pulse period\n");
                    else {
                        pulse_period = new_pulse_period;
                        sprintf(_packet_buffer, "New period : %u ms\n", pulse_period);
                    }

                    break;
                }
                case 'a': // change the recipient address
                {
                    // attempt to update the recipient IP ( andchecking if the
                    // IP is valid)
                    if (!(success = recipient_ip.fromString(parameter))) {
                        sprintf(_packet_buffer, "ERROR: Could not change the "
                                               "recipient IP from string %s\n"
                                               "Current address %s\n",
                            parameter.c_str(), recipient_ip.toString().c_str());
                    }
                    else {
                        sprintf(_packet_buffer, "New recipient IP : %s\n",
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
                        sprintf(_packet_buffer, "ERROR: Could not change the "
                                               "recipient port to%u\n",
                            new_port);
                        success = true;
                    }
                    else {
                        recipient_port = new_port;
                        sprintf(_packet_buffer, "New recipient port : %u\n",
                            recipient_port);
                    }

                    break;
                }
                case 'd': // change the led's duty cycle
                {
                    uint8_t new_duty_cycle = labs(parameter.toInt());

                    if (new_duty_cycle > 99 || new_duty_cycle < 1) {
                        sprintf(_packet_buffer, "ERROR: requested duty cycle %u% "
                                               "is out of range of [1, 99].\n",
                            new_duty_cycle);
                    }
                    else {
                        blinkLed.set_duty_cycle(new_duty_cycle);
                        sprintf(_packet_buffer, "Duty cycle changed to %u%\n",
                            new_duty_cycle);
                    }
                    break;
                }
                case 'e': // change the led's blink period
                {
                    uint16_t new_period = labs(parameter.toInt());

                    if (!blinkLed.set_period(new_period)) {
                        sprintf(_packet_buffer, "ERROR: failed to set blinking "
                                               "period to %u (probably due to "
                                               "TickerScheduler)\n",
                            new_period);
                    }
                    else {
                        sprintf(_packet_buffer, "Blink period changed to %u\n",
                            new_period);
                    }

                    break;
                }
                case 'h': // TODO: print a help message with all accepted commands
                default:
                    help_message(_packet_buffer);
                }

                // save the settings in EEPRO if the change was successful
                if (success)
                    save();
            }
            else {
                help_message(_packet_buffer);
            }
        }

        // send a reply, to the IP address and port that sent us the packet wee received
        _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
        _udp.write(_packet_buffer);
        _udp.endPacket();

        // Print the same reply to the serial line
        Serial.print(_packet_buffer);
    }
}

void Configuration::load()
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

void Configuration::save()
{
    EEPROM.put(0, (uint32_t)recipient_ip);
    EEPROM.put(4, recipient_port);
    EEPROM.put(6, pulse_period);
    EEPROM.commit();
}

void Configuration::help_message(char* buffer)
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