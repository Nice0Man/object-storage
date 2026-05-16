#pragma once

#include "console/common/Types.hpp"

#include <atomic>
#include <functional>
#include <thread>

namespace console::websocket {

/**
 * @brief One pseudo-terminal session (forkpty + reader thread).
 */
class PtySession {
  public:
    using OutputCallback = std::function<void(const char* data, size_t len)>;

    PtySession() = default;
    ~PtySession();

    PtySession(const PtySession&) = delete;
    PtySession& operator=(const PtySession&) = delete;

    bool start(const String& shell, OutputCallback on_output);
    void write_input(const char* data, size_t len);
    void resize(uint16_t cols, uint16_t rows);
    void stop();

    bool is_running() const { return running_.load(); }

  private:
    void read_loop();
    void set_nonblocking(int fd);

    int master_fd_{-1};
    int pid_{-1};
    std::thread reader_thread_;
    std::atomic<bool> running_{false};
    OutputCallback on_output_;
};

} // namespace console::websocket
