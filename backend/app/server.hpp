#ifndef RYFMACH_APP_SERVER_HPP_
#define RYFMACH_APP_SERVER_HPP_

#include "ryfmach_service.hpp"

#include <string_view>

namespace ryfmach::app {

class RyfmachServer {
public:
    explicit RyfmachServer(const RyfmachService& service);

    bool Listen(std::string_view host, int port) const;

private:
    const RyfmachService& service_;
};

} // namespace ryfmach::app

#endif // RYFMACH_APP_SERVER_HPP_
