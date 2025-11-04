#include "MultiClientServer.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "utils.h"

static constexpr int LISTEN_BACKLOG = 16;

MultiClientServer &MultiClientServer::instance() {
  static MultiClientServer inst;
  return inst;
}

void MultiClientServer::ensure_started(uint16_t port) {
  LOG("MultiClientServer::ensure_started(.) called.");
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

  if (bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr_), sizeof(addr_)) < 0) {
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

  {
    std::lock_guard<std::mutex> lk(m_);
    reserved_.fill(false);
    token_to_pid_.clear();
    resume_handlers_.clear();
    next_handler_id_ = 1;
  }

  accept_thread_ = std::thread(&MultiClientServer::accept_loop, this);
}

void MultiClientServer::stop() {
  LOG("MultiClientServer::stop() called.");
  if (!started_)
    return;
  shutting_down_ = true;

  close_fd(listen_fd_);

  {
    std::lock_guard<std::mutex> lk(m_);
    for (auto &s : sessions_) {
      if (s && s->connected) {
        close_fd(s->sock);
      }
    }
    reserved_.fill(false);
    token_to_pid_.clear();
    resume_handlers_.clear();
  }

  if (accept_thread_.joinable())
    accept_thread_.join();

  {
    std::lock_guard<std::mutex> lk(m_);
    for (auto &s : sessions_) {
      if (s && s->reader_thread.joinable()) {
        s->reader_thread.join();
      }
      s.reset();
    }
  }

  started_ = false;
  LOG("MultiClientServer::stop() ended.");
}

MultiClientServer::~MultiClientServer() {
  stop();
  LOG("MultiClientServer: decustructed.");
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

void MultiClientServer::close_fd(int &fd) {
  if (fd >= 0) {
    ::close(fd);
    fd = -1;
  }
}

bool MultiClientServer::send_all(int fd, const char *data, size_t len) {
  size_t sent = 0;
  while (sent < len) {
    ssize_t n = ::send(fd, data + sent, len - sent, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (n == 0)
      return false;
    sent += static_cast<size_t>(n);
  }
  return true;
}

bool MultiClientServer::send_line(int fd, const std::string &line) {
  std::string msg = line;
  if (msg.empty() || msg.back() != '\n')
    msg.push_back('\n');
  return send_all(fd, msg.c_str(), msg.size());
}

bool MultiClientServer::read_line(int fd, std::string &out) {
  out.clear();
  char ch;
  while (true) {
    ssize_t n = ::recv(fd, &ch, 1, 0);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return false;
    }
    if (n == 0)
      return false;
    if (ch == '\n')
      break;
    out.push_back(ch);
    if (out.size() > 65536)
      return false;
  }
  return true;
}

bool MultiClientServer::read_line_timeout(int fd, std::string& out, int timeout_ms) {
  out.clear();
  struct pollfd pfd;
  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;
  int rv = ::poll(&pfd, 1, timeout_ms);
  if (rv <= 0) {
    return false; // timeout or error
  }
  if (!(pfd.revents & POLLIN)) {
    return false;
  }
  return read_line(fd, out);
}

static bool starts_with(const std::string& s, const std::string& pfx) {
  return s.size() >= pfx.size() && std::equal(pfx.begin(), pfx.end(), s.begin());
}

std::string MultiClientServer::generate_token() {
  // Simple hex token, 32 bytes entropy -> 64 hex chars
  std::random_device rd;
  std::ostringstream oss;
  for (int i = 0; i < 32; ++i) {
    unsigned byte = rd() & 0xFF;
    static const char* hex = "0123456789abcdef";
    oss << hex[(byte >> 4) & 0xF] << hex[byte & 0xF];
  }
  return oss.str();
}

int MultiClientServer::add_resume_handler(std::function<void(int)> fn) {
  std::lock_guard<std::mutex> lk(m_);
  int id = next_handler_id_++;
  resume_handlers_.push_back({id, std::move(fn)});
  return id;
}

void MultiClientServer::remove_resume_handler(int id) {
  std::lock_guard<std::mutex> lk(m_);
  auto it = std::remove_if(resume_handlers_.begin(), resume_handlers_.end(),
                           [&](const auto& p){ return p.first == id; });
  resume_handlers_.erase(it, resume_handlers_.end());
}

void MultiClientServer::fire_resume_handlers(int player_id) {
  // Copy handlers to avoid holding lock while invoking user code
  std::vector<std::function<void(int)>> cbs;
  {
    std::lock_guard<std::mutex> lk(m_);
    for (auto& p : resume_handlers_) cbs.push_back(p.second);
  }
  for (auto& cb : cbs) {
    try { cb(player_id); } catch (...) {}
  }
}

void MultiClientServer::accept_loop() {
  LOG("MultiClientServer::accept_loop() called.");
  while (!shutting_down_) {
    sockaddr_in cli{};
    socklen_t len = sizeof(cli);
    LOG("MultiClientServer::accept_loop(): waiting on accept");
    int cfd = ::accept(listen_fd_, reinterpret_cast<sockaddr *>(&cli), &len);
    if (cfd < 0) {
      if (errno == EINTR)
        continue;
      if (shutting_down_)
        break;
      std::perror("accept");
      continue;
    }
    LOG("MultiClientServer::accept_loop(): accepted fd: " << cfd);

    // Try to read an immediate RESUME <token> line (short timeout).
    std::string firstline;
    bool handled = false;
    if (read_line_timeout(cfd, firstline, 300) && starts_with(firstline, "RESUME ")) {
      std::string token = firstline.substr(std::string("RESUME ").size());
      int pid = -1;
      std::shared_ptr<Session> s;
      {
        std::lock_guard<std::mutex> lk(m_);
        auto it = token_to_pid_.find(token);
        if (it != token_to_pid_.end()) {
          pid = it->second;
          auto &slot = sessions_[pid];
          if (!slot) {
            slot = std::make_shared<Session>();
            slot->player_id = pid;
            slot->name = "Player" + std::to_string(pid);
            slot->token = token;
          } else {
            if (slot->connected) {
              close_fd(slot->sock);
            }
            if (slot->reader_thread.joinable()) {
              slot->reader_thread.join();
            }
          }
          slot->sock = cfd;
          slot->connected = true;
          s = slot;
        }
      }

      if (pid != -1) {
        // Send OK <pid> for compatibility
        std::string ok = "OK " + std::to_string(pid) + "\n";
        send_all(cfd, ok.c_str(), ok.size());

        // Start reader thread
        {
          std::lock_guard<std::mutex> lk(m_);
          auto &slot = sessions_[pid];
          slot->reader_thread = std::thread(&MultiClientServer::reader_loop, this, slot);
        }

        // Notify game to resend current state snapshot to this player
        fire_resume_handlers(pid);

        // Replay last prompt if any (after snapshot so UI is up to date)
        {
          std::lock_guard<std::mutex> lk(m_);
          auto &slot = sessions_[pid];
          if (!slot->last_prompt.empty()) {
            std::lock_guard<std::mutex> lk2(slot->send_mtx);
            send_all(slot->sock, slot->last_prompt.c_str(), slot->last_prompt.size());
          }
        }

        cv_.notify_all();
        handled = true;
      } else {
        // Invalid token -> fall through to normal assignment
      }
    }

    if (handled) {
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
    LOG("MultiClientServer::accept_loop(): pid: " << pid);
    if (pid == -1) {
      const char *full = "ERR server full\n";
      send_all(cfd, full, std::strlen(full));
      ::close(cfd);
      continue;
    }

    auto s = std::make_shared<Session>();
    s->sock = cfd;
    s->player_id = pid;
    s->name = "Player" + std::to_string(pid);
    s->connected = true;
    s->token = generate_token();

    {
      std::lock_guard<std::mutex> lk(m_);
      auto &slot = sessions_[pid];
      if (slot && slot->connected) {
        close_fd(slot->sock);
        if (slot->reader_thread.joinable())
          slot->reader_thread.join();
      }
      slot = s;
      reserved_[pid] = false; // consumed reservation
      token_to_pid_[s->token] = pid;
    }

    // Inform client of assigned id (compatibility)
    std::string ok = "OK " + std::to_string(pid) + "\n";
    send_all(cfd, ok.c_str(), ok.size());

    // Also send reconnect token as a framed message
    {
      std::lock_guard<std::mutex> lk2(s->send_mtx);
      std::string tok_msg = "/TOK" + s->token + "[SEP]";
      send_all(s->sock, tok_msg.c_str(), tok_msg.size());
    }

    s->reader_thread = std::thread(&MultiClientServer::reader_loop, this, s);
    cv_.notify_all();
  }
  LOG("MultiClientServer::accept_loop() ended.");
}

void MultiClientServer::reader_loop(std::shared_ptr<Session> s) {
  LOG("MultiClientServer::reader_thread(s) started: pl_id: " << s->player_id);
  while (!shutting_down_ && s->connected) {
    std::string line;
    if (!read_line(s->sock, line)) {
      break;
    }
    s->inbox.push(std::move(line));

    // A line received typically answers a prompt: clear last_prompt
    {
      std::lock_guard<std::mutex> lk(m_);
      auto &slot = sessions_[s->player_id];
      if (slot) {
        slot->last_prompt.clear();
      }
    }
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
  LOG("MultiClientServer: wait_for_client(" << player_id << ") ended.");
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

void MultiClientServer::send_to(int player_id, const std::string &msg) {
  std::shared_ptr<Session> s;
  {
    std::lock_guard<std::mutex> lk(m_);
    s = sessions_[player_id];
  }
  if (!s || !s->connected)
    return;
  std::lock_guard<std::mutex> lk2(s->send_mtx);
  if (!send_all(s->sock, msg.c_str(), msg.size())) {
    s->connected = false;
    close_fd(s->sock);
    cv_.notify_all();
  }
}

void MultiClientServer::send_prompt(int player_id, const std::string& prompt_with_prefix) {
  // prompt_with_prefix must start with "/INP" and NOT include trailing "[SEP]".
  std::shared_ptr<Session> s;
  {
    std::lock_guard<std::mutex> lk(m_);
    s = sessions_[player_id];
    if (!s) return;
    s->last_prompt = prompt_with_prefix + "[SEP]";
  }
  send_to(player_id, prompt_with_prefix + "[SEP]");
}

void MultiClientServer::broadcast(const std::string &msg) {
  std::lock_guard<std::mutex> lk(m_);
  for (auto &s : sessions_) {
    if (!s || !s->connected)
      continue;
    std::lock_guard<std::mutex> lk2(s->send_mtx);
    send_all(s->sock, msg.c_str(), msg.size());
  }
}

bool MultiClientServer::is_connected(int player_id) const {
  std::lock_guard<std::mutex> lk(m_);
  auto s = sessions_[player_id];
  return s && s->connected;
}

bool MultiClientServer::any_connected() const {
  std::lock_guard<std::mutex> lk(m_);
  for (const auto &s : sessions_) {
    if (s && s->connected)
      return true;
  }
  return false;
}

bool MultiClientServer::any_reserved() const {
  std::lock_guard<std::mutex> lk(m_);
  for (bool r : reserved_) {
    if (r)
      return true;
  }
  return false;
}
