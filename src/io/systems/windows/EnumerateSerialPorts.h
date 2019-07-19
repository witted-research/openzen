#ifndef ZEN_IO_SYSTEMS_WINDOWS_ENUMERATESERIALPORTS_H_
#define ZEN_IO_SYSTEMS_WINDOWS_ENUMERATESERIALPORTS_H_

#include <string>
#include <vector>

namespace zen {
    bool EnumerateSerialPorts(std::vector<std::string>& ports);
}

#endif
