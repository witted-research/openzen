#ifndef ZEN_SENSORCONFIG_H_
#define ZEN_SENSORCONFIG_H_

#include <cstdint>
#include <string>

struct ComponentConfig
{
    std::string id;
    uint32_t version;
};

struct SensorConfig
{
    uint32_t version;
    std::vector<ComponentConfig> components;
};

#endif
