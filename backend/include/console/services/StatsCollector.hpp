#pragma once

#include "console/common/Types.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <atomic>
#include <functional>
#include <memory>
#include <thread>
#include <vector>

namespace console::services {

/**
 * @brief Background service for collecting and aggregating statistics
 *
 * Manages periodic tasks for:
 * - Server heartbeat monitoring
 * - Drive status updates
 * - Storage pool aggregation
 * - Old data cleanup
 */
class StatsCollector {
  public:
    /**
     * @brief Construct StatsCollector
     *
     * @param database DatabaseManager instance for storing stats
     */
    explicit StatsCollector(std::shared_ptr<storage::DatabaseManager> database);

    ~StatsCollector();

    // Prevent copying
    StatsCollector(const StatsCollector&) = delete;
    StatsCollector& operator=(const StatsCollector&) = delete;

    /**
     * @brief Start background collection tasks
     */
    void start();

    /**
     * @brief Stop background collection tasks
     */
    void stop();

    /**
     * @brief Check if collector is running
     */
    bool is_running() const;

    /**
     * @brief Force immediate update of server stats
     * Useful for testing or manual refresh
     */
    void update_server_stats_now();

    /**
     * @brief Force immediate update of drive stats
     */
    void update_drive_stats_now();

    /**
     * @brief Force immediate update of pool stats
     */
    void update_pool_stats_now();

    /**
     * @brief Force immediate cleanup of old stats
     * @param days_to_keep Number of days of data to retain (default: 7)
     */
    void cleanup_old_stats_now(int days_to_keep = 7);

    /**
     * @brief Initialize with sample data for testing
     * Creates sample servers, drives, and pools in the database
     */
    void initialize_sample_data();

  private:
    /**
     * @brief Worker thread function
     */
    void worker_loop();

    /**
     * @brief Update server statistics
     * Queries admin API for server status and updates database
     */
    void collect_server_stats();

    /**
     * @brief Update drive statistics
     * Queries admin API for drive status and updates database
     */
    void collect_drive_stats();

    /**
     * @brief Aggregate and update storage pool statistics
     * Calculates pool totals from drive data
     */
    void aggregate_pool_stats();

    /**
     * @brief Cleanup old statistics data
     * Removes data older than retention period
     */
    void cleanup_old_data();

    std::shared_ptr<storage::DatabaseManager> database_;
    std::atomic<bool> running_{false};
    std::thread worker_thread_;

    // Timing intervals (seconds)
    static constexpr int SERVER_UPDATE_INTERVAL = 30;
    static constexpr int DRIVE_UPDATE_INTERVAL = 60;
    static constexpr int POOL_UPDATE_INTERVAL = 60;
    static constexpr int CLEANUP_INTERVAL = 86400; // 24 hours

    // Counters for timing
    int64_t last_server_update_{0};
    int64_t last_drive_update_{0};
    int64_t last_pool_update_{0};
    int64_t last_cleanup_{0};
};

} // namespace console::services
