# Interview Project

This is a greenfield project to demonstrate your technical skills. We would like you to touch on Rabbit MQ, some SQL, C++, and Python. the file "minutely-bars.csv" has 1-min bars for a couple of markets and you will (1) load it into a database and (2) create a C++ component that listens on a queue and can return ticks based on an input JSON and (3) create a Python component that connects to RabbitMQ to query for data.

# Local development stack with PostgreSQL and RabbitMQ.

## Start Services

```bash
cd interview_project
docker compose up -d
```

## Stop Services

```bash
docker compose down
```

To also delete the stored data (volumes):

```bash
docker compose down -v
```

## Check Status

```bash
docker compose ps
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

Connect via psql:

```bash
docker exec -it app_postgres psql -U postgres -d app_db
```

### RabbitMQ

| Property       | Value |
|----------------|-------|
| Host           | localhost |
| AMQP Port      | 5672 |
| Management UI  | http://localhost:15672 |
| User           | guest |
| Password       | guest |

## Verify

Check that the `market_quotes` table was created:

```bash
docker exec -it app_postgres psql -U postgres -d app_db -c '\dt'
```
