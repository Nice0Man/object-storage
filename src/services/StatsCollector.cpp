#include "console/services/StatsCollector.hpp"

#include "console/common/Logger.hpp"

#include <chrono>

namespace console::services {

StatsCollector::StatsCollector(std::shared_ptr<storage::DatabaseManager> database) : database_(std::move(database)) {
    CONSOLE_LOG_DEBUG("StatsCollector created");
}

StatsCollector::~StatsCollector() {
    stop();
    CONSOLE_LOG_DEBUG("StatsCollector destroyed");
}

void
StatsCollector::start() {
    if (running_.exchange(true)) {
        CONSOLE_LOG_WARN("StatsCollector already running");
        return;
    }

    CONSOLE_LOG_INFO("Starting StatsCollector background tasks");

    // Initialize timestamps
    auto now = std::time(nullptr);
    last_server_update_ = now;
    last_drive_update_ = now;
    last_pool_update_ = now;
    last_cleanup_ = now;

    // Start worker thread
    worker_thread_ = std::thread(&StatsCollector::worker_loop, this);

    CONSOLE_LOG_INFO("StatsCollector started successfully");
}

void
StatsCollector::stop() {
    if (!running_.exchange(false)) {
        return;
    }

    CONSOLE_LOG_INFO("Stopping StatsCollector...");

    if (worker_thread_.joinable()) {
        worker_thread_.join();
    }

    CONSOLE_LOG_INFO("StatsCollector stopped");
}

bool
StatsCollector::is_running() const {
    return running_;
}

void
StatsCollector::worker_loop() {
    CONSOLE_LOG_DEBUG("StatsCollector worker thread started");

    while (running_) {
        auto now = std::time(nullptr);

        // Check if it's time for server update
        if (now - last_server_update_ >= SERVER_UPDATE_INTERVAL) {
            collect_server_stats();
            last_server_update_ = now;
        }

        // Check if it's time for drive update
        if (now - last_drive_update_ >= DRIVE_UPDATE_INTERVAL) {
            collect_drive_stats();
            last_drive_update_ = now;
        }

        // Check if it's time for pool aggregation
        if (now - last_pool_update_ >= POOL_UPDATE_INTERVAL) {
            aggregate_pool_stats();
            last_pool_update_ = now;
        }

        // Check if it's time for cleanup
        if (now - last_cleanup_ >= CLEANUP_INTERVAL) {
            cleanup_old_data();
            last_cleanup_ = now;
        }

        // Sleep for 1 second between checks
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    CONSOLE_LOG_DEBUG("StatsCollector worker thread stopped");
}

void
StatsCollector::collect_server_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Collecting server statistics...");

    // Get existing servers and update heartbeat
    auto servers_result = database_->list_servers();
    if (!servers_result) {
        CONSOLE_LOG_WARN("Failed to get servers for heartbeat update");
        return;
    }

    auto now = std::time(nullptr);
    for (auto& server : servers_result.value()) {
        // Update last heartbeat
        server.last_heartbeat = now;

        // In real implementation, we would ping the server here
        // and update status based on response

        database_->upsert_server(server);
    }

    CONSOLE_LOG_DEBUG("Server statistics updated for {} servers", servers_result.value().size());
}

void
StatsCollector::collect_drive_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Collecting drive statistics...");

    // Get existing drives and update check time
    auto drives_result = database_->list_drives();
    if (!drives_result) {
        CONSOLE_LOG_WARN("Failed to get drives for status update");
        return;
    }

    auto now = std::time(nullptr);
    for (auto& drive : drives_result.value()) {
        // Update last check time
        drive.last_check = now;

        // In real implementation, we would check drive status here
        // via admin API and update capacity/used/available

        database_->upsert_drive(drive);
    }

    CONSOLE_LOG_DEBUG("Drive statistics updated for {} drives", drives_result.value().size());
}

void
StatsCollector::aggregate_pool_stats() {
    if (!database_)
        return;

    CONSOLE_LOG_DEBUG("Aggregating storage pool statistics...");

    // Get all drives
    auto drives_result = database_->list_drives();
    if (!drives_result) {
        CONSOLE_LOG_WARN("Failed to get drives for pool aggregation");
        return;
    }

    // Group drives by server (simple pool logic - one pool per server)
    std::map<String, storage::DbStoragePool> pools;

    for (const auto& drive : drives_result.value()) {
        String pool_id = "pool-" + drive.server_id;

        auto& pool = pools[pool_id];
        if (pool.id.empty()) {
            pool.id = pool_id;
            pool.name = "Pool " + drive.server_id;
        }

        pool.capacity += drive.capacity;
        pool.used += drive.used;
        pool.available += drive.available;
        pool.drives_count++;

        if (drive.status == "online") {
            pool.online_drives++;
        } else {
            pool.offline_drives++;
        }
    }

    // Save aggregated pools
    auto now = std::time(nullptr);
    for (auto& [id, pool] : pools) {
        pool.last_update = now;
        database_->upsert_storage_pool(pool);
    }

    CONSOLE_LOG_DEBUG("Pool statistics aggregated for {} pools", pools.size());
}

void
StatsCollector::cleanup_old_data() {
    if (!database_)
        return;

    CONSOLE_LOG_INFO("Running cleanup of old statistics data...");

    auto now = std::time(nullptr);
    auto week_ago = now - (7 * 24 * 3600);

    auto api_cleanup = database_->cleanup_old_api_stats(week_ago);
    if (!api_cleanup) {
        CONSOLE_LOG_WARN("Failed to cleanup old API stats: {}", api_cleanup.error());
    }

    auto throughput_cleanup = database_->cleanup_old_throughput_stats(week_ago);
    if (!throughput_cleanup) {
        CONSOLE_LOG_WARN("Failed to cleanup old throughput stats: {}", throughput_cleanup.error());
    }

    CONSOLE_LOG_INFO("Cleanup of old statistics completed");
}

void
StatsCollector::update_server_stats_now() {
    collect_server_stats();
}

void
StatsCollector::update_drive_stats_now() {
    collect_drive_stats();
}

void
StatsCollector::update_pool_stats_now() {
    aggregate_pool_stats();
}

void
StatsCollector::cleanup_old_stats_now(int days_to_keep) {
    if (!database_)
        return;

    auto now = std::time(nullptr);
    auto cutoff = now - (days_to_keep * 24 * 3600);

    database_->cleanup_old_api_stats(cutoff);
    database_->cleanup_old_throughput_stats(cutoff);
}

void
StatsCollector::initialize_sample_data() {
    if (!database_)
        return;

    CONSOLE_LOG_INFO("Initializing sample statistics data...");

    auto now = std::time(nullptr);

    // Create sample servers
    for (int i = 1; i <= 4; i++) {
        storage::DbServer server;
        server.id = "server-" + std::to_string(i);
        server.name = "Storage Server " + std::to_string(i);
        server.endpoint = "http://server" + std::to_string(i) + ":9000";
        server.status = (i <= 3) ? "online" : "offline";    // 3 online, 1 offline
        server.uptime = (i <= 3) ? (now - (86400 * i)) : 0; // Uptime in seconds
        server.last_heartbeat = now;

        database_->upsert_server(server);
    }

    // Create sample drives (4 drives per server)
    int64_t drive_capacity = 1024LL * 1024 * 1024 * 1024; // 1 TiB per drive

    for (int s = 1; s <= 4; s++) {
        for (int d = 1; d <= 4; d++) {
            storage::DbDrive drive;
            drive.id = "server-" + std::to_string(s) + "-drive-" + std::to_string(d);
            drive.server_id = "server-" + std::to_string(s);
            drive.path = "/mnt/disk" + std::to_string(d);
            drive.status = (s <= 3) ? "online" : "offline";
            drive.capacity = drive_capacity;
            drive.used = static_cast<int64_t>(drive_capacity * 0.3 * d / 4.0); // 30% used on average
            drive.available = drive.capacity - drive.used;
            drive.last_check = now;

            database_->upsert_drive(drive);
        }
    }

    // Aggregate pools
    aggregate_pool_stats();

    CONSOLE_LOG_INFO("Sample statistics data initialized: 4 servers, 16 drives");
}

} // namespace console::services
