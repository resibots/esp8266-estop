#include "config.hpp"

void Configuration::update()
{
    // If there is incoming data, read it
    int packet_size = read();

    if (packet_size) {
        _buffer[packet_size] = 0;

        // Special case: use wants to [r]ead the current settings
        if (_buffer[0] == 'r') {
            sprintf(_buffer,
                "Recipient IP   %s\n"
                "Recipient port %u\n"
                "Heartbeat period   %u\n",
                recipient_ip.toString().c_str(),
                recipient_port,
                heartbeat_period);
        }
        else if (_buffer[0] == 's') {
            // save the settings in EEPROM if the change was successful
            if (save())
                sprintf(_buffer, "Settings saved\n");
            else
                sprintf(_buffer, "Settings failed to be saved\n");
        }
        else if (_buffer[0] == 'h') {
            help_message(true);
        }
        else if (packet_size > 2 && _buffer[1] == ' ') {
            // Store the parameters in a String and remove empty characters
            String parameter(_buffer + 2);
            parameter.trim(); // remove front and trailing spaces but above
            // all newline character

            // Select the command to execute based on first received
            // character
            switch (_buffer[0]) {
            case 't': // update heartbeat period
            {
                // retrieve the desired period
                uint32_t new_heartbeat_period = labs(parameter.toInt());

                // tell whether the setting change was successful
                bool success = false;
                // Change the period of the heartbeat task (change value and
                // update task)
                success = _scheduler.changePeriod(0, new_heartbeat_period)
                    & _scheduler.disable(0) // required for the change to be
                    & _scheduler.enable(0); // effective
                if (!success)
                    sprintf(_buffer, "ERROR: Could not change the heartbeat period\n");
                else {
                    heartbeat_period = new_heartbeat_period;
                    sprintf(_buffer, "New period : %u ms\n", heartbeat_period);
                }

                break;
            }
            case 'a': // change the recipient address
            {
                // attempt to update the recipient IP ( andchecking if the
                // IP is valid)
                if (!recipient_ip.fromString(parameter)) {
                    sprintf(_buffer, "ERROR: Could not change the "
                                     "recipient IP from string %s\n"
                                     "Current address %s\n",
                        parameter.c_str(), recipient_ip.toString().c_str());
                }
                else {
                    sprintf(_buffer, "New recipient IP : %s\n",
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
                    sprintf(_buffer, "ERROR: Could not change the "
                                     "recipient port to%u\n",
                        new_port);
                }
                else {
                    recipient_port = new_port;
                    sprintf(_buffer, "New recipient port : %u\n",
                        recipient_port);
                }

                break;
            }
            case 'd': // change the led's duty cycle
            {
                uint8_t new_duty_cycle = labs(parameter.toInt());

                if (new_duty_cycle > 99 || new_duty_cycle < 1) {
                    sprintf(_buffer, "ERROR: requested duty cycle %u% "
                                     "is out of range of [1, 99].\n",
                        (int)new_duty_cycle);
                }
                else {
                    blinkLed.set_duty_cycle(new_duty_cycle);
                    sprintf(_buffer, "Duty cycle changed to %u%\n",
                        (int)new_duty_cycle);
                }
                break;
            }
            case 'e': // change the led's blink period
            {
                uint16_t new_period = labs(parameter.toInt());

                if (!blinkLed.set_period(new_period)) {
                    sprintf(_buffer, "ERROR: failed to set blinking "
                                     "period to %u (probably due to "
                                     "TickerScheduler)\n",
                        new_period);
                }
                else {
                    sprintf(_buffer, "Blink period changed to %u\n",
                        new_period);
                }

                break;
            }
            }
        }
        else {
            help_message(false);
        }

        write();
    }
}

void Configuration::load()
{
    uint32_t temp_ip;
    EEPROM.get(0, temp_ip);
    recipient_ip = temp_ip;
    EEPROM.get(4, recipient_port);
    EEPROM.get(6, heartbeat_period);
    Serial.printf(
        "Recipient IP   %s\n"
        "Recipient port %u\n"
        "Heartbeat period   %u\n",
        recipient_ip.toString().c_str(),
        recipient_port,
        heartbeat_period);
}

bool Configuration::save()
{
    EEPROM.put(0, (uint32_t)recipient_ip);
    EEPROM.put(4, recipient_port);
    EEPROM.put(6, heartbeat_period);
    return EEPROM.commit();
}

void Configuration::help_message(bool full_message)
{
    snprintf(_buffer, _buffer_size,
        "Commands are single-character, optionally followed by one argument. Accepted commands are\n"
        "\tt change heartbeat period (time), in ms\n"
        "\ta change recipient ip [a]ddress\n"
        "\tp change recipient [p]ort\n"
        "\td change led blinking [d]uty cycle\n"
        "\te change l[e]d blinking period, in ms\n"
        "\tr [r]ead settings currently used\n"
        "\ts [s]ave current settings.\n");
    if (full_message) {
        char examples[] = "Examples:\n"
                          "\tt 500           set heartbeat period to 500 ms\n"
                          "\ta 152.81.70.3   set recipient IP\n"
                          "\tp 1042          set recipient port to 1042\n"
                          "\td 61            set duty cycle to 61%\n"
                          "\te 1000          set led blink period to 1000 ms\n"
                          "\nThe wifi SSID and password and network settings of the emergency stop"
                          " are hard-coded in the firmware.\n";
        // only append the examples if the buffer is big enough
        if (strlen(_buffer) + strlen(examples) < _buffer_size)
            strcat(_buffer, examples);
        else {
            // FIXME: this is a sort of hack to get the full help message to be sent.
            write();
            snprintf(_buffer, _buffer_size, examples);
        }
    }
}

size_t Configuration::read(void)
{
    size_t bytes_count = 0;

    if (CommunicationMode::serial == _com_mode) {
        // This code was taken from
        // https://create.arduino.cc/projecthub/mikefarr/simple-command-line-interface-4f0a3f
        size_t len = strlen(_buffer);
        while (Serial.available()) {
            char c = Serial.read();
            switch (c) {
            case '\r':
            case '\n':
                _buffer[len] = '\0'; // terminate the string
                bytes_count = len;
                break;
            case '\b': // backspace, remove one character from the buffer
                if (len > 0) {
                    _buffer[--len] = '\0';
                    // This generates a warning that should be ignored
                    // (about shift biffer than width of type).
                    Serial << byte('\b') << byte(' ') << byte('\b');
                }
                break;
            default:
                if (len < _buffer_size)
                    _buffer[len++] = c;
                _buffer[len] = '\0';
                break;
            }
        }
    }
    else if (CommunicationMode::udp == _com_mode) {
        if (_udp.parsePacket() > 0) {
            bytes_count = _udp.read(_buffer, _buffer_size);
        }
    }

    return bytes_count;
}

size_t Configuration::write(void)
{
    size_t amount_written = 0;
    if (strlen(_buffer) > 0) {
        if (CommunicationMode::serial == _com_mode) {
            // Print the reply to the serial line
            amount_written = Serial.print(_buffer);
            // Mark the buffer as empty, so that the read method can start anew.
            _buffer[0] = '\0';
        }
        else if (CommunicationMode::udp == _com_mode) {
            // send a reply, to the IP address and port that sent us the packet we received
            _udp.beginPacket(_udp.remoteIP(), _udp.remotePort());
            amount_written = _udp.write(_buffer);
            _udp.endPacket();
        }
    }

    return amount_written;
}