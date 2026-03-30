#pragma once

#include <amqp.h>
#include <amqp_tcp_socket.h>

#include <utilities.hpp>

#include <stdexcept>
#include <optional>

namespace tick_server::mq {

constexpr char k_queue_name[] = "market_data_requests";
constexpr int  k_channel = 1;

// Helpers
std::string bytes_to_str(amqp_bytes_t bytes);
void assert_status(int status, const char* action);
std::string describe_rpc_reply(const amqp_rpc_reply_t& reply);
void require_rpc_ok(amqp_connection_state_t connection, const char* action);
std::string get_field(const json& body, const char* key, std::optional<std::string>& error_message);

// Core Messaging
amqp_connection_state_t connect();
void publish(amqp_connection_state_t connection, const std::string& reply_to, const std::string& correlation_id, const json& response_body);
void acknowledge(amqp_connection_state_t connection, uint64_t delivery_tag);
json error_response(const std::string& request_id, const std::string& error_message);

// Wrappers
inline void cleanup_consumed_message(amqp_envelope_t* envelope) {
    amqp_destroy_envelope(envelope);
}
inline void cleanup_consumed_message(amqp_envelope_t* envelope, amqp_connection_state_t rabbitmq) {
    cleanup_consumed_message(envelope);
    amqp_maybe_release_buffers(rabbitmq);
}

inline amqp_rpc_reply_t consume(amqp_connection_state_t& rabbitmq, amqp_envelope_t* envelope) {
    return amqp_consume_message(rabbitmq, envelope, nullptr, 0); // blocking call with nullptr timeout
}

};
