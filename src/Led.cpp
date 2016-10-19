#include "Led.hpp"

Led blinkLed;

void led_on();
void led_off();
void led_duty_cycle();

// Led::Led(uint8_t led_pin, TickerScheduler* scheduler)
//     : _led_pin(led_pin), _scheduler(scheduler), _led_status_changed(false), _led_status(false), _tasks_need_sync(true), _period(1000), _duty_cycle(50)
// {
//     pinMode(_led_pin, OUTPUT);
//
//     if (!_scheduler->add(2, 1000, led_on, true))
//         Serial.println("ERROR: Could not create the led blinking (on) task");
//     if (!_scheduler->add(3, 1000, led_off, false))
//         Serial.println("ERROR: Could not create the led blinking (off) task");
//     if (!_scheduler->add(4, 500, led_duty_cycle, false))
//         Serial.println("ERROR: Could not create the led blinking synchronisation task");
// }

Led::Led()
    : _led_status_changed(false), _led_status(false), _tasks_need_sync(true), _period(1000), _duty_cycle(50)
{
}

void Led::init(uint8_t led_pin, TickerScheduler* scheduler)
{
    _led_pin = led_pin;
    _scheduler = scheduler;

    pinMode(_led_pin, OUTPUT);

    if (!_scheduler->add(2, _period, led_on, true))
        Serial.println("ERROR: Could not create the led blinking (on) task");
    if (!_scheduler->add(3, _period, led_off, false))
        Serial.println("ERROR: Could not create the led blinking (off) task");
    if (!_scheduler->add(4, (_duty_cycle / 100.0 * _period), led_duty_cycle, false))
        Serial.println("ERROR: Could not create the led blinking synchronisation task");
}

void Led::update()
{
    if (_led_status_changed) {
        // digitalWrite(4, led_status ? HIGH : LOW);
        analogWrite(4, _led_status ? 700 : 0);
        _led_status_changed = false;
    }
}

bool Led::set_duty_cycle(uint8_t new_duty_cycle)
{
    if (new_duty_cycle > 99 || new_duty_cycle < 1)
        return false;

    if (_scheduler->changePeriod(4, (new_duty_cycle / 100.0 * _period))) {
        _tasks_need_sync = true;
        return true;
    }

    return false;
}

bool Led::set_period(uint16_t new_period)
{
    _period = new_period;

    bool success = _scheduler->disable(2)
        & _scheduler->disable(3)
        & _scheduler->changePeriod(2, _period)
        & _scheduler->changePeriod(3, _period)
        & set_duty_cycle(_duty_cycle)
        & _scheduler->enable(2)
        & _scheduler->enable(3);

    if (success)
        _tasks_need_sync = true;

    return success;
}

bool Led::led_status()
{
    return _led_status;
}

void Led::set_led_status(bool status)
{
    _led_status = status;
    _led_status_changed = true;

    if (_tasks_need_sync) {
        _scheduler->enable(4);
    }
}

bool Led::sync_tasks()
{
    bool status = _scheduler->disable(3)
        & _scheduler->enable(3)
        & _scheduler->disable(4);

    if (status) {
        _led_status = false;
        _tasks_need_sync = false;
    }

    return status;
}

void led_on()
{
    blinkLed.set_led_status(true);
}

void led_off()
{
    blinkLed.set_led_status(false);
}

void led_duty_cycle()
{
    if (!blinkLed.sync_tasks())
        Serial.println("ERROR: could not synchronise the tasks in charge of led blinking");
}