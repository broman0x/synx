#include "server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <csignal>
#include <sys/wait.h>
#include "../utils/logger.h"

namespace synx {

static volatile sig_atomic_t g_running = 1;

Server::Server()
    : fd_epoll_(-1)
    , running_(false)
    , is_master_(true)
    , worker_id_(0)
    , daftar_event_(1024)
{}

Server::~Server() {
    stop();
}

void Server::worker_signal_handler(int signum) {
    (void)signum;
    g_running = 0;
}

bool Server::init(const ConfigServer& config) {
    config_ = config;
    
    if (!get_logger().init(config.log_file)) {
        std::cerr << "[WARNING] Gagal init logger" << std::endl;
    }
    
    fd_epoll_ = epoll_create1(EPOLL_CLOEXEC);
    if (fd_epoll_ < 0) {
        std::cerr << "[ERROR] Gagal buat epoll" << std::endl;
        return false;
    }
    
#ifdef ENABLE_HTTPS
    if (config_.enable_https) {
        SSLContext::init_openssl();
        
        if (!ssl_ctx_.init()) {
            std::cerr << "[ERROR] Gagal init SSL context" << std::endl;
            return false;
        }
        
        if (!ssl_ctx_.load_cert(config_.cert_path, config_.key_path)) {
            std::cerr << "[ERROR] Gagal load certificate/key" << std::endl;
            return false;
        }
        
        std::cout << "[INFO] HTTPS enabled on port " << config_.https_port << std::endl;
    }
#endif
    
    SocketInfo http_socket;
    http_socket.fd = -1;
    http_socket.is_https = false;
    http_socket.ssl_ctx = nullptr;
    
    http_socket.fd = buat_socket_server(config_.port, false);
    if (http_socket.fd < 0) {
        return false;
    }
    daftar_socket_.push_back(http_socket);
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = http_socket.fd;
    if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, http_socket.fd, &ev) < 0) {
        std::cerr << "[ERROR] Gagal daftar HTTP socket ke epoll" << std::endl;
        return false;
    }
    
    std::cout << "[INFO] HTTP server siap di port " << config_.port << std::endl;
    
#ifdef ENABLE_HTTPS
    if (config_.enable_https) {
        SocketInfo https_socket;
        https_socket.fd = -1;
        https_socket.is_https = true;
        https_socket.ssl_ctx = &ssl_ctx_;
        
        https_socket.fd = buat_socket_server(config_.https_port, true);
        if (https_socket.fd < 0) {
            std::cerr << "[WARNING] Gagal buat HTTPS socket" << std::endl;
        } else {
            ev.events = EPOLLIN;
            ev.data.fd = https_socket.fd;
            if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, https_socket.fd, &ev) < 0) {
                std::cerr << "[ERROR] Gagal daftar HTTPS socket ke epoll" << std::endl;
                close(https_socket.fd);
            } else {
                daftar_socket_.push_back(https_socket);
                std::cout << "[INFO] HTTPS server siap di port " << config_.https_port << std::endl;
            }
        }
    }
#endif
    
    std::cout << "[INFO] Root directory: " << config_.root << std::endl;
    std::cout << "[INFO] Max connections: " << config_.max_koneksi << std::endl;
    std::cout << "[INFO] Worker count: " << config_.worker_count << std::endl;
    
    return true;
}

int Server::buat_socket_server(int port, bool) {
    int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return -1;
    }
    
    if (fd < 3) {
        int new_fd = fcntl(fd, F_DUPFD, 3);
        if (new_fd < 0) {
            close(fd);
            return -1;
        }
        close(fd);
        fd = new_fd;
    }
    
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(fd);
        return -1;
    }
    
    struct sockaddr_in addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    
    if (listen(fd, config_.max_koneksi) < 0) {
        close(fd);
        return -1;
    }
    
    return fd;
}

void Server::run() {
    if (config_.benchmark_mode) {
        run_benchmark();
        return;
    }
    
    if (config_.worker_count > 1 && is_master_) {
        std::cout << "[INFO] Master process starting " << config_.worker_count << " workers" << std::endl;
        
        std::vector<pid_t> worker_pids;
        for (int i = 0; i < config_.worker_count - 1; i++) {
            pid_t pid = fork();
            if (pid < 0) {
                std::cerr << "[ERROR] Gagal fork worker" << std::endl;
            } else if (pid == 0) {
                is_master_ = false;
                worker_id_ = i + 1;
                running_ = true;
                signal(SIGTERM, worker_signal_handler);
                signal(SIGINT, worker_signal_handler);
                run_worker();
                exit(0);
            } else {
                worker_pids.push_back(pid);
            }
        }
        
        signal(SIGCHLD, SIG_IGN);
        signal(SIGTERM, worker_signal_handler);
        signal(SIGINT, worker_signal_handler);
        
        worker_id_ = 0;
        running_ = true;
        run_worker();
        
        while (g_running) {
            waitpid(-1, nullptr, WNOHANG);
            usleep(100000);
        }
        
        for (pid_t p : worker_pids) {
            kill(p, SIGTERM);
        }
        for (pid_t p : worker_pids) {
            waitpid(p, nullptr, 0);
        }
    } else {
        running_ = true;
        run_worker();
    }
}

void Server::run_worker() {
    std::cout << "[INFO] Worker " << worker_id_ << " starting event loop" << std::endl;
    
    while (running_ && g_running) {
        cleanup_rate_limit();
        
        int n_events = epoll_wait(fd_epoll_, daftar_event_.data(), 
                                   static_cast<int>(daftar_event_.size()), 1000);
        
        if (n_events < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[ERROR] epoll_wait: " << strerror(errno) << std::endl;
            break;
        }
        
        for (int i = 0; i < n_events; ++i) {
            int fd = daftar_event_[i].data.fd;
            uint32_t events = daftar_event_[i].events;
            
            SocketInfo* socket_info = nullptr;
            for (auto& sock : daftar_socket_) {
                if (sock.fd == fd) {
                    socket_info = &sock;
                    break;
                }
            }
            
            if (socket_info) {
                if (events & EPOLLIN) {
                    terima_koneksi_baru(*socket_info);
                }
            } else {
                auto it = daftar_koneksi_.find(fd);
                if (it != daftar_koneksi_.end()) {
                    KoneksiAktif& koneksi = *it->second;
                    
                    if (events & (EPOLLIN | EPOLLRDHUP)) {
                        tangani_baca(koneksi);
                    } else if (events & EPOLLOUT) {
                        tangani_tulis(koneksi);
                    }
                    
                    if (events & EPOLLERR) {
                        hapus_koneksi(fd);
                    }
                }
            }
        }
    }
}

void Server::stop() {
    running_ = false;
    g_running = 0;
    
    for (auto& kv : daftar_koneksi_) {
        kv.second->tutup();
    }
    daftar_koneksi_.clear();
    
    for (auto& sock : daftar_socket_) {
        if (sock.fd >= 0) {
            close(sock.fd);
        }
    }
    daftar_socket_.clear();
    
    if (fd_epoll_ >= 0) {
        close(fd_epoll_);
        fd_epoll_ = -1;
    }
    
#ifdef ENABLE_HTTPS
    SSLContext::cleanup_openssl();
#endif
}

void Server::terima_koneksi_baru(SocketInfo& socket_info) {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept4(socket_info.fd, (struct sockaddr*)&client_addr, 
                                 &client_len, SOCK_NONBLOCK | SOCK_CLOEXEC);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            }
            break;
        }
        
        if (daftar_koneksi_.size() >= static_cast<size_t>(config_.max_koneksi)) {
            close(client_fd);
            continue;
        }
        
        auto koneksi = std::make_unique<KoneksiAktif>();
        koneksi->fd = client_fd;
        koneksi->ip_client = ambil_ip_peer(client_fd);
        koneksi->is_https = socket_info.is_https;
        
#ifdef ENABLE_HTTPS
        if (socket_info.is_https && socket_info.ssl_ctx) {
            koneksi->ssl_conn = std::make_unique<SSLConnection>(
                socket_info.ssl_ctx->get_ctx(), client_fd);
            
            int ret = koneksi->ssl_conn->handshake();
            if (ret <= 0) {
                int err = SSL_get_error(koneksi->ssl_conn->get_ssl(), ret);
                if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                    close(client_fd);
                    continue;
                }
            }
        }
#endif
        
        struct epoll_event ev;
        ev.events = EPOLLIN;
        ev.data.fd = client_fd;
        
        if (epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            close(client_fd);
            continue;
        }
        
        daftar_koneksi_[client_fd] = std::move(koneksi);
    }
}

bool Server::cek_rate_limit(const std::string& ip) {
    if (config_.rate_limit <= 0) {
        return true;
    }
    
    auto now = std::chrono::steady_clock::now();
    int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    auto it = rate_limit_map_.find(ip);
    if (it == rate_limit_map_.end()) {
        RateLimitEntry entry;
        entry.waktu_mulai = now_sec;
        entry.request_count = 1;
        rate_limit_map_[ip] = entry;
        return true;
    }
    
    if (now_sec - it->second.waktu_mulai >= 1) {
        it->second.waktu_mulai = now_sec;
        it->second.request_count = 1;
        return true;
    }
    
    if (it->second.request_count >= config_.rate_limit) {
        return false;
    }
    
    it->second.request_count++;
    return true;
}

void Server::cleanup_rate_limit() {
    auto now = std::chrono::steady_clock::now();
    int64_t now_sec = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    auto it = rate_limit_map_.begin();
    while (it != rate_limit_map_.end()) {
        if (now_sec - it->second.waktu_mulai > 60) {
            it = rate_limit_map_.erase(it);
        } else {
            ++it;
        }
    }
}

void Server::tangani_baca(KoneksiAktif& koneksi) {
    if (!cek_rate_limit(koneksi.ip_client)) {
        hapus_koneksi(koneksi.fd);
        return;
    }
    
    ssize_t n = 0;
    
#ifdef ENABLE_HTTPS
    if (koneksi.is_https && koneksi.ssl_conn && koneksi.ssl_conn->is_secure()) {
        n = koneksi.ssl_conn->read(koneksi.buffer_baca.data(),
                                    koneksi.buffer_baca.size());
    } else
#endif
    {
        n = recv(koneksi.fd, koneksi.buffer_baca.data(), 
                 koneksi.buffer_baca.size(), 0);
    }
    
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        hapus_koneksi(koneksi.fd);
        return;
    }
    
    if (n == 0) {
        hapus_koneksi(koneksi.fd);
        return;
    }
    
    if (koneksi.parser_http.get_buffer().size() + n > config_.max_request_size) {
        HandlerRequest::buat_respon_error(koneksi, 413, "Payload Too Large");
        update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
        return;
    }
    
    auto now = std::chrono::steady_clock::now();
    koneksi.waktu_terakhir_aktif = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
    
    StatusParse status = koneksi.parser_http.parse(koneksi.buffer_baca.data(), 
                                                    static_cast<size_t>(n), 
                                                    koneksi.request);
    
    if (status == StatusParse::BELUM_SELESAI) {
        return;
    }
    
    if (status == StatusParse::ERROR) {
        HandlerRequest::buat_respon_error(koneksi, 400, "Bad Request");
        update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
        return;
    }
    
    koneksi.status = StatusKoneksi::LAGI_PROSES;
    
    bool is_proxied = false;
    UpstreamServer target_upstream;
    
    for (const auto& route : config_.routes) {
        if (koneksi.request.path.find(route.path_prefix) == 0) {
            is_proxied = true;
            target_upstream.host = route.upstream_host;
            target_upstream.port = route.upstream_port;
            target_upstream.weight = 1;
            break;
        }
    }
    
    if (!is_proxied && config_.reverse_proxy && !config_.upstreams.empty()) {
        size_t idx = koneksi.request_count % config_.upstreams.size();
        target_upstream = config_.upstreams[idx];
        is_proxied = true;
    }
    
    if (is_proxied) {
        int proxy_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
        if (proxy_fd < 0) {
            HandlerRequest::buat_respon_error(koneksi, 500, "Internal Server Error");
            update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
            return;
        }
        
        struct sockaddr_in proxy_addr;
        std::memset(&proxy_addr, 0, sizeof(proxy_addr));
        proxy_addr.sin_family = AF_INET;
        proxy_addr.sin_port = htons(target_upstream.port);
        inet_pton(AF_INET, target_upstream.host.c_str(), &proxy_addr.sin_addr);
        
        if (connect(proxy_fd, (struct sockaddr*)&proxy_addr, sizeof(proxy_addr)) < 0) {
            if (errno != EINPROGRESS) {
                close(proxy_fd);
                HandlerRequest::buat_respon_error(koneksi, 502, "Bad Gateway");
                update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
                return;
            }
        }
        
        std::string proxy_request = koneksi.request.method + " " + 
                                     koneksi.request.path + " HTTP/1.1\r\n";
        proxy_request += "Host: " + target_upstream.host + "\r\n";
        proxy_request += "Connection: close\r\n\r\n";
        
        send(proxy_fd, proxy_request.c_str(), proxy_request.size(), 0);
        
        char proxy_buf[8192];
        ssize_t proxy_n = recv(proxy_fd, proxy_buf, sizeof(proxy_buf), 0);
        
        if (proxy_n > 0) {
            koneksi.panjang_kirim = static_cast<size_t>(proxy_n);
            if (koneksi.buffer_kirim.size() < koneksi.panjang_kirim) {
                koneksi.buffer_kirim.resize(koneksi.panjang_kirim);
            }
            std::memcpy(koneksi.buffer_kirim.data(), proxy_buf, koneksi.panjang_kirim);
            koneksi.posisi_kirim = 0;
            koneksi.status = StatusKoneksi::SIAP_KIRIM;
        } else {
            HandlerRequest::buat_respon_error(koneksi, 502, "Bad Gateway");
        }
        
        close(proxy_fd);
        update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
    } else {
        HandlerRequest::proses_request(koneksi, config_.root);
        
        get_logger().tulis_access_log(koneksi.ip_client,
                                       koneksi.request.method,
                                       koneksi.request.path,
                                       koneksi.response.kode_status,
                                       koneksi.panjang_kirim);
        
        update_koneksi_di_epoll(koneksi.fd, EPOLLOUT);
    }
    
    koneksi.request_count++;
}

void Server::tangani_tulis(KoneksiAktif& koneksi) {
    while (koneksi.posisi_kirim < koneksi.panjang_kirim) {
        ssize_t n = 0;
        size_t remaining = koneksi.panjang_kirim - koneksi.posisi_kirim;
        
#ifdef ENABLE_HTTPS
        if (koneksi.is_https && koneksi.ssl_conn && koneksi.ssl_conn->is_secure()) {
            n = koneksi.ssl_conn->write(koneksi.buffer_kirim.data() + koneksi.posisi_kirim,
                                         static_cast<int>(remaining));
        } else
#endif
        {
            n = send(koneksi.fd, 
                     koneksi.buffer_kirim.data() + koneksi.posisi_kirim,
                     remaining, 
                     MSG_NOSIGNAL);
        }
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return;
            }
            hapus_koneksi(koneksi.fd);
            return;
        }
        
        koneksi.posisi_kirim += static_cast<size_t>(n);
        koneksi.bytes_dikirim += static_cast<size_t>(n);
    }
    
    if (koneksi.is_keep_alive() && koneksi.request_count < 100) {
        koneksi.parser_http.reset();
        koneksi.request = RequestHTTP();
        koneksi.response = ResponseHTTP();
        koneksi.posisi_kirim = 0;
        koneksi.panjang_kirim = 0;
        koneksi.status = StatusKoneksi::NUNGGU_BACA;
        
        update_koneksi_di_epoll(koneksi.fd, EPOLLIN);
    } else {
        hapus_koneksi(koneksi.fd);
    }
}

void Server::hapus_koneksi(int fd) {
    auto it = daftar_koneksi_.find(fd);
    if (it != daftar_koneksi_.end()) {
        epoll_ctl(fd_epoll_, EPOLL_CTL_DEL, fd, nullptr);
        it->second->tutup();
        daftar_koneksi_.erase(it);
    }
}

void Server::tambah_koneksi_ke_epoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(fd_epoll_, EPOLL_CTL_ADD, fd, &ev);
}

void Server::update_koneksi_di_epoll(int fd, uint32_t events) {
    struct epoll_event ev;
    ev.events = events;
    ev.data.fd = fd;
    epoll_ctl(fd_epoll_, EPOLL_CTL_MOD, fd, &ev);
}

std::string Server::ambil_ip_peer(int fd) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    
    if (getpeername(fd, (struct sockaddr*)&addr, &len) == 0) {
        return inet_ntoa(addr.sin_addr);
    }
    
    return "unknown";
}

void Server::run_benchmark() {
    std::cout << "[INFO] Starting benchmark mode" << std::endl;
    std::cout << "[INFO] Duration: " << config_.benchmark_duration << "s" << std::endl;
    std::cout << "[INFO] Connections: " << config_.benchmark_connections << std::endl;
    
    auto start = std::chrono::steady_clock::now();
    auto end = start + std::chrono::seconds(config_.benchmark_duration);
    
    running_ = true;
    while (running_ && std::chrono::steady_clock::now() < end) {
        int n_events = epoll_wait(fd_epoll_, daftar_event_.data(), 
                                   static_cast<int>(daftar_event_.size()), 100);
        
        if (n_events < 0) {
            if (errno == EINTR) continue;
            break;
        }
        
        for (int i = 0; i < n_events; ++i) {
            int fd = daftar_event_[i].data.fd;
            uint32_t events = daftar_event_[i].events;
            
            SocketInfo* socket_info = nullptr;
            for (auto& sock : daftar_socket_) {
                if (sock.fd == fd) {
                    socket_info = &sock;
                    break;
                }
            }
            
            if (socket_info) {
                if (events & EPOLLIN) {
                    terima_koneksi_baru(*socket_info);
                }
            } else {
                auto it = daftar_koneksi_.find(fd);
                if (it != daftar_koneksi_.end()) {
                    KoneksiAktif& koneksi = *it->second;
                    
                    if (events & EPOLLIN) {
                        tangani_baca(koneksi);
                    } else if (events & EPOLLOUT) {
                        tangani_tulis(koneksi);
                    }
                    
                    if (events & EPOLLERR) {
                        hapus_koneksi(fd);
                    }
                }
            }
        }
    }
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    
    std::cout << "\n========== BENCHMARK RESULTS ==========" << std::endl;
    std::cout << "Duration: " << duration << "s" << std::endl;
    std::cout << "Total Requests: " << get_logger().get_request_count() << std::endl;
    std::cout << "Requests/sec: " << (duration > 0 ? get_logger().get_request_count() / duration : 0) << std::endl;
    std::cout << "Errors: " << get_logger().get_error_count() << std::endl;
    std::cout << "========================================" << std::endl;
}

}
