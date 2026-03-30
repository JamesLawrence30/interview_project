#pragma once

#include <nlohmann/json.hpp>

#include <cstdlib>
#include <string>
#include <regex>

namespace tick_server {
    using json = nlohmann::json;

    namespace utilities {
        inline std::string get_env_or_default(const char* env, const char* fallback) {
            const char* value = std::getenv(env);
            if (value == nullptr || *value == '\0') {
                return fallback;
            }
            return value;
        }

        inline bool is_full_timestamp(const std::string& value) {
            const std::regex k_timestamp_pattern(R"(^\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}(?:\.\d+)?(?:Z|[+-]\d{2}:\d{2})$)");
            return std::regex_match(value, k_timestamp_pattern);
        }
    };
};
