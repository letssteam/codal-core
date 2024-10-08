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

#include "Button.h"

#include "CodalConfig.h"
#include "CodalDmesg.h"
#include "EventModel.h"
#include "Timer.h"

using namespace codal;

/**
 * Constructor.
 *
 * Create a software representation of a button.
 *
 * @param pin the physical pin on the device connected to this button.
 *
 * @param id the ID of the new Button object.
 *
 * @param eventConfiguration Configures the events that will be generated by this Button instance.
 *                           Defaults to DEVICE_BUTTON_ALL_EVENTS.
 *
 * @param mode the configuration of internal pullups/pulldowns, as defined in the mbed PullMode class. PullNone by
 * default.
 *
 */
Button::Button(Pin& pin, uint16_t id, ButtonEventConfiguration eventConfiguration, ButtonPolarity polarity,
               PullMode mode)
    : _pin(pin), pullMode(mode)
{
    this->id                 = id;
    this->eventConfiguration = eventConfiguration;
    this->downStartTime      = 0;
    this->sigma              = 0;
    this->polarity           = polarity;

    // For backward compatibility, always perform demand activation.
    this->isPressed();
}

/**
 * Changes the event configuration used by this button to the given ButtonEventConfiguration.
 *
 * All subsequent events generated by this button will then be informed by this configuraiton.
 *
 * @param config The new configuration for this button. Legal values are DEVICE_BUTTON_ALL_EVENTS or
 * DEVICE_BUTTON_SIMPLE_EVENTS.
 *
 * Example:
 * @code
 * // Configure a button to generate all possible events.
 * buttonA.setEventConfiguration(DEVICE_BUTTON_ALL_EVENTS);
 *
 * // Configure a button to suppress DEVICE_BUTTON_EVT_CLICK and DEVICE_BUTTON_EVT_LONG_CLICK events.
 * buttonA.setEventConfiguration(DEVICE_BUTTON_SIMPLE_EVENTS);
 * @endcode
 */
void Button::setEventConfiguration(ButtonEventConfiguration config)
{
    this->eventConfiguration = config;
}

/**
 * Determines if this button is instantenously active (i.e. pressed).
 * Internal method, use before debouncing.
 */
int Button::buttonActive()
{
    bool active;

    setPinLock(true);
    active = _pin.getDigitalValue() == polarity;
    setPinLock(false);

    return active;
}

/**
 * periodic callback from Device system timer.
 *
 * Check for state change for this button, and fires various events on a state change.
 */
void Button::periodicCallback()
{
    // If this button is disabled, do nothing.
    if (!(status & DEVICE_COMPONENT_RUNNING)) return;

    //
    // If the pin is pulled low (touched), increment our culumative counter.
    // otherwise, decrement it. We're essentially building a lazy follower here.
    // This makes the output debounced for buttons, and desensitizes touch sensors
    // (particularly in environments where there is mains noise!)
    //
    if (buttonActive()) {
        if (sigma < DEVICE_BUTTON_SIGMA_MAX) sigma++;
    }
    else {
        if (sigma > DEVICE_BUTTON_SIGMA_MIN) sigma--;
    }

    // Check to see if we have off->on state change.
    if (sigma > DEVICE_BUTTON_SIGMA_THRESH_HI && !(status & DEVICE_BUTTON_STATE)) {
        // Record we have a state change, and raise an event.
        status |= DEVICE_BUTTON_STATE;
        Event evt(id, DEVICE_BUTTON_EVT_DOWN);
        clickCount++;

        // Record the time the button was pressed.
        downStartTime = system_timer_current_time();
    }

    // Check to see if we have on->off state change.
    if (sigma < DEVICE_BUTTON_SIGMA_THRESH_LO && (status & DEVICE_BUTTON_STATE)) {
        status &= ~DEVICE_BUTTON_STATE;
        status &= ~DEVICE_BUTTON_STATE_HOLD_TRIGGERED;
        Event evt(id, DEVICE_BUTTON_EVT_UP);

        if (eventConfiguration == DEVICE_BUTTON_ALL_EVENTS) {
            // determine if this is a long click or a normal click and send event
            if ((system_timer_current_time() - downStartTime) >= DEVICE_BUTTON_LONG_CLICK_TIME)
                Event evt(id, DEVICE_BUTTON_EVT_LONG_CLICK);
            else
                Event evt(id, DEVICE_BUTTON_EVT_CLICK);
        }
    }

    // if button is pressed and the hold triggered event state is not triggered AND we are greater than the button
    // debounce value
    if ((status & DEVICE_BUTTON_STATE) && !(status & DEVICE_BUTTON_STATE_HOLD_TRIGGERED) &&
        (system_timer_current_time() - downStartTime) >= DEVICE_BUTTON_HOLD_TIME) {
        // set the hold triggered event flag
        status |= DEVICE_BUTTON_STATE_HOLD_TRIGGERED;

        // fire hold event
        Event evt(id, DEVICE_BUTTON_EVT_HOLD);
    }
}

/**
 * Tests if this Button is currently pressed.
 *
 * @code
 * if(buttonA.isPressed())
 *     display.scroll("Pressed!");
 * @endcode
 *
 * @return 1 if this button is pressed, 0 otherwise.
 */
int Button::isPressed()
{
    if (_pin.obj != this) {
        if (_pin.obj != NULL) _pin.obj->releasePin(_pin);

        _pin.obj = this;
        _pin.setPolarity(polarity == ACTIVE_HIGH ? 1 : 0);
        _pin.setPull(pullMode);
        this->status |= DEVICE_COMPONENT_STATUS_SYSTEM_TICK;
    }

    return status & DEVICE_BUTTON_STATE ? 1 : 0;
}

/**
 * Method to release the given pin from a peripheral, if already bound.
 * Device drivers should override this method to disconnect themselves from the give pin
 * to allow it to be used by a different peripheral.
 *
 * @param pin the Pin to be released.
 * @return DEVICE_OK on success, or DEVICE_NOT_IMPLEMENTED if unsupported, or DEVICE_INVALID_PARAMETER if the pin is not
 * bound to this peripheral.
 */
int Button::releasePin(Pin& pin)
{
    // We've been asked to disconnect from the given pin.
    // Stop requesting periodic callbacks from the scheduler.
    this->status &= ~DEVICE_COMPONENT_STATUS_SYSTEM_TICK;
    pin.obj = NULL;

    if (deleteOnRelease) delete this;

    return DEVICE_OK;
}

/**
 * Destructor for Button, where we deregister this instance from the array of fiber components.
 */
Button::~Button() {}

/**
 * Puts the component in (or out of) sleep (low power) mode.
 */
int Button::setSleep(bool doSleep)
{
    if (doSleep) {
        status &= ~DEVICE_BUTTON_STATE;
        status &= ~DEVICE_BUTTON_STATE_HOLD_TRIGGERED;
        clickCount = 0;
        sigma      = 0;
    }
    else {
        if (isWakeOnActive()) {
            if (buttonActive()) {
                sigma = DEVICE_BUTTON_SIGMA_THRESH_LO + 1;
                status |= DEVICE_BUTTON_STATE;
                Event evt(id, DEVICE_BUTTON_EVT_DOWN);
                clickCount    = 1;
                downStartTime = system_timer_current_time();
            }
        }
    }

    return DEVICE_OK;
}
