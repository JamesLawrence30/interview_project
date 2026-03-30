#include <rabbitmq.hpp>

namespace tick_server::mq {

// Helpers

std::string bytes_to_str(amqp_bytes_t bytes) {
    return std::string(static_cast<char*>(bytes.bytes), bytes.len);
}

void assert_status(int status, const char* action) {
    if (status != AMQP_STATUS_OK) {
        throw std::runtime_error(std::string(action) + " failed: " + amqp_error_string2(status));
    }
}

std::string describe_rpc_reply(const amqp_rpc_reply_t& reply) {
    switch (reply.reply_type) {
        case AMQP_RESPONSE_NORMAL:
            return "normal";
        case AMQP_RESPONSE_NONE:
            return "missing RPC reply";
        case AMQP_RESPONSE_LIBRARY_EXCEPTION:
            return amqp_error_string2(reply.library_error);
        case AMQP_RESPONSE_SERVER_EXCEPTION:
            if (reply.reply.id == AMQP_CONNECTION_CLOSE_METHOD) {
                const auto* close = static_cast<amqp_connection_close_t*>(reply.reply.decoded);
                return "Server connection error " + std::to_string(close->reply_code) + ": " + bytes_to_str(close->reply_text);
            }
            if (reply.reply.id == AMQP_CHANNEL_CLOSE_METHOD) {
                const auto* close = static_cast<amqp_channel_close_t*>(reply.reply.decoded);
                return "Server channel error " + std::to_string(close->reply_code) + ": " + bytes_to_str(close->reply_text);
            }
            return "Unknown server exception";
    }
    return "Unrecognized RPC reply";
}

void require_rpc_ok(amqp_connection_state_t connection, const char* action) {
    amqp_rpc_reply_t reply = amqp_get_rpc_reply(connection);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        throw std::runtime_error(std::string(action) + " failed: " + describe_rpc_reply(reply));
    }
}

std::string get_field(const json& body, const char* key, std::optional<std::string>& error_message) {
    if (!body.contains(key)) {
        error_message = std::string("Field not found: ") + key;
        return {};
    }
    if (!body.at(key).is_string()) {
        error_message = std::string("Field must be string: ") + key;
        return {};
    }

    return body.at(key).get<std::string>();
}

// Core Messaging

amqp_connection_state_t connect() {
    std::string host = utilities::get_env_or_default("RABBITMQ_HOST", "localhost");
    int port = std::stoi(utilities::get_env_or_default("RABBITMQ_PORT", "5672"));
    std::string user = utilities::get_env_or_default("RABBITMQ_USER", "guest");
    std::string password = utilities::get_env_or_default("RABBITMQ_PASSWORD", "guest");

    amqp_connection_state_t connection = amqp_new_connection();
    amqp_socket_t* socket = amqp_tcp_socket_new(connection);
    if (socket == nullptr) {
        amqp_destroy_connection(connection);
        throw std::runtime_error("Failed to create RabbitMQ TCP socket");
    }

    assert_status(amqp_socket_open(socket, host.c_str(), port), "Opening RabbitMQ socket");

    amqp_rpc_reply_t login_reply = amqp_login(connection,
                                              "/",
                                              0,
                                              131072,
                                              0,
                                              AMQP_SASL_METHOD_PLAIN,
                                              user.c_str(),
                                              password.c_str());
    if (login_reply.reply_type != AMQP_RESPONSE_NORMAL) {
        std::string error = describe_rpc_reply(login_reply);
        amqp_destroy_connection(connection);
        throw std::runtime_error("RabbitMQ login failed: " + error);
    }

    amqp_channel_open(connection, k_channel);
    require_rpc_ok(connection, "Opening RabbitMQ channel");

    amqp_queue_declare(connection,
                       k_channel,
                       amqp_cstring_bytes(k_queue_name),
                       0,
                       0,
                       0,
                       0,
                       amqp_empty_table);
    require_rpc_ok(connection, "Declaring request queue");

    amqp_basic_qos(connection, k_channel, 0, 1, 0);
    require_rpc_ok(connection, "Setting basic qos");

    amqp_basic_consume(connection,
                       k_channel,
                       amqp_cstring_bytes(k_queue_name),
                       amqp_empty_bytes,
                       0,
                       0,
                       0,
                       amqp_empty_table);
    require_rpc_ok(connection, "Starting consumer");

    return connection;
}

void publish(amqp_connection_state_t connection, const std::string& reply_to, const std::string& correlation_id, const json& response_body) {
    std::string payload = response_body.dump();

    amqp_basic_properties_t properties{};
    properties._flags = AMQP_BASIC_CONTENT_TYPE_FLAG;
    properties.content_type = amqp_cstring_bytes("application/json");

    if (!correlation_id.empty()) {
        properties._flags |= AMQP_BASIC_CORRELATION_ID_FLAG;
        properties.correlation_id = amqp_cstring_bytes(correlation_id.c_str());
    }

    assert_status(amqp_basic_publish(connection,
                                     k_channel,
                                     amqp_empty_bytes,
                                     amqp_cstring_bytes(reply_to.c_str()),
                                     0,
                                     0,
                                     &properties,
                                     amqp_cstring_bytes(payload.c_str())),
                                    "Publishing response");
}

void acknowledge(amqp_connection_state_t connection, uint64_t delivery_tag) {
    assert_status(
        amqp_basic_ack(connection, k_channel, delivery_tag, 0),
        "Acking message");
}

json error_response(const std::string& request_id, const std::string& error_message) {
    return json{
        {"request_id", request_id},
        {"status", "error"},
        {"error", error_message}
    };
}

};

