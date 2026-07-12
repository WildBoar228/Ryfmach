#include "bel_lang_engine.hpp"
#include "ryfmach_service.hpp"
#include "server.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

std::string GetHostName() {
    constexpr char kDefaultHostName[] = "0.0.0.0";
    const char* host_name = std::getenv("RYFMACH_HOST_NAME");
    return (host_name ? host_name : kDefaultHostName);
}

int GetPort() {
    constexpr int kDefaultPort = 8080;
    const char* port_str = std::getenv("RYFMACH_APP_PORT");
    if (port_str == nullptr) {
        return kDefaultPort;
    }
    int port;
    try {
        port = std::stoi(port_str);
    } catch (...) {
        return kDefaultPort;
    }
    return port;
}

} // namespace

int main() {
    ryfmach::bel::Slounik slounik;
    ryfmach::app::RyfmachService service(slounik);
    ryfmach::app::RyfmachServer server(service);

    std::string host = GetHostName();
    int port = GetPort();

    std::cout << "Ryfmach is listening on " << host << ":" << port << "\n";
    if (!server.Listen(host, port)) {
        std::cerr << "failed to bind " << host << ":" << port << "\n";
        return 1;
    }

    return 0;
}
