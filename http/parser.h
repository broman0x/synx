#ifndef HTTP_PARSER_H
#define HTTP_PARSER_H

#include <string>
#include <unordered_map>
#include <cstdint>

namespace synx {

enum class StatusParse {
    BELUM_SELESAI,
    SELESAI,
    ERROR
};

struct RequestHTTP {
    std::string method;
    std::string path;
    std::string versi;
    std::unordered_map<std::string, std::string> header;
    std::string body;
    std::string host;
    
    bool is_keep_alive() const;
    std::string get_header(const std::string& key) const;
};

class ParserHTTP {
public:
    ParserHTTP();
    
    void reset();
    StatusParse parse(const char* data, size_t panjang, RequestHTTP& request);
    
    const std::string& get_buffer() const { return buffer_; }
    void clear_buffer();
    
private:
    std::string buffer_;
    size_t header_end_pos_;
    bool header_parsed_;
    
    StatusParse parse_line(const std::string& line, RequestHTTP& request);
    bool parse_request_line(const std::string& line, RequestHTTP& request);
    bool parse_header_line(const std::string& line, RequestHTTP& request);
};

struct ResponseHTTP {
    int kode_status;
    std::string status_text;
    std::unordered_map<std::string, std::string> header;
    std::string body;
    std::string konten_file;
    
    void set_status(int code);
    void add_header(const std::string& key, const std::string& value);
    std::string build() const;
};

std::string detect_mime_type(const std::string& path);

}

#endif
