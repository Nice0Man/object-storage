#include "console/websocket/PtySession.hpp"

#include "console/common/Logger.hpp"

#include <cerrno>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#if defined(__linux__) || defined(__APPLE__)
#include <pty.h>
#include <sys/ioctl.h>
#include <termios.h>
#endif

namespace console::websocket {

PtySession::~PtySession() {
    stop();
}

void
PtySession::set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags >= 0) {
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    }
}

bool
PtySession::start(const String& shell, OutputCallback on_output) {
#if !defined(__linux__) && !defined(__APPLE__)
    (void)shell;
    (void)on_output;
    CONSOLE_LOG_ERROR("PTY terminal is not supported on this platform");
    return false;
#else
    if (running_.load()) {
        return false;
    }

    on_output_ = std::move(on_output);
    pid_t pid = forkpty(&master_fd_, nullptr, nullptr, nullptr);
    if (pid < 0) {
        CONSOLE_LOG_ERROR("forkpty failed: {}", strerror(errno));
        return false;
    }

    if (pid == 0) {
        setenv("TERM", "xterm-256color", 1);
        setenv("LANG", "C.UTF-8", 1);
        execl(shell.c_str(), shell.c_str(), nullptr);
        _exit(127);
    }

    pid_ = static_cast<int>(pid);
    set_nonblocking(master_fd_);
    running_.store(true);
    reader_thread_ = std::thread([this]() { read_loop(); });
    return true;
#endif
}

void
PtySession::read_loop() {
#if defined(__linux__) || defined(__APPLE__)
    char buffer[4096];
    while (running_.load()) {
        ssize_t n = ::read(master_fd_, buffer, sizeof(buffer));
        if (n > 0) {
            if (on_output_) {
                on_output_(buffer, static_cast<size_t>(n));
            }
        } else if (n == 0) {
            break;
        } else if (errno == EAGAIN || errno == EINTR) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        } else {
            break;
        }
    }
#endif
}

void
PtySession::write_input(const char* data, size_t len) {
#if defined(__linux__) || defined(__APPLE__)
    if (master_fd_ < 0 || !running_.load()) {
        return;
    }
    size_t offset = 0;
    while (offset < len) {
        ssize_t written = ::write(master_fd_, data + offset, len - offset);
        if (written < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        offset += static_cast<size_t>(written);
    }
#else
    (void)data;
    (void)len;
#endif
}

void
PtySession::resize(uint16_t cols, uint16_t rows) {
#if defined(__linux__) || defined(__APPLE__)
    if (master_fd_ < 0) {
        return;
    }
    struct winsize ws {};
    ws.ws_row = rows;
    ws.ws_col = cols;
    ioctl(master_fd_, TIOCSWINSZ, &ws);
#else
    (void)cols;
    (void)rows;
#endif
}

void
PtySession::stop() {
    running_.store(false);

#if defined(__linux__) || defined(__APPLE__)
    if (master_fd_ >= 0) {
        ::close(master_fd_);
        master_fd_ = -1;
    }
    if (pid_ > 0) {
        kill(pid_, SIGHUP);
        int status = 0;
        waitpid(pid_, &status, WNOHANG);
        pid_ = -1;
    }
#endif

    if (reader_thread_.joinable()) {
        reader_thread_.join();
    }
}

} // namespace console::websocket
