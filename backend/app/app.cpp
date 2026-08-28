#include "bel_lang_engine.hpp"
#include "log.hpp"
#include "ryfmach_service.hpp"
#include "server.hpp"

#include <cstdlib>
#include <exception>
#include <iostream>
#include <memory>
#include <string>

namespace {

std::string GetHostName() {
    constexpr char kDefaultHostName[] = "0.0.0.0";
    const char* host_name = std::getenv("RYFMACH_HOST_NAME");
    return (host_name ? host_name : kDefaultHostName);
}

int GetPort() {
    constexpr int kDefaultPort = 8080;
    const char* port_str = std::getenv("RYFMACH_API_PORT");
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
    try {
        (void)ryfmach::bel::DefaultSoundCompatibilityTable();
    } catch (const std::exception& exception) {
        std::cerr << "failed to load sound compatibility data: "
                  << exception.what() << std::endl;
        return 1;
    }

    ryfmach::bel::Slounik slounik;
    ryfmach::app::RyfmachService service(
        slounik, std::make_unique<ryfmach::bel::RhymeLikes>());
    ryfmach::app::RyfmachServer server(service);

    std::string host = GetHostName();
    int port = GetPort();

    ryfmach::app::common_log.info("Ryfmach is listening on {}:{}", host, port);
    if (!server.Listen(host, port)) {
        std::cerr << "failed to bind " << host << ":" << port << std::endl;
        ryfmach::app::common_log.error("failed to bind {}:{}", host, port);
        return 2;
    }

    return 0;
}
