#pragma once

#include <libpq-fe.h>

#include <utilities.hpp>

#include <stdexcept>

namespace tick_server::pg {

constexpr char k_prepared_statement_name[] = "fetch_quotes";
constexpr int  k_num_params = 3;

PGconn* connect() {
    std::string conn_info = "host=" + utilities::get_env_or_default("POSTGRES_HOST", "localhost") +
                            " port=" + utilities::get_env_or_default("POSTGRES_PORT", "5432") +
                            " dbname=" + utilities::get_env_or_default("POSTGRES_DB", "app_db") +
                            " user=" + utilities::get_env_or_default("POSTGRES_USER", "postgres") +
                            " password=" + utilities::get_env_or_default("POSTGRES_PASSWORD", "postgres");
    
    PGconn* connection = PQconnectdb(conn_info.c_str());
    if (PQstatus(connection) != CONNECTION_OK) {
        std::string error = PQerrorMessage(connection);
        PQfinish(connection);
        throw std::runtime_error("Postgres connection failed: " + error);
    }

    const char* prepared_statement = 
        "SELECT "
        "to_char(time AT TIME ZONE 'UTC', 'YYYY-MM-DD\"T\"HH24:MI:SS.MS\"Z\"'), "
        "CAST(ask_price AS text), "
        "CAST(ask_size  AS text), "
        "CAST(bid_price AS text), "
        "CAST(bid_size  AS text) "
        "FROM market_quotes "
        "WHERE symbol_name = $1 "
        "AND time >= CAST($2 AS timestamptz)"
        "AND time <= CAST($3 AS timestamptz)"
        "ORDER BY time";
    
    PGresult* prepare_result = PQprepare(connection, k_prepared_statement_name, prepared_statement, k_num_params, nullptr);
    if (PQresultStatus(prepare_result) != PGRES_COMMAND_OK) {
        std::string error = PQresultErrorMessage(prepare_result);
        PQclear(prepare_result);
        PQfinish(connection);
        throw std::runtime_error("Preparing query failed: " + error);
    }
    PQclear(prepare_result);

    return connection;
}

json query(PGconn* connection, const std::string& symbol, const std::string& start_time, const std::string& end_time) {
    const char* params[] = {symbol.c_str(), start_time.c_str(), end_time.c_str()};

    PGresult* result = PQexecPrepared(connection, k_prepared_statement_name, k_num_params, params, nullptr, nullptr, 0);

    if (PQresultStatus(result) != PGRES_TUPLES_OK) {
        std::string error = PQresultErrorMessage(result);
        PQclear(result);
        throw std::runtime_error("Query failed: " + error);
    }

    json records = json::array();
    int rows = PQntuples(result);
    for (int row = 0; row < rows; row++) {
        records.push_back({
            {"time", PQgetvalue(result, row, 0)},
            {"ask_price", std::stod(PQgetvalue(result, row, 1))},
            {"ask_size",  std::stod(PQgetvalue(result, row, 2))},
            {"bid_price", std::stod(PQgetvalue(result, row, 3))},
            {"bid_size",  std::stod(PQgetvalue(result, row, 4))}
        });
    }

    PQclear(result);
    return records;
}

};
