#include <rabbitmq.hpp>
#include <postgres.hpp>

#include <cstring>
#include <iostream>
#include <optional>

using namespace tick_server;

void handle_message(amqp_connection_state_t connection, PGconn* postgres, const amqp_envelope_t& envelope) {
    const std::string body_str = mq::bytes_to_str(envelope.message.body);
    
    const auto& properties = envelope.message.properties;

    const std::string reply_to = (properties._flags & AMQP_BASIC_REPLY_TO_FLAG) != 0
        ? mq::bytes_to_str(properties.reply_to)
        : "";

    const std::string correlation_id = (properties._flags & AMQP_BASIC_CORRELATION_ID_FLAG) != 0
        ? mq::bytes_to_str(properties.correlation_id)
        : "";
    std::string response_request_id = correlation_id;

    json response_body{};

    try {
        json body = json::parse(body_str);

        std::optional<std::string> validation_error;
        std::string request_id = mq::get_field(body, "request_id", validation_error);
        if (validation_error.has_value()) {
            response_body = mq::error_response(response_request_id, *validation_error);
        }
        else {
            response_request_id = request_id;

            std::string symbol = mq::get_field(body, "symbol", validation_error);
            if (!validation_error.has_value()) {
                std::string start_time = mq::get_field(body, "start_time", validation_error);
                if (!validation_error.has_value()) {
                    if (!utilities::is_full_timestamp(start_time)) {
                        validation_error = "start_time must be full timestamp with timezone";
                    }
                }
                if (!validation_error.has_value()) {
                    std::string end_time = mq::get_field(body, "end_time", validation_error);
                    if (!validation_error.has_value()) {
                        if (!utilities::is_full_timestamp(end_time)) {
                            validation_error = "end_time must be full timestamp with timezone";
                        }
                    }
                    if (!validation_error.has_value()) {
                        json records = pg::query(postgres, symbol, start_time, end_time);
                        response_body = json{
                            {"request_id", request_id},
                            {"status", "ok"},
                            {"symbol", symbol},
                            {"count",  records.size()},
                            {"records", records}
                        };
                    }
                }
            }

            if (validation_error.has_value()) {
                response_body = mq::error_response(response_request_id, *validation_error);
            }
        }
    } catch (const json::exception& ex) {
        response_body = mq::error_response(response_request_id, std::string("Invalid JSON: ") + ex.what());
    } catch (const std::exception& ex) {
        response_body = mq::error_response(response_request_id, ex.what());
    }

    if (!reply_to.empty()) {
        std::string response_correlation_id = !correlation_id.empty() ? correlation_id : response_request_id;
        mq::publish(connection, reply_to, response_correlation_id, response_body);
    } else {
        std::cerr << "Skipping response: reply_to was not set" << std::endl;
    }

    mq::acknowledge(connection, envelope.delivery_tag);
}

int main() {
    try {
        auto* postgres = pg::connect();
        auto  rabbitmq = mq::connect();

        std::cout << "tick_server listening on " << mq::k_queue_name << std::endl;

        while(true) {
            amqp_envelope_t envelope;
            std::memset(&envelope, 0, sizeof(envelope));

            auto consumed_reply = mq::consume(rabbitmq, &envelope);
            if (consumed_reply.reply_type != AMQP_RESPONSE_NORMAL) {
                throw std::runtime_error("Failed to consume message: " + mq::describe_rpc_reply(consumed_reply));
            }

            try {
                handle_message(rabbitmq, postgres, envelope);
            } catch (...) {
                mq::cleanup_consumed_message(&envelope);
                throw;
            }

            mq::cleanup_consumed_message(&envelope, rabbitmq);
        }
    }
    catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        return 1;
    }

    return 0;
}
