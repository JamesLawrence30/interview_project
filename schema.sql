CREATE TABLE IF NOT EXISTS market_quotes (
    id BIGSERIAL PRIMARY KEY,
    symbol_name VARCHAR(20) NOT NULL,
    time TIMESTAMPTZ NOT NULL,
    ask_price NUMERIC(18, 8) NOT NULL,
    ask_size NUMERIC(18, 8) NOT NULL,
    bid_price NUMERIC(18, 8) NOT NULL,
    bid_size NUMERIC(18, 8) NOT NULL
);

CREATE INDEX idx_market_quotes_symbol ON market_quotes (symbol_name);
CREATE INDEX idx_market_quotes_time ON market_quotes (time);
