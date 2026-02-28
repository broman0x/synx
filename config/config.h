#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <cstdint>
#include <vector>

namespace synx {

struct UpstreamServer {
    std::string host;
    uint16_t port;
    int weight;
};

struct ProxyRoute {
    std::string path_prefix;
    std::string upstream_host;
    uint16_t upstream_port;
};

struct ConfigServer {
    uint16_t port;
    std::string root;
    std::string log_file;
    int max_koneksi;
    int buffer_size;
    int timeout_keepalive;
    int worker_count;
    bool enable_https;
    uint16_t https_port;
    std::string cert_path;
    std::string key_path;
    bool reverse_proxy;
    std::vector<UpstreamServer> upstreams;
    std::vector<ProxyRoute> routes;
    int request_timeout;
    size_t max_request_size;
    int rate_limit;
    bool benchmark_mode;
    int benchmark_duration;
    int benchmark_connections;

    ConfigServer();
    bool load_dari_file(const std::string& path);
};

class ParserConfig {
public:
    static bool parse(const std::string& isi, ConfigServer& config);
    static std::string trim(const std::string& s);
};

}

#endif
