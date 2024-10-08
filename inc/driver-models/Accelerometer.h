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

#ifndef CODAL_ACCELEROMETER_H
#define CODAL_ACCELEROMETER_H

#include "CodalComponent.h"
#include "CodalConfig.h"
#include "CodalUtil.h"
#include "CoordinateSystem.h"
#include "Pin.h"

/**
 * Status flags
 */
#define ACCELEROMETER_IMU_DATA_VALID 0x02

/**
 * Accelerometer events
 */
#define ACCELEROMETER_EVT_DATA_UPDATE 1

/**
 * Gesture events
 */
#define ACCELEROMETER_EVT_NONE       0
#define ACCELEROMETER_EVT_TILT_UP    1
#define ACCELEROMETER_EVT_TILT_DOWN  2
#define ACCELEROMETER_EVT_TILT_LEFT  3
#define ACCELEROMETER_EVT_TILT_RIGHT 4
#define ACCELEROMETER_EVT_FACE_UP    5
#define ACCELEROMETER_EVT_FACE_DOWN  6
#define ACCELEROMETER_EVT_FREEFALL   7
#define ACCELEROMETER_EVT_3G         8
#define ACCELEROMETER_EVT_6G         9
#define ACCELEROMETER_EVT_8G         10
#define ACCELEROMETER_EVT_SHAKE      11
#define ACCELEROMETER_EVT_2G         12

/**
 * Gesture recogniser constants
 */
#define ACCELEROMETER_REST_TOLERANCE     200
#define ACCELEROMETER_TILT_TOLERANCE     200
#define ACCELEROMETER_FREEFALL_TOLERANCE 400
#define ACCELEROMETER_SHAKE_TOLERANCE    400
#define ACCELEROMETER_2G_TOLERANCE       2'048
#define ACCELEROMETER_3G_TOLERANCE       3'072
#define ACCELEROMETER_6G_TOLERANCE       6'144
#define ACCELEROMETER_8G_TOLERANCE       8'192
#define ACCELEROMETER_GESTURE_DAMPING    5
#define ACCELEROMETER_SHAKE_DAMPING      10
#define ACCELEROMETER_SHAKE_RTX          30

#define ACCELEROMETER_REST_THRESHOLD     (ACCELEROMETER_REST_TOLERANCE * ACCELEROMETER_REST_TOLERANCE)
#define ACCELEROMETER_FREEFALL_THRESHOLD \
    ((uint32_t)ACCELEROMETER_FREEFALL_TOLERANCE * (uint32_t)ACCELEROMETER_FREEFALL_TOLERANCE)
#define ACCELEROMETER_2G_THRESHOLD          ((uint32_t)ACCELEROMETER_2G_TOLERANCE * (uint32_t)ACCELEROMETER_2G_TOLERANCE)
#define ACCELEROMETER_3G_THRESHOLD          ((uint32_t)ACCELEROMETER_3G_TOLERANCE * (uint32_t)ACCELEROMETER_3G_TOLERANCE)
#define ACCELEROMETER_6G_THRESHOLD          ((uint32_t)ACCELEROMETER_6G_TOLERANCE * (uint32_t)ACCELEROMETER_6G_TOLERANCE)
#define ACCELEROMETER_8G_THRESHOLD          ((uint32_t)ACCELEROMETER_8G_TOLERANCE * (uint32_t)ACCELEROMETER_8G_TOLERANCE)
#define ACCELEROMETER_SHAKE_COUNT_THRESHOLD 4

namespace codal {
struct ShakeHistory {
    uint16_t shaken : 1, x : 1, y : 1, z : 1, impulse_2, impulse_3, impulse_6, impulse_8, count : 8;

    uint16_t timer;
};

/**
 * Class definition for Accelerometer.
 */
class Accelerometer : public CodalComponent {
  protected:
    uint16_t samplePeriod;  // The time between samples, in milliseconds.
    uint8_t sampleRange;    // The sample range of the accelerometer in g.
    Sample3D sample;        // The last sample read, in the coordinate system specified by the coordinateSpace variable.
    Sample3D sampleENU;  // The last sample read, in raw ENU format (stored in case requests are made for data in other
                         // coordinate spaces)
    CoordinateSpace&
        coordinateSpace;  // The coordinate space transform (if any) to apply to the raw data from the hardware.

    float pitch;  // Pitch of the device, in radians.
    float roll;   // Roll of the device, in radians.

    uint8_t sigma;            // the number of ticks that the instantaneous gesture has been stable.
    uint8_t impulseSigma;     // the number of ticks since an impulse event has been generated.
    uint16_t lastGesture;     // the last, stable gesture recorded.
    uint16_t currentGesture;  // the instantaneous, unfiltered gesture detected.
    ShakeHistory shake;       // State information needed to detect shake events.

  public:
    /**
     * Constructor.
     * Create a software abstraction of an accelerometer.
     *
     * @param coordinateSpace the orientation of the sensor. Defaults to: SIMPLE_CARTESIAN
     * @param id the unique EventModel id of this component. Defaults to: DEVICE_ID_ACCELEROMETER
     *
     */
    Accelerometer(CoordinateSpace& coordinateSpace, uint16_t id = DEVICE_ID_ACCELEROMETER);

    /**
     * Attempts to set the sample rate of the accelerometer to the specified value (in ms).
     *
     * @param period the requested time between samples, in milliseconds.
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR is the request fails.
     *
     * @note The requested rate may not be possible on the hardware. In this case, the
     * nearest lower rate is chosen.
     *
     * @note This method should be overriden (if supported) by specific accelerometer device drivers.
     */
    virtual int setPeriod(int period);

    /**
     * Reads the currently configured sample rate of the accelerometer.
     *
     * @return The time between samples, in milliseconds.
     */
    virtual int getPeriod();

    /**
     * Attempts to set the sample range of the accelerometer to the specified value (in g).
     *
     * @param range The requested sample range of samples, in g.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR is the request fails.
     *
     * @note The requested range may not be possible on the hardware. In this case, the
     * nearest lower range is chosen.
     *
     * @note This method should be overriden (if supported) by specific accelerometer device drivers.
     */
    virtual int setRange(int range);

    /**
     * Reads the currently configured sample range of the accelerometer.
     *
     * @return The sample range, in g.
     */
    virtual int getRange();

    /**
     * Configures the accelerometer for G range and sample rate defined
     * in this object. The nearest values are chosen to those defined
     * that are supported by the hardware. The instance variables are then
     * updated to reflect reality.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR if the accelerometer could not be configured.
     *
     * @note This method should be overridden by the hardware driver to implement the requested
     * changes in hardware.
     */
    virtual int configure() = 0;

    /**
     * Poll to see if new data is available from the hardware. If so, update it.
     * n.b. it is not necessary to explicitly call this function to update data
     * (it normally happens in the background when the scheduler is idle), but a check is performed
     * if the user explicitly requests up to date data.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR if the update fails.
     *
     * @note This method should be overridden by the hardware driver to implement the requested
     * changes in hardware.
     */
    virtual int requestUpdate() = 0;

    /**
     * Stores data from the accelerometer sensor in our buffer, and perform gesture tracking.
     *
     * On first use, this member function will attempt to add this component to the
     * list of fiber components in order to constantly update the values stored
     * by this object.
     *
     * This lazy instantiation means that we do not
     * obtain the overhead from non-chalantly adding this component to fiber components.
     *
     * @return DEVICE_OK on success, DEVICE_I2C_ERROR if the read request fails.
     */
    virtual int update();

    /**
     * Reads the last accelerometer value stored, and provides it in the coordinate system requested.
     *
     * @param coordinateSpace The coordinate system to use.
     * @return The force measured in each axis, in milli-g.
     */
    Sample3D getSample(CoordinateSystem coordinateSystem);

    /**
     * Reads the last accelerometer value stored, and in the coordinate system defined in the constructor.
     * @return The force measured in each axis, in milli-g.
     */
    Sample3D getSample();

    /**
     * reads the value of the x axis from the latest update retrieved from the accelerometer,
     * using the default coordinate system as specified in the constructor.
     *
     * @return the force measured in the x axis, in milli-g.
     */
    int getX();

    /**
     * reads the value of the y axis from the latest update retrieved from the accelerometer,
     * using the default coordinate system as specified in the constructor.
     *
     * @return the force measured in the y axis, in milli-g.
     */
    int getY();

    /**
     * reads the value of the z axis from the latest update retrieved from the accelerometer,
     * using the default coordinate system as specified in the constructor.
     *
     * @return the force measured in the z axis, in milli-g.
     */
    int getZ();

    /**
     * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
     *
     * @return The pitch of the device, in degrees.
     *
     * @code
     * accelerometer.getPitch();
     * @endcode
     */
    int getPitch();

    /**
     * Provides a rotation compensated pitch of the device, based on the latest update retrieved from the accelerometer.
     *
     * @return The pitch of the device, in radians.
     *
     * @code
     * accelerometer.getPitchRadians();
     * @endcode
     */
    float getPitchRadians();

    /**
     * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
     *
     * @return The roll of the device, in degrees.
     *
     * @code
     * accelerometer.getRoll();
     * @endcode
     */
    int getRoll();

    /**
     * Provides a rotation compensated roll of the device, based on the latest update retrieved from the accelerometer.
     *
     * @return The roll of the device, in radians.
     *
     * @code
     * accelerometer.getRollRadians();
     * @endcode
     */
    float getRollRadians();

    /**
     * Retrieves the last recorded gesture.
     *
     * @return The last gesture that was detected.
     *
     * Example:
     * @code
     *
     * if (accelerometer.getGesture() == SHAKE)
     *     display.scroll("SHAKE!");
     * @endcode
     */
    uint16_t getGesture();

    /**
     * Destructor.
     */
    ~Accelerometer();

  private:
    /**
     * Recalculate roll and pitch values for the current sample.
     *
     * @note We only do this at most once per sample, as the necessary trigonemteric functions are rather
     *       heavyweight for a CPU without a floating point unit.
     */
    void recalculatePitchRoll();

    /**
     * Updates the basic gesture recognizer. This performs instantaneous pose recognition, and also some low pass
     * filtering to promote stability.
     */
    void updateGesture();

    /**
     * A service function.
     * It calculates the current scalar acceleration of the device (x^2 + y^2 + z^2).
     * It does not, however, square root the result, as this is a relatively high cost operation.
     *
     * This is left to application code should it be needed.
     *
     * @return the sum of the square of the acceleration of the device across all axes.
     */
    uint32_t instantaneousAccelerationSquared();

    /**
     * Service function.
     * Determines a 'best guess' posture of the device based on instantaneous data.
     *
     * This makes no use of historic data, and forms this input to the filter implemented in updateGesture().
     *
     * @return A 'best guess' of the current posture of the device, based on instanataneous data.
     */
    uint16_t instantaneousPosture();
};
}  // namespace codal

#endif
