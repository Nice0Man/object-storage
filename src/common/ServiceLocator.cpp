//
#include "console/common/ServiceLocator.hpp"

#include "console/clients/LocalAdminClient.hpp"
#include "console/clients/LocalStorageClient.hpp"
#include "console/services/AuthService.hpp"
#include "console/services/BucketService.hpp"
#include "console/services/ObjectService.hpp"
#include "console/services/UserService.hpp"
#include "console/storage/DatabaseManager.hpp"

namespace console {

// Static member definitions
std::shared_ptr<clients::LocalStorageClient> ServiceLocator::storage_client_;
std::shared_ptr<clients::LocalAdminClient> ServiceLocator::admin_client_;
std::shared_ptr<storage::DatabaseManager> ServiceLocator::database_;
std::shared_ptr<services::ObjectService> ServiceLocator::object_service_;
std::shared_ptr<services::BucketService> ServiceLocator::bucket_service_;
std::shared_ptr<services::UserService> ServiceLocator::user_service_;
std::shared_ptr<services::AuthService> ServiceLocator::auth_service_;

// Getters
std::shared_ptr<clients::LocalStorageClient>
ServiceLocator::storage_client() {
    return storage_client_;
}

std::shared_ptr<clients::LocalAdminClient>
ServiceLocator::admin_client() {
    return admin_client_;
}

std::shared_ptr<storage::DatabaseManager>
ServiceLocator::database() {
    return database_;
}

std::shared_ptr<services::ObjectService>
ServiceLocator::object_service() {
    return object_service_;
}

std::shared_ptr<services::BucketService>
ServiceLocator::bucket_service() {
    return bucket_service_;
}

std::shared_ptr<services::UserService>
ServiceLocator::user_service() {
    return user_service_;
}

std::shared_ptr<services::AuthService>
ServiceLocator::auth_service() {
    return auth_service_;
}

// Setters
void
ServiceLocator::set_storage_client(std::shared_ptr<clients::LocalStorageClient> client) {
    storage_client_ = client;
}

void
ServiceLocator::set_admin_client(std::shared_ptr<clients::LocalAdminClient> client) {
    admin_client_ = client;
}

void
ServiceLocator::set_database(std::shared_ptr<storage::DatabaseManager> db) {
    database_ = db;
}

void
ServiceLocator::set_object_service(std::shared_ptr<services::ObjectService> service) {
    object_service_ = service;
}

void
ServiceLocator::set_bucket_service(std::shared_ptr<services::BucketService> service) {
    bucket_service_ = service;
}

void
ServiceLocator::set_user_service(std::shared_ptr<services::UserService> service) {
    user_service_ = service;
}

void
ServiceLocator::set_auth_service(std::shared_ptr<services::AuthService> service) {
    auth_service_ = service;
}

// Cleanup - clear all services in reverse order of initialization
void
ServiceLocator::clear() {
    // Clear services first
    auth_service_.reset();
    user_service_.reset();
    bucket_service_.reset();
    object_service_.reset();

    // Then clear clients
    admin_client_.reset();
    storage_client_.reset();

    // Finally clear database
    database_.reset();
}

} // namespace console
