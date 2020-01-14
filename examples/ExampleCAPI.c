/*
    This example illustrates usage of the plain C API.

    It also serves to verify that no C++ snuck into the API.

    The example lists all available sensors to stdout.
*/

#include <stdio.h>
#include <stdlib.h>

#include "OpenZenCAPI.h"

typedef struct SensorList {
    ZenSensorDesc desc;
    struct SensorList* next;
} SensorList;

int main(int argc, char **argv)
{
    ZenError_t zenError = 0;
    ZenClientHandle_t clientHandle = { 0 };
    zenError = ZenInit(&clientHandle);
    if (zenError != ZenError_None) {
        printf("ZenError %d when obtaining client.\n", zenError);
        return -1;
    }

    zenError = ZenListSensorsAsync(clientHandle);
    if (zenError != ZenError_None) {
        printf("ZenError %d when starting sensor search.\n", zenError);
        return -1;
    }

    printf("Listing sensors ...");
    fflush(stdout);
    ZenEvent zenEvent;
    SensorList* sensorList = NULL;
    do {
        ZenWaitForNextEvent(clientHandle, &zenEvent);
        if (zenEvent.eventType == ZenSensorEvent_SensorFound) {
            SensorList* s = (SensorList*)malloc(sizeof(SensorList));
            s->desc = zenEvent.data.sensorFound;
            s->next = sensorList;
            sensorList = s;
        }
    } while (zenEvent.eventType != ZenSensorEvent_SensorListingProgress
        || !zenEvent.data.sensorListingProgress.complete);
    puts(" done.");

    SensorList* p = sensorList;
    puts("List of available sensors:");
    while (p) {
        printf("%s\n", p->desc.name);
        p = p->next;
    }

    // Free sensorList.
    p = sensorList;
    while (p) {
        SensorList* prev = p;
        p = p->next;
        free(prev);
    }

    ZenShutdown(clientHandle);
}
