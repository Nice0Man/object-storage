# ============================================================================
# Runtime image — backend binary and frontend/build are produced on the host
# (see build-backend.sh, build-frontend.sh, build-image.sh).
# No pulls from docker.io/library/node — avoids registry/TLS issues.
# ============================================================================
FROM ubuntu:24.04 AS runtime

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
    libssl3t64 libjsoncpp25 libuuid1 zlib1g \
    libc-ares2 libbrotli1 ca-certificates curl \
    libsnappy1v5 libbz2-1.0 liblz4-1 libzstd1 \
    libpq5 libmariadb3 libsqlite3-0 \
    libspdlog1.12 libfmt9 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -r -s /usr/sbin/nologin -m console

WORKDIR /app

COPY backend/build/bin/console /app/console
COPY frontend/build /app/frontend/build
COPY config/config.example.json /app/config.json

RUN mkdir -p /app/storage /app/data/rocksdb /app/logs \
    && chown -R console:console /app

USER console
EXPOSE 9090

HEALTHCHECK --interval=30s --timeout=5s --start-period=10s --retries=3 \
    CMD curl -sf http://localhost:9090/api/v1/health || exit 1

ENTRYPOINT ["/app/console"]
