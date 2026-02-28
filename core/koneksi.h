#ifndef KONEKSI_H
#define KONEKSI_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "../http/parser.h"

#ifdef ENABLE_HTTPS
#include "../utils/ssl.h"
#endif

namespace synx {

enum class StatusKoneksi {
    NUNGGU_BACA,
    LAGI_PROSES,
    SIAP_KIRIM,
    SELESAI
};

struct KoneksiAktif {
    int fd;
    std::string ip_client;
    StatusKoneksi status;
    bool is_https;
    
    std::vector<char> buffer_baca;
    size_t panjang_data_baca;
    
    std::vector<char> buffer_kirim;
    size_t posisi_kirim;
    size_t panjang_kirim;
    
    ParserHTTP parser_http;
    RequestHTTP request;
    ResponseHTTP response;
    
    std::string file_diminta;
    std::string path_lengkap;
    
    int64_t waktu_terakhir_aktif;
    size_t bytes_dikirim;
    int request_count;
    
#ifdef ENABLE_HTTPS
    std::unique_ptr<SSLConnection> ssl_conn;
#endif
    
    KoneksiAktif();
    void reset();
    void tutup();
    bool is_keep_alive() const;
};

class HandlerRequest {
public:
    static bool proses_request(KoneksiAktif& koneksi, const std::string& root_dir);
    static bool baca_file(const std::string& path, std::string& konten);
    static void buat_respon_200(KoneksiAktif& koneksi, const std::string& konten, 
                                 const std::string& mime_type);
    static void buat_respon_404(KoneksiAktif& koneksi);
    static void buat_respon_error(KoneksiAktif& koneksi, int kode, const std::string& text);
    static std::string sanitize_path(const std::string& path);
    static bool is_directory(const std::string& path);
    static bool cek_file_index(const std::string& dir_path, std::string& index_file);
};

}

#endif
