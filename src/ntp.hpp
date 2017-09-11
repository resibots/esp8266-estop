#ifndef ESTOP_NTP_HPP
#define ESTOP_NTP_HPP

/** Provider for the Time library.

    This method is to be given to `setSyncProvider` and will request the time
    from an NTP server and set it for later use in the Time library.
**/
time_t getNtpTime();

// // These methods are used to display the time. We comment them to use less
// // programm memory, since they are only usefull to debugging.
// void digitalClockDisplay()
// void printDigits(int digits)

#endif