#pragma once

#include <libpq-fe.h>

#include <utilities.hpp>

#include <stdexcept>

namespace tick_server::pg {

constexpr char k_prepared_statement_name[] = "fetch_quotes";
constexpr int  k_num_params = 3;

PGconn* connect();
json query(PGconn* connection, const std::string& symbol, const std::string& start_time, const std::string& end_time);

};
