/*
The MIT License (MIT)

Copyright (c) 2017 Lancaster University.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
*/

#ifndef CODAL_TIMER_H
#define CODAL_TIMER_H

#include "CodalConfig.h"
#include "ErrorNo.h"
#include "LowLevelTimer.h"

#ifndef CODAL_TIMER_DEFAULT_EVENT_LIST_SIZE
    #define CODAL_TIMER_DEFAULT_EVENT_LIST_SIZE 10
#endif

//
// TimerEvent flags
//
#define CODAL_TIMER_EVENT_FLAGS_NONE   0
#define CODAL_TIMER_EVENT_FLAGS_WAKEUP 0x01

namespace codal {
struct TimerEvent {
    CODAL_TIMESTAMP period;
    CODAL_TIMESTAMP timestamp;
    uint16_t id;
    uint16_t value;
    uint32_t flags;  // We only need one byte, but sizeof(TimerEvent) is still 24

    void set(CODAL_TIMESTAMP timestamp, CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
             uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE)
    {
        this->timestamp = timestamp;
        this->period    = period;
        this->id        = id;
        this->value     = value;
        this->flags     = flags;
    }
};

class Timer {
#if CONFIG_ENABLED(CODAL_TIMER_32BIT)
    uint32_t sigma;
    uint32_t delta;
#else
    uint16_t sigma;
    uint16_t delta;
#endif
    LowLevelTimer& timer;

    /**
     * Synchronises low level timer counter with ours.
     */
    void sync();

    /**
     * request to the physical timer implementation code to provide a trigger callback at the given time.
     * @note it is perfectly legitimate for the implementation to trigger before this time if convenient.
     * @param t Indication that t time units (typically microsends) have elapsed.
     */
    void triggerIn(CODAL_TIMESTAMP t);

    /**
     * Make sure nextTimerEvent is pointing the the right event.
     */
    void recomputeNextTimerEvent();

  public:
    uint8_t ccPeriodChannel;
    uint8_t ccEventChannel;

    /**
     * Constructor for a generic system clock interface.
     *
     * @param t A low-level timer instance that wraps a hardware timer.
     *
     * @param ccPeriodChannel The channel to use a a periodic interrupt as a fallback
     *
     * @param ccEventChannel The channel to use as the next TimerEvents' interrupt.
     */
    Timer(LowLevelTimer& t, uint8_t ccPeriodChannel = 0, uint8_t ccEventChannel = 1);

    /**
     * Retrieves the current time tracked by this Timer instance
     * in milliseconds
     *
     * @return the timestamp in milliseconds
     */
    CODAL_TIMESTAMP getTime();

    /**
     * Retrieves the current time tracked by this Timer instance
     * in microseconds
     *
     * @return the timestamp in microseconds
     */
    CODAL_TIMESTAMP getTimeUs();

    /**
     * Configures this Timer instance to fire an event after period
     * milliseconds.
     *
     * @param period the period to wait until an event is triggered, in milliseconds.
     *
     * @param id the ID to be used in event generation.
     *
     * @param value the value to place into the Events' value field.
     *
     * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
     */
    int eventAfter(CODAL_TIMESTAMP period, uint16_t id, uint16_t value, uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

    /**
     * Configures this Timer instance to fire an event after period
     * microseconds.
     *
     * @param period the period to wait until an event is triggered, in microseconds.
     *
     * @param id the ID to be used in event generation.
     *
     * @param value the value to place into the Events' value field.
     *
     * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
     */
    int eventAfterUs(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                     uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

    /**
     * Configures this Timer instance to fire an event every period
     * milliseconds.
     *
     * @param period the period to wait until an event is triggered, in milliseconds.
     *
     * @param id the ID to be used in event generation.
     *
     * @param value the value to place into the Events' value field.
     *
     * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
     */
    int eventEvery(CODAL_TIMESTAMP period, uint16_t id, uint16_t value, uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

    /**
     * Configures this Timer instance to fire an event every period
     * microseconds.
     *
     * @param period the period to wait until an event is triggered, in microseconds.
     *
     * @param id the ID to be used in event generation.
     *
     * @param value the value to place into the Events' value field.
     *
     * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
     */
    int eventEveryUs(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                     uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

    /**
     * Cancels any events matching the given id and value.
     *
     * @param id the ID that was given upon a previous call to eventEvery / eventAfter
     *
     * @param value the value that was given upon a previous call to eventEvery / eventAfter
     */
    int cancel(uint16_t id, uint16_t value);

    /**
     * Destructor for this Timer instance
     */
    ~Timer();

    /**
     * Callback from physical timer implementation code, indicating a requested time span *may* have been completed.
     */
    void trigger(bool isFallback);

    /**
     * Called from power manager before sleep.
     * @param counter reference to a variable to receive the current timer counter
     *
     * @return the current time since power on in microseconds
     */
    CODAL_TIMESTAMP deepSleepBegin(CODAL_TIMESTAMP& counter);

    /**
     * Called from power manager after sleep.
     * @param counter the current timer counter
     * @param micros time elapsed since deepSleepBegin
     */
    void deepSleepEnd(CODAL_TIMESTAMP counter, CODAL_TIMESTAMP micros);

    /**
     * Determine the time of the next wake up event.
     * @param timestamp reference to a variable to receive the time.
     * @return true if there is an event.
     */
    bool deepSleepWakeUpTime(CODAL_TIMESTAMP& timestamp);

    /**
     * Enables interrupts for this timer instance.
     */
    virtual int enableInterrupts();

    /**
     * Disables interrupts for this timer instance.
     */
    virtual int disableInterrupts();

  protected:
    CODAL_TIMESTAMP currentTime;
    CODAL_TIMESTAMP currentTimeUs;
    uint32_t overflow;

    TimerEvent* timerEventList;
    TimerEvent* nextTimerEvent;
    int eventListSize;

    TimerEvent* getTimerEvent();
    void releaseTimerEvent(TimerEvent* event);
    int setEvent(CODAL_TIMESTAMP period, uint16_t id, uint16_t value, bool repeat, uint32_t flags);
    TimerEvent* deepSleepWakeUpEvent();
};

/*
 *
 * Convenience C API Interface that wraps this class, using the first compatible timer that is created
 *
 */

/**
 * Determines the time since the device was powered on.
 *
 * @return the current time since power on in milliseconds
 */
CODAL_TIMESTAMP system_timer_current_time();

/**
 * Determines the time since the device was powered on.
 *
 * @return the current time since power on in microseconds
 */
CODAL_TIMESTAMP system_timer_current_time_us();

/**
 * Configure an event to occur every given number of microseconds.
 *
 * @param period the interval between events
 *
 * @param the value to fire against the current system_timer id.
 *
 * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
 *
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_event_every_us(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                                uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

/**
 * Configure an event to occur every given number of milliseconds.
 *
 * @param period the interval between events
 *
 * @param the value to fire against the current system_timer id.
 *
 * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
 *
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_event_every(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                             uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

/**
 * Configure an event to occur after a given number of microseconds.
 *
 * @param period the interval between events
 *
 * @param the value to fire against the current system_timer id.
 *
 * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
 *
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_event_after(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                             uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

/**
 * Configure an event to occur after a given number of milliseconds.
 *
 * @param period the interval between events
 *
 * @param the value to fire against the current system_timer id.
 *
 * @param flags CODAL_TIMER_EVENT_FLAGS_WAKEUP for event to trigger deep sleep wake-up.
 *
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_event_after_us(CODAL_TIMESTAMP period, uint16_t id, uint16_t value,
                                uint32_t flags = CODAL_TIMER_EVENT_FLAGS_NONE);

/**
 * Cancels any events matching the given id and value.
 *
 * @param id the ID that was given upon a previous call to eventEvery / eventAfter
 *
 * @param value the value that was given upon a previous call to eventEvery / eventAfter
 */
int system_timer_cancel_event(uint16_t id, uint16_t value);

/**
 * An auto calibration method that uses the hardware timer to compute the number of cycles
 * per us.
 *
 * The result of this method is then used to compute accurate us waits using instruction counting in
 * system_timer_wait_us.
 *
 * If this method is not called, a less accurate timer implementation is used in system_timer_wait_us.
 *
 * @return DEVICE_OK on success, and DEVICE_NOT_SUPPORTED if no system_timer yet exists.
 */
int system_timer_calibrate_cycles();

/**
 * Spin wait for a given number of cycles.
 *
 * @param cycles the number of nops to execute
 * @return DEVICE_OK
 *
 * @note the amount of cycles per iteration will vary between CPUs.
 */
FORCE_RAM_FUNC
void system_timer_wait_cycles(uint32_t cycles);

/**
 * Spin wait for a given number of microseconds.
 *
 * @param period the interval between events
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 *
 * @note if system_timer_calibrate_cycles is not called, a hardware timer implementation will be used instead
 * which is far less accurate than instruction counting.
 */
int system_timer_wait_us(uint32_t period);

/**
 * Spin wait for a given number of milliseconds.
 *
 * @param period the interval between events
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_wait_ms(uint32_t period);

/**
 * Determine the current time and the corresponding timer counter,
 * to enable the caller to take over tracking time.
 *
 * @param counter reference to a variable to receive the current timer counter
 *
 * @return the current time since power on in microseconds
 */
CODAL_TIMESTAMP system_timer_deepsleep_begin(CODAL_TIMESTAMP& counter);

/**
 * After taking over time tracking with system_timer_deepsleep_begin,
 * hand back control by supplying a new timer counter value with
 * corresponding elapsed time since taking over tracking.
 *
 * The counter and elapsed time may be zero if the time has been maintained
 * meanwhile by calling system_timer_current_time_us().
 *
 * Event timestamps are are shifted towards the present.
 * "after" and "every" events that would have fired during deep sleep
 * will fire once as if firing late, then "every" events will
 * resume the same relative timings.
 *
 * @param counter the current timer counter.
 * @param micros time elapsed since system_timer_deepsleep_begin
 *
 * @return DEVICE_OK or DEVICE_NOT_SUPPORTED if no timer has been registered.
 */
int system_timer_deepsleep_end(CODAL_TIMESTAMP counter, CODAL_TIMESTAMP micros);

/**
 * Determine the time of the next wake-up event.
 * @param timestamp reference to a variable to receive the time.
 * @return true if there is an event.
 */
bool system_timer_deepsleep_wakeup_time(CODAL_TIMESTAMP& timestamp);

extern Timer* system_timer;
}  // namespace codal

#endif
