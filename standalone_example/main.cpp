
#include <iostream>

#include <OpenZen.h>

int main() {
    auto clientPair = zen::make_client();
    auto& clientError = clientPair.first;
    auto& client = clientPair.second;
    if (clientError) {
        std::cout << "OpenZen client could not be created" << std::endl;
        return clientError;
    }

    std::cout << "OpenZen client created successfully" << std::endl;
    client.close();
    std::cout << "OpenZen client closed successfully" << std::endl;

    return 0;
}
