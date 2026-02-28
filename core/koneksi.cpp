#include "koneksi.h"
#include <unistd.h>
#include <sys/stat.h>
#include <cstring>
#include <chrono>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace synx {

KoneksiAktif::KoneksiAktif()
    : fd(-1)
    , status(StatusKoneksi::NUNGGU_BACA)
    , is_https(false)
    , buffer_baca(8192)
    , panjang_data_baca(0)
    , buffer_kirim(8192)
    , posisi_kirim(0)
    , panjang_kirim(0)
    , waktu_terakhir_aktif(0)
    , bytes_dikirim(0)
    , request_count(0)
{
    auto now = std::chrono::steady_clock::now();
    waktu_terakhir_aktif = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}

void KoneksiAktif::reset() {
    fd = -1;
    ip_client.clear();
    status = StatusKoneksi::NUNGGU_BACA;
    panjang_data_baca = 0;
    posisi_kirim = 0;
    panjang_kirim = 0;
    parser_http.reset();
    request = RequestHTTP();
    response = ResponseHTTP();
    file_diminta.clear();
    path_lengkap.clear();
    bytes_dikirim = 0;
    
    auto now = std::chrono::steady_clock::now();
    waktu_terakhir_aktif = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()).count();
}

void KoneksiAktif::tutup() {
    if (fd >= 0) {
        ::close(fd);
        fd = -1;
    }
}

bool KoneksiAktif::is_keep_alive() const {
    return request.is_keep_alive();
}

std::string HandlerRequest::sanitize_path(const std::string& path) {
    std::string result = path;
    
    size_t query = result.find('?');
    if (query != std::string::npos) {
        result = result.substr(0, query);
    }
    
    std::string decoded;
    for (size_t i = 0; i < result.size(); ++i) {
        if (result[i] == '%' && i + 2 < result.size()) {
            int hex;
            std::istringstream iss(result.substr(i + 1, 2));
            if (iss >> std::hex >> hex) {
                decoded += static_cast<char>(hex);
                i += 2;
                continue;
            }
        }
        decoded += result[i];
    }
    
    if (decoded.find("..") != std::string::npos) {
        return "";
    }
    
    return decoded;
}

bool HandlerRequest::is_directory(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) != 0) {
        return false;
    }
    return S_ISDIR(st.st_mode);
}

bool HandlerRequest::cek_file_index(const std::string& dir_path, std::string& index_file) {
    const char* index_names[] = {
        "index.html", "index.htm", "default.html", 
        "index.php", "index.js", "index.py", "index.rb", "index.pl"
    };
    
    for (const char* name : index_names) {
        std::string full_path = dir_path + "/" + name;
        struct stat st;
        if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode)) {
            index_file = full_path;
            return true;
        }
    }
    return false;
}

bool HandlerRequest::baca_file(const std::string& path, std::string& konten) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    konten = buffer.str();
    
    return !file.bad();
}

void HandlerRequest::buat_respon_200(KoneksiAktif& koneksi, const std::string& konten,
                                      const std::string& mime_type) {
    koneksi.response.set_status(200);
    koneksi.response.add_header("Content-Type", mime_type);
    koneksi.response.add_header("Content-Length", std::to_string(konten.size()));
    koneksi.response.add_header("Connection", koneksi.is_keep_alive() ? "keep-alive" : "close");
    koneksi.response.add_header("Server", "synx/1.0");
    koneksi.response.body = konten;
    
    std::string resp_str = koneksi.response.build();
    koneksi.panjang_kirim = resp_str.size();
    
    if (koneksi.buffer_kirim.size() < koneksi.panjang_kirim) {
        koneksi.buffer_kirim.resize(koneksi.panjang_kirim);
    }
    
    std::memcpy(koneksi.buffer_kirim.data(), resp_str.c_str(), koneksi.panjang_kirim);
    koneksi.posisi_kirim = 0;
    koneksi.status = StatusKoneksi::SIAP_KIRIM;
}

void HandlerRequest::buat_respon_404(KoneksiAktif& koneksi) {
    buat_respon_error(koneksi, 404, "Not Found");
}

void HandlerRequest::buat_respon_error(KoneksiAktif& koneksi, int kode, const std::string& text) {
    std::string body = 
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>" + std::to_string(kode) + " " + text + "</title>\n"
        "    <style>\n"
        "        :root { --bg: #0a0a0a; --text: #ededed; --accent: #555; }\n"
        "        body { background-color: var(--bg); color: var(--text); font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif; display: flex; flex-direction: column; align-items: center; justify-content: center; height: 100vh; margin: 0; text-align: center; }\n"
        "        h1 { font-size: 5rem; font-weight: 600; margin: 0; letter-spacing: -0.05em; line-height: 1; }\n"
        "        p { font-size: 1.25rem; color: var(--accent); margin-top: 1rem; font-weight: 400; }\n"
        "        .divider { width: 40px; height: 4px; background: var(--text); margin: 2rem auto; border-radius: 2px; }\n"
        "        footer { position: absolute; bottom: 2rem; font-size: 0.8rem; color: #444; font-family: monospace; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <h1>" + std::to_string(kode) + "</h1>\n"
        "    <p>" + text + "</p>\n"
        "    <div class=\"divider\"></div>\n"
        "    <footer>synx web server</footer>\n"
        "</body>\n"
        "</html>";
    
    koneksi.response.set_status(kode);
    koneksi.response.add_header("Content-Type", "text/html");
    koneksi.response.add_header("Content-Length", std::to_string(body.size()));
    koneksi.response.add_header("Connection", "close");
    koneksi.response.add_header("Server", "synx/1.0");
    koneksi.response.body = body;
    
    std::string resp_str = koneksi.response.build();
    koneksi.panjang_kirim = resp_str.size();
    
    if (koneksi.buffer_kirim.size() < koneksi.panjang_kirim) {
        koneksi.buffer_kirim.resize(koneksi.panjang_kirim);
    }
    
    std::memcpy(koneksi.buffer_kirim.data(), resp_str.c_str(), koneksi.panjang_kirim);
    koneksi.posisi_kirim = 0;
    koneksi.status = StatusKoneksi::SIAP_KIRIM;
}

bool HandlerRequest::proses_request(KoneksiAktif& koneksi, const std::string& root_dir) {
    std::string clean_path = sanitize_path(koneksi.request.path);
    if (clean_path.empty()) {
        buat_respon_error(koneksi, 400, "Bad Request");
        return false;
    }
    
    std::string full_path = root_dir + clean_path;
    
    if (is_directory(full_path)) {
        std::string index_file;
        if (cek_file_index(full_path, index_file)) {
            full_path = index_file;
        } else {
            buat_respon_error(koneksi, 403, "Forbidden");
            return false;
        }
    }
    
    size_t dot_pos = full_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        std::string ext = full_path.substr(dot_pos);
        std::transform(ext.begin(), ext.end(), ext.begin(), 
                       [](unsigned char c) { return std::tolower(c); });
                       
        if (ext == ".php") {
            std::string cmd = "REQUEST_METHOD=" + koneksi.request.method + 
                              " REMOTE_ADDR=" + koneksi.ip_client + 
                              " php -f " + full_path + " < /dev/null 2>/dev/null";
                              
            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                buat_respon_error(koneksi, 500, "Internal Server Error - CGI Failed");
                return false;
            }
            
            char buffer[4096];
            std::string result = "";
            while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
                result += buffer;
            }
            pclose(pipe);
            
            buat_respon_200(koneksi, result, "text/html");
            return true;
        }
                       
        if (ext == ".py" || ext == ".rb" || ext == ".pl" || 
            ext == ".env" || ext == ".git" || ext == ".ht") {
            buat_respon_error(koneksi, 403, "Forbidden");
            return false;
        }
    }
    
    std::string konten;
    if (!baca_file(full_path, konten)) {
        buat_respon_404(koneksi);
        return false;
    }
    
    std::string mime = detect_mime_type(full_path);
    
    buat_respon_200(koneksi, konten, mime);
    return true;
}

}
