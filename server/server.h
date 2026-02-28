#ifndef SERVER_H
#define SERVER_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <sys/epoll.h>
#include "../config/config.h"
#include "../core/koneksi.h"
#include "../utils/ssl.h"

namespace synx {

struct SocketInfo {
    int fd;
    bool is_https;
    SSLContext* ssl_ctx;
};

struct RateLimitEntry {
    int64_t waktu_mulai;
    int request_count;
};

class Server {
public:
    Server();
    ~Server();
    
    bool init(const ConfigServer& config);
    void run();
    void stop();
    void run_benchmark();
    
private:
    std::vector<SocketInfo> daftar_socket_;
    int fd_epoll_;
    ConfigServer config_;
    bool running_;
    bool is_master_;
    int worker_id_;
    
    SSLContext ssl_ctx_;
    
    std::unordered_map<int, std::unique_ptr<KoneksiAktif>> daftar_koneksi_;
    std::vector<struct epoll_event> daftar_event_;
    
    std::unordered_map<std::string, RateLimitEntry> rate_limit_map_;
    
    int buat_socket_server(int port, bool is_https);
    void terima_koneksi_baru(SocketInfo& socket_info);
    void tangani_baca(KoneksiAktif& koneksi);
    void tangani_tulis(KoneksiAktif& koneksi);
    void hapus_koneksi(int fd);
    void tambah_koneksi_ke_epoll(int fd, uint32_t events);
    void update_koneksi_di_epoll(int fd, uint32_t events);
    std::string ambil_ip_peer(int fd);
    bool cek_rate_limit(const std::string& ip);
    void cleanup_rate_limit();
    
    static void worker_signal_handler(int signum);
    void run_worker();
};

}

#endif
