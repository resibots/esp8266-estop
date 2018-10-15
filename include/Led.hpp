#ifndef LED_HPP_
#define LED_HPP_

#include <ESP8266WiFi.h>
#include <TickerScheduler.h>

class Led {
public:
    // Led(uint8_t led_pin, TickerScheduler* scheduler);
    Led();
    void init(uint8_t led_pin, TickerScheduler* scheduler);

    void update();
    bool set_duty_cycle(uint8_t new_duty_cycle);
    bool set_period(uint16_t new_period);
    // Return true when the led is ON and false otherwise.
    bool led_status();
    void set_led_status(bool status);

    bool sync_tasks();

private:
    // Led();

    uint8_t _led_pin;
    TickerScheduler* _scheduler;

    bool _led_status_changed;
    bool _led_status;
    bool _tasks_need_sync;

    uint16_t _period; // in ms
    uint8_t _duty_cycle; // in percentage, between 1 and 99
};

extern Led blinkLed;

#endif // LED_HPP_