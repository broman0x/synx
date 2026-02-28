#include "logger.h"
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace synx {

static Logger g_logger;

Logger::Logger() : initialized_(false), request_count_(0), error_count_(0) {}

Logger::~Logger() {
    flush();
    if (file_log_.is_open()) {
        file_log_.close();
    }
}

bool Logger::init(const std::string& file_path) {
    std::lock_guard<std::mutex> lock(mutex_);

    file_log_.open(file_path, std::ios::app);
    if (!file_log_.is_open()) {
        std::cerr << "[ERROR] Gagal buka file log: " << file_path << std::endl;
        return false;
    }
    initialized_ = true;
    return true;
}

std::string Logger::get_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::tm tm_now;
    localtime_r(&time_t_now, &tm_now);
    
    std::ostringstream oss;
    oss << std::put_time(&tm_now, "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return oss.str();
}

std::string Logger::get_level_str(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG:   return "DEBUG";
        case LogLevel::INFO:    return "INFO";
        case LogLevel::WARNING: return "WARN";
        case LogLevel::ERROR:   return "ERROR";
        default:                return "UNKNOWN";
    }
}

std::string Logger::get_level_color(LogLevel level) {
    if (!support_color()) return "";
    
    switch (level) {
        case LogLevel::DEBUG:   return "\033[36m"; 
        case LogLevel::INFO:    return "\033[32m"; 
        case LogLevel::WARNING: return "\033[33m"; 
        case LogLevel::ERROR:   return "\033[31m";
        default:                return "\033[0m";
    }
}

bool Logger::support_color() {
    const char* term = std::getenv("TERM");
    return term && std::string(term) != "dumb";
}

void Logger::log(LogLevel level, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string timestamp = get_timestamp();
    std::string level_str = get_level_str(level);
    std::string color = get_level_color(level);
    std::string reset = support_color() ? "\033[0m" : "";
    
    std::string log_line = "[" + timestamp + "] " + color + "[" + level_str + "]\033[0m " + msg;
    
    std::cout << log_line << std::endl;
    
    if (file_log_.is_open()) {
        std::string file_line = "[" + timestamp + "] [" + level_str + "] " + msg;
        file_log_ << file_line << std::endl;
        file_log_.flush();
    }
}

void Logger::debug(const std::string& msg) {
    log(LogLevel::DEBUG, msg);
}

void Logger::info(const std::string& msg) {
    log(LogLevel::INFO, msg);
}

void Logger::warning(const std::string& msg) {
    log(LogLevel::WARNING, msg);
}

void Logger::error(const std::string& msg) {
    log(LogLevel::ERROR, msg);
}

void Logger::tulis_access_log(const std::string& ip, const std::string& method,
                               const std::string& path, int kode_status, size_t bytes_sent) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!file_log_.is_open()) return;

    auto timestamp = get_timestamp();
    
    std::string color_status;
    std::string reset = "";
    if (support_color()) {
        reset = "\033[0m";
        if (kode_status >= 500) color_status = "\033[31m"; 
        else if (kode_status >= 400) color_status = "\033[33m"; 
        else if (kode_status >= 300) color_status = "\033[36m"; 
        else color_status = "\033[32m"; 
    }
    
    file_log_ << "[" << timestamp << "] " << ip << " \"" 
              << method << " " << path << "\" "
              << color_status << kode_status << reset 
              << " " << bytes_sent << " bytes" << std::endl;
    file_log_.flush();
    
    increment_requests();
}

void Logger::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (file_log_.is_open()) {
        file_log_.flush();
    }
}

void Logger::increment_requests() {
    request_count_++;
}

void Logger::increment_errors() {
    error_count_++;
}

size_t Logger::get_request_count() const {
    return request_count_.load();
}

size_t Logger::get_error_count() const {
    return error_count_.load();
}

Logger& get_logger() {
    return g_logger;
}

}
