#pragma once

#include "console/services/IInfrastructureService.hpp"
#include "console/storage/DatabaseManager.hpp"

#include <memory>

namespace console::services {

/**
 * @brief Implementation of infrastructure management service
 *
 * Manages servers, drives, and storage pools according to
 * object storage principles (erasure coding, distributed architecture)
 */
class InfrastructureService : public IInfrastructureService {
  public:
    explicit InfrastructureService(std::shared_ptr<storage::DatabaseManager> db_manager);

    // Summary
    Result<InfrastructureSummary, models::ApiError> get_summary(const UserInfo& user) override;

    // Server operations
    Result<Vector<ServerDetails>, models::ApiError> list_servers(const UserInfo& user) override;
    Result<ServerDetails, models::ApiError> get_server(const UserInfo& user, const String& id) override;
    Result<ServerDetails, models::ApiError> add_server(const UserInfo& user, const AddServerRequest& request) override;
    Result<void, models::ApiError> remove_server(const UserInfo& user, const String& id) override;
    Result<ServerDetails, models::ApiError> check_server_health(const UserInfo& user, const String& id) override;

    // Drive operations
    Result<Vector<DriveDetails>, models::ApiError> list_drives(const UserInfo& user,
                                                               const String& server_id = "") override;
    Result<DriveDetails, models::ApiError> get_drive(const UserInfo& user, const String& id) override;
    Result<DriveDetails, models::ApiError> add_drive(const UserInfo& user, const AddDriveRequest& request) override;
    Result<void, models::ApiError> remove_drive(const UserInfo& user, const String& id) override;
    Result<void, models::ApiError> set_drive_status(const UserInfo& user,
                                                    const String& id,
                                                    const String& status) override;

    // Pool operations
    Result<Vector<PoolDetails>, models::ApiError> list_pools(const UserInfo& user) override;
    Result<PoolDetails, models::ApiError> get_pool(const UserInfo& user, const String& id) override;
    Result<PoolDetails, models::ApiError> configure_pool(const UserInfo& user,
                                                         const ConfigurePoolRequest& request) override;
    Result<void, models::ApiError> decommission_pool(const UserInfo& user, const String& id) override;

    // Healing operations
    Result<Json::Value, models::ApiError> get_heal_status(const UserInfo& user) override;
    Result<void, models::ApiError> start_heal(const UserInfo& user, const String& pool_id = "") override;

  private:
    std::shared_ptr<storage::DatabaseManager> db_manager_;

    // Helper methods
    ServerDetails db_server_to_details(const storage::DbServer& server);
    DriveDetails db_drive_to_details(const storage::DbDrive& drive);
    PoolDetails db_pool_to_details(const storage::DbStoragePool& pool);

    String generate_server_id();
    String generate_drive_id(const String& server_id);
    String generate_pool_id();

    void recalculate_pool_stats(const String& pool_id);
    void update_server_drive_counts(const String& server_id);

    // Erasure coding calculations
    int calculate_erasure_sets(int drives_count, int set_size);
    int64_t calculate_usable_capacity(int64_t raw_capacity, int data_shards, int parity_shards);
};

} // namespace console::services
