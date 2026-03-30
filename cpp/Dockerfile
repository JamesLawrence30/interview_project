FROM gcc:12-bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    cmake \
    libpq-dev \
    librabbitmq-dev \
    nlohmann-json3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY CMakeLists.txt .
COPY include ./include
COPY src ./src

RUN cmake -S . -B build && cmake --build build

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libpq5 \
    librabbitmq4 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/tick_server ./tick_server

CMD ["./tick_server"]