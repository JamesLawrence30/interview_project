# Interview Project

This project loads market data from CSV into PostgreSQL, listens for requests over RabbitMQ in a C++ worker, and returns ticks over the requested time-range to a Python client.

The request flow is:

- Python client publishes a JSON request to `market_data_requests` queue
- C++ service validates the request, runs a prepared PostgreSQL query, and publishes a JSON response
- Client reads the response from a temporary reply queue and prints it

## Demo

Build containers:

```bash
docker compose up -d
```

Install Python dependencies:

```bash
pip install -r python/requirements.txt
```

Load CSV into the database:

```bash
./python/db_loader.py
```

Client request example:

```bash
./python/client.py \
--symbol MILLBURN::::CL.NY.J.2025 \
--start 2025-03-02T17:30:00-05:00 \
--end 2025-03-02T18:30:00-05:00
```

## Timestamps

Request must contain full timestamp with timezone, for example:

```text
2025-03-02T17:30:00-05:00
```


## Development

Build the server:

```bash
docker compose build tick_server
```

## Connection Details

### PostgreSQL

| Property | Value |
|----------|-------|
| Host     | localhost |
| Port     | 5432 |
| Database | app_db |
| User     | postgres |
| Password | postgres |

### RabbitMQ

| Property       | Value |
|----------------|-------|
| Host           | localhost |
| AMQP Port      | 5672 |
| Management UI  | http://localhost:15672 |
| User           | guest |
| Password       | guest |

## References

- RabbitMQ AMQP C docs: <https://alanxz.github.io/rabbitmq-c/docs/0.5.0/>
- PostgreSQL libpq docs: <https://www.postgresql.org/docs/current/libpq.html>
