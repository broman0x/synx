#include "config.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace synx {

ConfigServer::ConfigServer()
    : port(8080)
    , root("./public")
    , log_file("access.log")
    , max_koneksi(1024)
    , buffer_size(8192)
    , timeout_keepalive(30)
    , worker_count(1)
    , enable_https(false)
    , https_port(8443)
    , cert_path("cert.crt")
    , key_path("cert.key")
    , reverse_proxy(false)
    , request_timeout(30)
    , max_request_size(1048576)
    , rate_limit(100)
    , benchmark_mode(false)
    , benchmark_duration(10)
    , benchmark_connections(100)
{}

std::string ParserConfig::trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) {
        ++start;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
    }
    return s.substr(start, end - start);
}

bool ParserConfig::parse(const std::string& isi, ConfigServer& config) {
    std::istringstream stream(isi);
    std::string line;
    
    while (std::getline(stream, line)) {
        line = trim(line);
        
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        size_t pos = line.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        
        std::string key = trim(line.substr(0, pos));
        std::string value = trim(line.substr(pos + 1));
        
        if (key == "port") {
            config.port = static_cast<uint16_t>(std::stoi(value));
        } else if (key == "root") {
            config.root = value;
        } else if (key == "log") {
            config.log_file = value;
        } else if (key == "max_connections") {
            config.max_koneksi = std::stoi(value);
        } else if (key == "buffer_size") {
            config.buffer_size = std::stoi(value);
        } else if (key == "keepalive_timeout") {
            config.timeout_keepalive = std::stoi(value);
        } else if (key == "worker_count") {
            config.worker_count = std::stoi(value);
        } else if (key == "https_enable") {
            config.enable_https = (value == "true" || value == "1");
        } else if (key == "https_port") {
            config.https_port = static_cast<uint16_t>(std::stoi(value));
        } else if (key == "https_cert") {
            config.cert_path = value;
        } else if (key == "https_key") {
            config.key_path = value;
        } else if (key == "reverse_proxy") {
            config.reverse_proxy = (value == "true" || value == "1");
        } else if (key == "upstream") {
            size_t colon = value.find(':');
            if (colon != std::string::npos) {
                UpstreamServer upstream;
                upstream.host = value.substr(0, colon);
                upstream.port = static_cast<uint16_t>(std::stoi(value.substr(colon + 1)));
                upstream.weight = 1;
                size_t weight_pos = value.find('@');
                if (weight_pos != std::string::npos) {
                    upstream.weight = std::stoi(value.substr(weight_pos + 1));
                }
                config.upstreams.push_back(upstream);
            }
        } else if (key == "route") {
            size_t space_pos = value.find(' ');
            if (space_pos != std::string::npos) {
                std::string prefix = trim(value.substr(0, space_pos));
                std::string target = trim(value.substr(space_pos + 1));
                size_t colon = target.find(':');
                if (colon != std::string::npos) {
                    ProxyRoute route;
                    route.path_prefix = prefix;
                    route.upstream_host = target.substr(0, colon);
                    route.upstream_port = static_cast<uint16_t>(std::stoi(target.substr(colon + 1)));
                    config.routes.push_back(route);
                }
            }
        } else if (key == "request_timeout") {
            config.request_timeout = std::stoi(value);
        } else if (key == "max_request_size") {
            config.max_request_size = std::stoul(value);
        } else if (key == "rate_limit") {
            config.rate_limit = std::stoi(value);
        } else if (key == "benchmark_mode") {
            config.benchmark_mode = (value == "true" || value == "1");
        } else if (key == "benchmark_duration") {
            config.benchmark_duration = std::stoi(value);
        } else if (key == "benchmark_connections") {
            config.benchmark_connections = std::stoi(value);
        }
    }
    
    return true;
}

bool ConfigServer::load_dari_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string isi = buffer.str();
    
    return ParserConfig::parse(isi, *this);
}

}
