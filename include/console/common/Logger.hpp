#pragma once

#include "Types.hpp"

#include <spdlog/sinks/daily_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <format>
#include <source_location>

namespace console {

/**
 * @brief Logging levels
 */
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical,
    Off
};

/**
 * @brief Logger singleton class
 *
 * Provides structured logging functionality with support for
 * console and file outputs, log rotation, and formatting.
 */
class Logger {
  public:
    /**
     * @brief Get logger instance
     */
    static Logger& instance();

    /**
     * @brief Initialize logger with configuration
     * @param log_level Minimum log level
     * @param log_path Path to log files (optional)
     * @param max_file_size Maximum log file size before rotation
     * @param max_files Maximum number of rotated log files
     */
    void init(LogLevel log_level = LogLevel::Info,
              const String& log_path = "",
              size_t max_file_size = 1024 * 1024 * 10, // 10MB
              size_t max_files = 5);

    /**
     * @brief Set log level
     */
    void set_level(LogLevel level);

    /**
     * @brief Get current log level
     */
    LogLevel get_level() const { return current_level_; }

    /**
     * @brief Log trace message
     */
    template <typename... Args>
    void trace(const String& fmt, Args&&... args, const std::source_location& loc = std::source_location::current());

    /**
     * @brief Log debug message
     */
    template <typename... Args>
    void debug(fmt::format_string<Args...> fmt, Args&&... args);

    /**
     * @brief Log info message
     */
    template <typename... Args>
    void info(fmt::format_string<Args...> fmt, Args&&... args);

    /**
     * @brief Log warning message
     */
    template <typename... Args>
    void warn(fmt::format_string<Args...> fmt, Args&&... args);

    /**
     * @brief Log error message
     */
    template <typename... Args>
    void error(fmt::format_string<Args...> fmt, Args&&... args);

    /**
     * @brief Log critical message
     */
    template <typename... Args>
    void critical(const String& fmt, Args&&... args, const std::source_location& loc = std::source_location::current());

    /**
     * @brief Flush all loggers
     */
    void flush();

    // Prevent copying
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

  private:
    Logger() = default;

    String format_location(const std::source_location& loc) const;
    spdlog::level::level_enum to_spdlog_level(LogLevel level) const;

    std::shared_ptr<spdlog::logger> console_logger_;
    std::shared_ptr<spdlog::logger> file_logger_;
    LogLevel current_level_{LogLevel::Info};
    bool initialized_{false};
};

// ============================================================================
// Convenience Macros
// ============================================================================

#define CONSOLE_LOG_TRACE(...)    ::console::Logger::instance().trace(__VA_ARGS__)
#define CONSOLE_LOG_DEBUG(...)    ::console::Logger::instance().debug(__VA_ARGS__)
#define CONSOLE_LOG_INFO(...)     ::console::Logger::instance().info(__VA_ARGS__)
#define CONSOLE_LOG_WARN(...)     ::console::Logger::instance().warn(__VA_ARGS__)
#define CONSOLE_LOG_ERROR(...)    ::console::Logger::instance().error(__VA_ARGS__)
#define CONSOLE_LOG_CRITICAL(...) ::console::Logger::instance().critical(__VA_ARGS__)

// ============================================================================
// Template Implementations
// ============================================================================

template <typename... Args>
void
Logger::trace(const String& fmt, Args&&... args, const std::source_location& loc) {
    if (current_level_ <= LogLevel::Trace && console_logger_) {
        auto msg = std::vformat(fmt, std::make_format_args(args...));
        console_logger_->trace("[{}] {}", format_location(loc), msg);
        if (file_logger_) {
            file_logger_->trace("[{}] {}", format_location(loc), msg);
        }
    }
}

template <typename... Args>
void
Logger::debug(fmt::format_string<Args...> fmt, Args&&... args) {
    if (current_level_ <= LogLevel::Debug && console_logger_) {
        console_logger_->debug(fmt, std::forward<Args>(args)...);
        if (file_logger_) {
            file_logger_->debug(fmt, std::forward<Args>(args)...);
        }
    }
}

template <typename... Args>
void
Logger::info(fmt::format_string<Args...> fmt, Args&&... args) {
    if (current_level_ <= LogLevel::Info && console_logger_) {
        console_logger_->info(fmt, std::forward<Args>(args)...);
        if (file_logger_) {
            file_logger_->info(fmt, std::forward<Args>(args)...);
        }
    }
}

template <typename... Args>
void
Logger::warn(fmt::format_string<Args...> fmt, Args&&... args) {
    if (current_level_ <= LogLevel::Warning && console_logger_) {
        console_logger_->warn(fmt, std::forward<Args>(args)...);
        if (file_logger_) {
            file_logger_->warn(fmt, std::forward<Args>(args)...);
        }
    }
}

template <typename... Args>
void
Logger::error(fmt::format_string<Args...> fmt, Args&&... args) {
    if (current_level_ <= LogLevel::Error && console_logger_) {
        console_logger_->error(fmt, std::forward<Args>(args)...);
        if (file_logger_) {
            file_logger_->error(fmt, std::forward<Args>(args)...);
        }
    }
}

template <typename... Args>
void
Logger::critical(const String& fmt, Args&&... args, const std::source_location& loc) {
    if (console_logger_) {
        auto msg = std::vformat(fmt, std::make_format_args(args...));
        console_logger_->critical("[{}] {}", format_location(loc), msg);
        if (file_logger_) {
            file_logger_->critical("[{}] {}", format_location(loc), msg);
        }
    }
}

} // namespace console
