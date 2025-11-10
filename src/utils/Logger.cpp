#include "console/common/Logger.hpp"

#include <filesystem>

namespace console {

Logger&
Logger::instance() {
    static Logger logger;
    return logger;
}

void
Logger::init(LogLevel log_level, const String& log_path, size_t max_file_size, size_t max_files) {
    if (initialized_) {
        return;
    }

    current_level_ = log_level;

    try {
        // Console logger
        console_logger_ = spdlog::stdout_color_mt("console");
        console_logger_->set_level(to_spdlog_level(log_level));
        console_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

        // File logger (if path provided)
        if (!log_path.empty()) {
            std::filesystem::create_directories(log_path);

            auto file_path = std::filesystem::path(log_path) / "console.log";
            file_logger_ = spdlog::rotating_logger_mt("file", file_path.string(), max_file_size, max_files);
            file_logger_->set_level(to_spdlog_level(log_level));
            file_logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
        }

        initialized_ = true;

    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Logger initialization failed: " << ex.what() << std::endl;
    }
}

void
Logger::set_level(LogLevel level) {
    current_level_ = level;
    auto spdlog_level = to_spdlog_level(level);

    if (console_logger_) {
        console_logger_->set_level(spdlog_level);
    }
    if (file_logger_) {
        file_logger_->set_level(spdlog_level);
    }
}

void
Logger::flush() {
    if (console_logger_) {
        console_logger_->flush();
    }
    if (file_logger_) {
        file_logger_->flush();
    }
}

String
Logger::format_location(const std::source_location& loc) const {
    return std::format("{}:{}:{}", loc.file_name(), loc.line(), loc.column());
}

spdlog::level::level_enum
Logger::to_spdlog_level(LogLevel level) const {
    switch (level) {
        case LogLevel::Trace:
            return spdlog::level::trace;
        case LogLevel::Debug:
            return spdlog::level::debug;
        case LogLevel::Info:
            return spdlog::level::info;
        case LogLevel::Warning:
            return spdlog::level::warn;
        case LogLevel::Error:
            return spdlog::level::err;
        case LogLevel::Critical:
            return spdlog::level::critical;
        case LogLevel::Off:
            return spdlog::level::off;
        default:
            return spdlog::level::info;
    }
}

} // namespace console
