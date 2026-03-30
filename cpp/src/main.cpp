#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <libpq-fe.h>

#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include <nlohmann/json.hpp>

int main() {
    while(true) {
        // testing the container stays up
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    return 0;
}