#include "MultiClientServer.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>

#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

static constexpr int LISTEN_BACKLOG = 16;

MultiClientServer& MultiClientServer::instance() {
    static MultiClientServer inst;
    return inst;
}

void MultiClientServer::ensure_started(uint16_t port) {
    bool expected = false;
    if (!started_.compare_exchange_strong(expected, true)) {
        return; // already started
    }
    port_ = port;
    shutting_down_ = false;

#ifdef SIGPIPE
    std::signal(SIGPIPE, SIG_IGN);
#endif

    listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::perror("socket");
        std::cerr << "MultiClientServer: failed to create socket\n";
        std::abort();
    }

    int one = 1;
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
        std::perror("setsockopt(SO_REUSEADDR)");
        std::cerr << "MultiClientServer: failed to set SO_REUSEADDR\n";
        std::abort();
    }
#if defined(SO_REUSEPORT)
    if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEPORT, &one, sizeof(one)) < 0) {
        std::perror("setsockopt(SO_REUSEPORT)");
        std::cerr << "MultiClientServer: continuing without SO_REUSEPORT\n";
    }
#endif

    std::memset(&addr_, 0, sizeof(addr_));
    addr_.sin_family = AF_INET;
    addr_.sin_addr.s_addr = INADDR_ANY;
    addr_.sin_port = htons(port_);

    if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr_), sizeof(addr_)) < 0) {
        std::perror("bind");
        std::cerr << "MultiClientServer: bind failed on port " << port_ << "\n";
        std::abort();
    }

    if (listen(listen_fd_, LISTEN_BACKLOG) < 0) {
        std::perror("listen");
        std::cerr << "MultiClientServer: listen failed\n";
        std::abort();
    }

    std::cout << "Server listening on port " << port_ << std::endl;

    // Clear reservations
    {
        std::lock_guard<std::mutex> lk(m_);
        reserved_.fill(false);
    }

    accept_thread_ = std::thread(&MultiClientServer::accept_loop, this);
}

void MultiClientServer::stop() {
    if (!started_) return;
    shutting_down_ = true;

    close_fd(listen_fd_);

    {
        std::lock_guard<std::mutex> lk(m_);
        for (auto& s : sessions_) {
            if (s && s->connected) {
                close_fd(s->sock);
            }
        }
        reserved_.fill(false);
    }

    if (accept_thread_.joinable()) accept_thread_.join();

    {
        std::lock_guard<std::mutex> lk(m_);
        for (auto& s : sessions_) {
            if (s && s->reader_thread.joinable()) {
                s->reader_thread.join();
            }
            s.reset();
        }
    }

    started_ = false;
}

MultiClientServer::~MultiClientServer() {
    stop();
}

void MultiClientServer::reserve_slot(int player_id) {
    std::lock_guard<std::mutex> lk(m_);
    if (player_id >= 0 && player_id < (int)Hokm::N_PLAYERS) {
        reserved_[player_id] = true;
    }
}

void MultiClientServer::release_slot(int player_id) {
    std::lock_guard<std::mutex> lk(m_);
    if (player_id >= 0 && player_id < (int)Hokm::N_PLAYERS) {
        reserved_[player_id] = false;
    }
}

void MultiClientServer::close_fd(int& fd) {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool MultiClientServer::send_all(int fd, const char* data, size_t len) {
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = ::send(fd, data + sent, len - sent, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        sent += static_cast<size_t>(n);
    }
    return true;
}

bool MultiClientServer::send_line(int fd, const std::string& line) {
    std::string msg = line;
    if (msg.empty() || msg.back() != '\n') msg.push_back('\n');
    return send_all(fd, msg.c_str(), msg.size());
}

bool MultiClientServer::read_line(int fd, std::string& out) {
    out.clear();
    char ch;
    while (true) {
        ssize_t n = ::recv(fd, &ch, 1, 0);
        if (n < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        if (n == 0) return false;
        if (ch == '\n') break;
        out.push_back(ch);
        if (out.size() > 65536) return false;
    }
    return true;
}

void MultiClientServer::accept_loop() {
    while (!shutting_down_) {
        sockaddr_in cli{};
        socklen_t len = sizeof(cli);
        int cfd = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&cli), &len);
        if (cfd < 0) {
            if (errno == EINTR) continue;
            if (shutting_down_) break;
            std::perror("accept");
            continue;
        }

        // Choose slot: prefer reserved slots that are not yet connected,
        // otherwise pick the first free slot.
        int pid = -1;
        {
            std::lock_guard<std::mutex> lk(m_);
            for (int i = 0; i < (int)Hokm::N_PLAYERS; ++i) {
                if (reserved_[i] && (!sessions_[i] || !sessions_[i]->connected)) {
                    pid = i;
                    break;
                }
            }
            if (pid == -1) {
                for (int i = 0; i < (int)Hokm::N_PLAYERS; ++i) {
                    if (!sessions_[i] || !sessions_[i]->connected) {
                        pid = i;
                        break;
                    }
                }
            }
        }

        if (pid == -1) {
            const char* full = "ERR server full\n";
            send_all(cfd, full, std::strlen(full));
            ::close(cfd);
            continue;
        }

        auto s = std::make_shared<Session>();
        s->sock = cfd;
        s->player_id = pid;
        s->name = "Player" + std::to_string(pid);
        s->connected = true;

        {
            std::lock_guard<std::mutex> lk(m_);
            auto& slot = sessions_[pid];
            if (slot && slot->connected) {
                close_fd(slot->sock);
                if (slot->reader_thread.joinable()) slot->reader_thread.join();
            }
            slot = s;
            reserved_[pid] = false; // consumed reservation
        }

        // Inform client of assigned id
        std::string ok = "OK " + std::to_string(pid) + "\n";
        send_all(cfd, ok.c_str(), ok.size());

        s->reader_thread = std::thread(&MultiClientServer::reader_loop, this, s);

        cv_.notify_all();
    }
}

void MultiClientServer::reader_loop(std::shared_ptr<Session> s) {
    while (!shutting_down_ && s->connected) {
        std::string line;
        if (!read_line(s->sock, line)) {
            break;
        }
        s->inbox.push(std::move(line));
    }
    s->connected = false;
    close_fd(s->sock);
    cv_.notify_all();
}

void MultiClientServer::wait_for_client(int player_id) {
    std::unique_lock<std::mutex> lk(m_);
    cv_.wait(lk, [&] {
        auto s = sessions_[player_id];
        return s && s->connected;
    });
}

std::string MultiClientServer::recv_from(int player_id) {
    std::shared_ptr<Session> s;
    {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] {
            auto ss = sessions_[player_id];
            return ss && ss->connected;
        });
        s = sessions_[player_id];
    }
    return s->inbox.pop_wait();
}

void MultiClientServer::send_to(int player_id, const std::string& msg) {
    std::shared_ptr<Session> s;
    {
        std::lock_guard<std::mutex> lk(m_);
        s = sessions_[player_id];
    }
    if (!s || !s->connected) return;
    std::lock_guard<std::mutex> lk2(s->send_mtx);
    if (!send_all(s->sock, msg.c_str(), msg.size())) {
        s->connected = false;
        close_fd(s->sock);
        cv_.notify_all();
    }
}

void MultiClientServer::broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lk(m_);
    for (auto& s : sessions_) {
        if (!s || !s->connected) continue;
        std::lock_guard<std::mutex> lk2(s->send_mtx);
        send_all(s->sock, msg.c_str(), msg.size());
    }
}

bool MultiClientServer::is_connected(int player_id) const {
    std::lock_guard<std::mutex> lk(m_);
    auto s = sessions_[player_id];
    return s && s->connected;
}
