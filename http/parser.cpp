#include "parser.h"
#include <algorithm>
#include <cctype>
#include <sstream>

namespace synx {

bool RequestHTTP::is_keep_alive() const {
    auto it = header.find("Connection");
    if (it != header.end()) {
        return it->second == "keep-alive";
    }
    return versi == "HTTP/1.1";
}

std::string RequestHTTP::get_header(const std::string& key) const {
    auto it = header.find(key);
    if (it != header.end()) {
        return it->second;
    }
    return "";
}

ParserHTTP::ParserHTTP() : header_end_pos_(0), header_parsed_(false) {}

void ParserHTTP::reset() {
    buffer_.clear();
    header_end_pos_ = 0;
    header_parsed_ = false;
}

StatusParse ParserHTTP::parse(const char* data, size_t panjang, RequestHTTP& request) {
    buffer_.append(data, panjang);
    
    size_t pos = buffer_.find("\r\n\r\n");
    if (pos == std::string::npos) {
        if (buffer_.size() > 8192) {
            return StatusParse::ERROR;
        }
        return StatusParse::BELUM_SELESAI;
    }
    
    header_end_pos_ = pos + 4;
    
    std::istringstream stream(buffer_.substr(0, pos));
    std::string line;
    bool first_line = true;
    
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        if (first_line) {
            if (!parse_request_line(line, request)) {
                return StatusParse::ERROR;
            }
            first_line = false;
        } else {
            if (!parse_header_line(line, request)) {
                return StatusParse::ERROR;
            }
        }
    }
    
    auto it = request.header.find("Content-Length");
    if (it != request.header.end()) {
        size_t content_length = std::stoul(it->second);
        size_t body_start = header_end_pos_;
        size_t body_available = buffer_.size() - body_start;
        
        if (body_available < content_length) {
            return StatusParse::BELUM_SELESAI;
        }
        request.body = buffer_.substr(body_start, content_length);
    }
    
    return StatusParse::SELESAI;
}

bool ParserHTTP::parse_request_line(const std::string& line, RequestHTTP& request) {
    size_t sp1 = line.find(' ');
    if (sp1 == std::string::npos) return false;
    
    size_t sp2 = line.find(' ', sp1 + 1);
    if (sp2 == std::string::npos) return false;
    
    request.method = line.substr(0, sp1);
    request.path = line.substr(sp1 + 1, sp2 - sp1 - 1);
    request.versi = line.substr(sp2 + 1);
    
    if (request.method != "GET" && request.method != "HEAD" && request.method != "POST") {
        return false;
    }
    
    if (request.path.empty() || request.path[0] != '/') {
        return false;
    }
    
    return true;
}

bool ParserHTTP::parse_header_line(const std::string& line, RequestHTTP& request) {
    if (line.empty()) return true;
    
    size_t colon = line.find(':');
    if (colon == std::string::npos) return false;
    
    std::string key = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    
    while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) {
        key.pop_back();
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front()))) {
        value.erase(0, 1);
    }
    
    for (auto& c : key) {
        c = std::toupper(static_cast<unsigned char>(c));
    }
    
    request.header[key] = value;
    
    if (key == "HOST") {
        request.host = value;
    }
    
    return true;
}

void ParserHTTP::clear_buffer() {
    buffer_.clear();
    header_end_pos_ = 0;
    header_parsed_ = false;
}

void ResponseHTTP::set_status(int code) {
    kode_status = code;
    switch (code) {
        case 200: status_text = "OK"; break;
        case 301: status_text = "Moved Permanently"; break;
        case 304: status_text = "Not Modified"; break;
        case 400: status_text = "Bad Request"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 502: status_text = "Bad Gateway"; break;
        case 503: status_text = "Service Unavailable"; break;
        default:  status_text = "Unknown"; break;
    }
}

void ResponseHTTP::add_header(const std::string& key, const std::string& value) {
    header[key] = value;
}

std::string ResponseHTTP::build() const {
    std::string result;
    
    result += "HTTP/1.1 " + std::to_string(kode_status) + " " + status_text + "\r\n";
    
    for (const auto& h : header) {
        result += h.first + ": " + h.second + "\r\n";
    }
    result += "\r\n";
    
    result += body;
    
    return result;
}

std::string detect_mime_type(const std::string& path) {
    size_t dot = path.rfind('.');
    if (dot == std::string::npos) {
        return "application/octet-stream";
    }
    
    std::string ext = path.substr(dot);
    std::transform(ext.begin(), ext.end(), ext.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    
    if (ext == ".html" || ext == ".htm" || ext == ".php" || ext == ".jsp") return "text/html";
    if (ext == ".py" || ext == ".rb" || ext == ".pl" || ext == ".lua") return "text/plain";
    if (ext == ".css") return "text/css";
    if (ext == ".js") return "application/javascript";
    if (ext == ".json") return "application/json";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".ico") return "image/x-icon";
    if (ext == ".txt") return "text/plain";
    if (ext == ".xml") return "application/xml";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".woff") return "font/woff";
    if (ext == ".woff2") return "font/woff2";
    if (ext == ".ttf") return "font/ttf";
    
    return "application/octet-stream";
}

}
