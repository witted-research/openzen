#ifndef ZEN_API_IZENSENSORMANAGER_H_
#define ZEN_API_IZENSENSORMANAGER_H_

#include "ZenTypes.h"

class IZenSensor;

class IZenSensorManager
{
protected:
    /** Protected destructor, to prevent usage on pointer. Instead call ZenShutdown */
    virtual ~IZenSensorManager() = default;

public:
    /** Obtain a sensor */
    virtual ZenError obtain(const ZenSensorDesc* sensorDesc, IZenSensor** outSensor) = 0;

    /** Release a sensor */
    virtual ZenError release(IZenSensor* sensor) = 0;

    /** Returns true and fills the next event on the queue if there is one, otherwise returns false. */
    virtual bool pollNextEvent(ZenEvent* outEvent) = 0;

    /* Returns true and fills the next event on the queue when there is a new one, otherwise returns false upon a call to ZenShutdown() */
    virtual bool waitForNextEvent(ZenEvent* outEvent) = 0;

    /** Lists and returns
     * Returns ZenAsync_Updating while busy listing available sensors.
     * Returns ZenAsync_Finished once available sensors have been successfully listed, otherwise will return an error.
     * Returns ZenAsync_Failed if an error occurred while listing sensors.
     */
    virtual ZenAsyncStatus listSensorsAsync(ZenSensorDesc** outSensors, size_t* outLength, const char* typeFilter) = 0;
};

ZEN_API IZenSensorManager* ZenInit(ZenError* error);
ZEN_API ZenError ZenShutdown();

#endif
