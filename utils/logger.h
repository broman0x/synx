#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <atomic>

namespace synx {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    Logger();
    ~Logger();

    bool init(const std::string& file_path);
    
    void log(LogLevel level, const std::string& msg);
    void debug(const std::string& msg);
    void info(const std::string& msg);
    void warning(const std::string& msg);
    void error(const std::string& msg);
    
    void tulis_access_log(const std::string& ip, const std::string& method,
                          const std::string& path, int kode_status, size_t bytes_sent);
    void flush();
    
    void increment_requests();
    void increment_errors();
    size_t get_request_count() const;
    size_t get_error_count() const;

private:
    std::ofstream file_log_;
    std::mutex mutex_;
    bool initialized_;
    std::atomic<size_t> request_count_;
    std::atomic<size_t> error_count_;
    
    std::string get_timestamp();
    std::string get_level_str(LogLevel level);
    std::string get_level_color(LogLevel level);
    bool support_color();
};

Logger& get_logger();

}

#endif
