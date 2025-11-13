#include "console/clients/LocalStorageClient.hpp"
#include "console/common/Logger.hpp"

#include <filesystem>
#include <iostream>

using namespace console;
using namespace console::clients;

void
print_result(const String& test_name, bool success) {
    std::cout << "[" << (success ? "✓" : "✗") << "] " << test_name << std::endl;
}

int
main() {
    // Initialize logger
    Logger::instance().init(LogLevel::Info, "logs", 10 * 1024 * 1024, 5);

    std::cout << "\n========================================" << std::endl;
    std::cout << "🧪 LocalStorageClient Integration Test" << std::endl;
    std::cout << "========================================\n" << std::endl;

    // Create temporary storage directory
    auto temp_storage = std::filesystem::temp_directory_path() / "test-storage";
    std::filesystem::remove_all(temp_storage); // Clean start

    std::cout << "📁 Storage location: " << temp_storage << "\n" << std::endl;

    // Create client
    auto client = std::make_shared<LocalStorageClient>(temp_storage.string());

    int passed = 0;
    int failed = 0;

    // Test 1: Connection check
    {
        auto result = client->is_connected();
        bool success = result && result.value();
        print_result("Connection check", success);
        success ? passed++ : failed++;
    }

    // Test 2: Create bucket
    {
        auto result = client->create_bucket("test-bucket", "us-east-1");
        bool success = result && result.value();
        print_result("Create bucket 'test-bucket'", success);
        success ? passed++ : failed++;
    }

    // Test 3: Bucket exists
    {
        auto result = client->bucket_exists("test-bucket");
        bool success = result && result.value();
        print_result("Bucket exists check", success);
        success ? passed++ : failed++;
    }

    // Test 4: List buckets
    {
        auto result = client->list_buckets();
        bool success = result && result.value().size() == 1;
        print_result("List buckets (expected 1)", success);
        if (success) {
            std::cout << "    Found bucket: " << result.value()[0].name() << std::endl;
        }
        success ? passed++ : failed++;
    }

    // Test 5: Put object
    {
        ByteArray data = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd', '!'};
        StringMap metadata = {{"author", "test-user"}, {"version", "1.0"}};

        auto result = client->put_object("test-bucket", "documents/hello.txt", data, "text/plain", metadata);

        bool success = static_cast<bool>(result);
        print_result("Put object 'documents/hello.txt'", success);
        if (success) {
            std::cout << "    ETag: " << result.value().etag() << std::endl;
            std::cout << "    Size: " << result.value().size() << " bytes" << std::endl;
        }
        success ? passed++ : failed++;
    }

    // Test 6: Get object
    {
        auto result = client->get_object("test-bucket", "documents/hello.txt");
        bool success = static_cast<bool>(result);
        if (success) {
            String content(result.value().begin(), result.value().end());
            success = (content == "Hello World!");
        }
        print_result("Get object and verify content", success);
        if (success) {
            String content(result.value().begin(), result.value().end());
            std::cout << "    Content: " << content << std::endl;
        }
        success ? passed++ : failed++;
    }

    // Test 7: Stat object
    {
        auto result = client->stat_object("test-bucket", "documents/hello.txt");
        bool success = result && result.value().size() == 12;
        print_result("Stat object (verify size)", success);
        if (success) {
            std::cout << "    Key: " << result.value().key() << std::endl;
            std::cout << "    Size: " << result.value().size() << " bytes" << std::endl;
            std::cout << "    Type: " << result.value().content_type() << std::endl;
        }
        success ? passed++ : failed++;
    }

    // Test 8: Get object metadata
    {
        auto result = client->get_object_metadata("test-bucket", "documents/hello.txt");
        bool success = result && result.value().count("author") > 0;
        print_result("Get object metadata", success);
        if (success) {
            for (const auto& [key, value] : result.value()) {
                std::cout << "    " << key << ": " << value << std::endl;
            }
        }
        success ? passed++ : failed++;
    }

    // Test 9: Set object tags
    {
        StringMap tags = {{"Environment", "test"}, {"Owner", "integration-test"}};
        auto result = client->set_object_tags("test-bucket", "documents/hello.txt", tags);
        bool success = result && result.value();
        print_result("Set object tags", success);
        success ? passed++ : failed++;
    }

    // Test 10: Get object tags
    {
        auto result = client->get_object_tags("test-bucket", "documents/hello.txt");
        bool success = result && result.value().count("Environment") > 0;
        print_result("Get object tags", success);
        if (success) {
            for (const auto& [key, value] : result.value()) {
                std::cout << "    " << key << ": " << value << std::endl;
            }
        }
        success ? passed++ : failed++;
    }

    // Test 11: List objects
    {
        ListObjectsOptions options;
        options.prefix = "documents/";
        options.max_keys = 10;

        auto result = client->list_objects("test-bucket", options);
        bool success = result && result.value().objects.size() == 1;
        print_result("List objects in bucket", success);
        if (success) {
            for (const auto& obj : result.value().objects) {
                std::cout << "    " << obj.key() << " (" << obj.size() << " bytes)" << std::endl;
            }
        }
        success ? passed++ : failed++;
    }

    // Test 12: Copy object
    {
        auto result = client->copy_object("test-bucket", "documents/hello.txt", "test-bucket", "backup/hello-copy.txt");
        bool success = result && result.value();
        print_result("Copy object", success);
        success ? passed++ : failed++;
    }

    // Test 13: Verify copied object
    {
        auto result = client->get_object("test-bucket", "backup/hello-copy.txt");
        bool success = static_cast<bool>(result);
        if (success) {
            String content(result.value().begin(), result.value().end());
            success = (content == "Hello World!");
        }
        print_result("Verify copied object content", success);
        success ? passed++ : failed++;
    }

    // Test 14: Generate presigned URL
    {
        auto result = client->generate_presigned_url("test-bucket", "documents/hello.txt", 3600, "GET");
        bool success = static_cast<bool>(result);
        print_result("Generate presigned URL", success);
        if (success) {
            std::cout << "    URL: " << result.value() << std::endl;
        }
        success ? passed++ : failed++;
    }

    // Test 15: Delete object
    {
        auto result = client->delete_object("test-bucket", "documents/hello.txt");
        bool success = result && result.value();
        print_result("Delete object", success);
        success ? passed++ : failed++;
    }

    // Test 16: Verify object deleted
    {
        auto result = client->stat_object("test-bucket", "documents/hello.txt");
        bool success = !static_cast<bool>(result); // Should fail
        print_result("Verify object is deleted", success);
        success ? passed++ : failed++;
    }

    // Test 17: Delete remaining object
    {
        auto result = client->delete_object("test-bucket", "backup/hello-copy.txt");
        bool success = result && result.value();
        print_result("Delete copied object", success);
        success ? passed++ : failed++;
    }

    // Test 18: Delete bucket
    {
        auto result = client->delete_bucket("test-bucket");
        bool success = result && result.value();
        print_result("Delete bucket", success);
        success ? passed++ : failed++;
    }

    // Test 19: Verify bucket deleted
    {
        auto result = client->bucket_exists("test-bucket");
        bool success = result && !result.value(); // Should be false
        print_result("Verify bucket is deleted", success);
        success ? passed++ : failed++;
    }

    // Cleanup
    std::filesystem::remove_all(temp_storage);

    // Summary
    std::cout << "\n========================================" << std::endl;
    std::cout << "📊 Test Results" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Total:  " << (passed + failed) << std::endl;
    std::cout << "Passed: " << passed << std::endl;
    std::cout << "Failed: " << failed << std::endl;
    std::cout << "========================================\n" << std::endl;

    if (failed == 0) {
        std::cout << "✅ All tests passed!" << std::endl;
        return 0;
    } else {
        std::cout << "❌ Some tests failed" << std::endl;
        return 1;
    }
}
