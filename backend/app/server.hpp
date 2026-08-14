#ifndef RYFMACH_APP_SERVER_HPP_
#define RYFMACH_APP_SERVER_HPP_

#include "log.hpp"
#include "ryfmach_service.hpp"

#include <chrono>
#include <memory>
#include <string_view>

namespace ryfmach::app {

class RyfmachServer {
public:
    explicit RyfmachServer(RyfmachService& service);
    ~RyfmachServer();

    bool Listen(std::string_view host, int port) const;

private:
    const size_t max_log_size_ = 5 * 1024 * 1024;
    const size_t max_log_files_ = 5;

    RyfmachService& service_;
    std::chrono::steady_clock bench_clock_;
};

} // namespace ryfmach::app

#endif // RYFMACH_APP_SERVER_HPP_
