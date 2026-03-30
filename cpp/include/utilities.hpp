#pragma once

#include <nlohmann/json.hpp>

namespace tick_server {
    using json = nlohmann::json;

    namespace utilities {
        std::string get_env_or_default(const char* env, const char* fallback) {
            const char* value = std::getenv(env);
            if (value == nullptr || *value == '\0') {
                return fallback;
            }
            return value;
        }
    };
};
