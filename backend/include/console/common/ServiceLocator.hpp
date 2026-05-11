#pragma once

#include "console/common/Types.hpp"

#include <memory>

// Forward declarations
namespace console {
namespace clients {
class LocalStorageClient;
class LocalAdminClient;
} // namespace clients
namespace storage {
class DatabaseManager;
}
namespace services {
class ObjectService;
class BucketService;
class UserService;
class AuthService;
class StatsCollector;
class InfrastructureService;
class EncryptionService;
class LifecycleService;
} // namespace services
} // namespace console

namespace console {

/**
 * @brief Service Locator pattern for accessing global service instances
 *
 * Provides access to initialized service instances throughout the application.
 * Services are initialized in main.cpp during startup.
 */
class ServiceLocator {
  public:
    // Storage clients
    static std::shared_ptr<clients::LocalStorageClient> storage_client();
    static std::shared_ptr<clients::LocalAdminClient> admin_client();
    static std::shared_ptr<storage::DatabaseManager> database();

    // Services
    static std::shared_ptr<services::ObjectService> object_service();
    static std::shared_ptr<services::BucketService> bucket_service();
    static std::shared_ptr<services::UserService> user_service();
    static std::shared_ptr<services::AuthService> auth_service();
    static std::shared_ptr<services::StatsCollector> stats_collector();
    static std::shared_ptr<services::InfrastructureService> infrastructure_service();
    static std::shared_ptr<services::EncryptionService> encryption_service();
    static std::shared_ptr<services::LifecycleService> lifecycle_service();

    // Setters (called from main.cpp during initialization)
    static void set_storage_client(std::shared_ptr<clients::LocalStorageClient> client);
    static void set_admin_client(std::shared_ptr<clients::LocalAdminClient> client);
    static void set_database(std::shared_ptr<storage::DatabaseManager> db);
    static void set_object_service(std::shared_ptr<services::ObjectService> service);
    static void set_bucket_service(std::shared_ptr<services::BucketService> service);
    static void set_user_service(std::shared_ptr<services::UserService> service);
    static void set_auth_service(std::shared_ptr<services::AuthService> service);
    static void set_stats_collector(std::shared_ptr<services::StatsCollector> collector);
    static void set_infrastructure_service(std::shared_ptr<services::InfrastructureService> service);
    static void set_encryption_service(std::shared_ptr<services::EncryptionService> service);
    static void set_lifecycle_service(std::shared_ptr<services::LifecycleService> service);

    // Cleanup (called before program termination to ensure proper destruction order)
    static void clear();

  private:
    static std::shared_ptr<clients::LocalStorageClient> storage_client_;
    static std::shared_ptr<clients::LocalAdminClient> admin_client_;
    static std::shared_ptr<storage::DatabaseManager> database_;
    static std::shared_ptr<services::ObjectService> object_service_;
    static std::shared_ptr<services::BucketService> bucket_service_;
    static std::shared_ptr<services::UserService> user_service_;
    static std::shared_ptr<services::AuthService> auth_service_;
    static std::shared_ptr<services::StatsCollector> stats_collector_;
    static std::shared_ptr<services::InfrastructureService> infrastructure_service_;
    static std::shared_ptr<services::EncryptionService> encryption_service_;
    static std::shared_ptr<services::LifecycleService> lifecycle_service_;
};

} // namespace console
