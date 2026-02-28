#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/resource.h>
#include "server/server.h"
#include "config/config.h"
#include "utils/logger.h"

using namespace synx;

static Server* g_server = nullptr;

void signal_handler(int signum) {
    if (g_server) {
        g_server->stop();
    }
    (void)signum;
}

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [options] [config_file]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -h, --help      Show this help" << std::endl;
    std::cout << "  -p, --port      Override port" << std::endl;
    std::cout << "  -w, --workers   Override worker count" << std::endl;
    std::cout << "  -b, --benchmark Enable benchmark mode" << std::endl;
    std::cout << "  -v, --version   Show version" << std::endl;
}

void set_resource_limits() {
    struct rlimit rl;
    rl.rlim_cur = 65536;
    rl.rlim_max = 65536;
    setrlimit(RLIMIT_NOFILE, &rl);
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "  synx Web Server v0.1" << std::endl;
    std::cout << "========================================" << std::endl;
    
    std::string config_path = "synx.conf";
    int override_port = 0;
    int override_workers = 0;
    bool benchmark_mode = false;
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            std::cout << "synx v0.1.0" << std::endl;
            return 0;
        } else if (arg == "-p" || arg == "--port") {
            if (i + 1 < argc) {
                override_port = std::stoi(argv[++i]);
            }
        } else if (arg == "-w" || arg == "--workers") {
            if (i + 1 < argc) {
                override_workers = std::stoi(argv[++i]);
            }
        } else if (arg == "-b" || arg == "--benchmark") {
            benchmark_mode = true;
        } else if (arg[0] != '-') {
            config_path = arg;
        }
    }
    
    ConfigServer config;
    if (!config.load_dari_file(config_path)) {
        std::cerr << "[WARNING] Gagal load config, pakai default" << std::endl;
    } else {
        std::cout << "[INFO] Config loaded dari " << config_path << std::endl;
    }
    
    if (override_port > 0) {
        config.port = static_cast<uint16_t>(override_port);
    }
    if (override_workers > 0) {
        config.worker_count = override_workers;
    }
    if (benchmark_mode) {
        config.benchmark_mode = true;
    }
    
    set_resource_limits();
    
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    
    Server server;
    g_server = &server;
    
    if (!server.init(config)) {
        std::cerr << "[ERROR] Gagal init server" << std::endl;
        return 1;
    }
    
    std::cout << "[INFO] Starting server..." << std::endl;
    server.run();
    
    std::cout << "[INFO] Server stopped" << std::endl;
    std::cout << "[INFO] Total requests served: " << get_logger().get_request_count() << std::endl;
    std::cout << "[INFO] Total errors: " << get_logger().get_error_count() << std::endl;
    
    g_server = nullptr;
    
    return 0;
}
