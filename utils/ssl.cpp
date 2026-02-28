#include "ssl.h"

#ifdef ENABLE_HTTPS

#include <cstring>

namespace synx {

void SSLContext::init_openssl() {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();
}

void SSLContext::cleanup_openssl() {
    EVP_cleanup();
    ERR_free_strings();
}

SSLContext::SSLContext() : ctx_(nullptr) {}

SSLContext::~SSLContext() {
    if (ctx_) {
        SSL_CTX_free(ctx_);
    }
}

bool SSLContext::init() {
    const SSL_METHOD* method = TLS_server_method();
    ctx_ = SSL_CTX_new(method);
    
    if (!ctx_) {
        return false;
    }
    
    SSL_CTX_set_min_proto_version(ctx_, TLS1_2_VERSION);
    
    return true;
}

bool SSLContext::load_cert(const std::string& cert_path, const std::string& key_path) {
    if (!ctx_) {
        return false;
    }
    
    if (SSL_CTX_use_certificate_file(ctx_, cert_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx_, key_path.c_str(), SSL_FILETYPE_PEM) <= 0) {
        return false;
    }
    
    if (!SSL_CTX_check_private_key(ctx_)) {
        return false;
    }
    
    return true;
}

SSLConnection::SSLConnection(SSL_CTX* ctx, int fd) : ssl_(nullptr) {
    if (!ctx || fd < 0) {
        return;
    }
    
    ssl_ = SSL_new(ctx);
    if (!ssl_) {
        return;
    }
    
    SSL_set_fd(ssl_, fd);
    SSL_set_accept_state(ssl_);
}

SSLConnection::~SSLConnection() {
    if (ssl_) {
        SSL_shutdown(ssl_);
        SSL_free(ssl_);
    }
}

int SSLConnection::handshake() {
    if (!ssl_) {
        return -1;
    }
    
    int ret = SSL_accept(ssl_);
    if (ret <= 0) {
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            return 0;
        }
        return -1;
    }
    
    return 1;
}

int SSLConnection::read(char* buf, int len) {
    if (!ssl_) {
        return -1;
    }
    
    int ret = SSL_read(ssl_, buf, len);
    if (ret <= 0) {
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            errno = EAGAIN;
            return -1;
        }
        return -1;
    }
    
    return ret;
}

int SSLConnection::write(const char* buf, int len) {
    if (!ssl_) {
        return -1;
    }
    
    int ret = SSL_write(ssl_, buf, len);
    if (ret <= 0) {
        int err = SSL_get_error(ssl_, ret);
        if (err == SSL_ERROR_WANT_READ || err == SSL_ERROR_WANT_WRITE) {
            errno = EAGAIN;
            return -1;
        }
        return -1;
    }
    
    return ret;
}

int SSLConnection::shutdown() {
    if (!ssl_) {
        return -1;
    }
    
    return SSL_shutdown(ssl_);
}

std::string get_ssl_error() {
    char buf[256];
    ERR_error_string_n(ERR_get_error(), buf, sizeof(buf));
    return std::string(buf);
}

}

#endif
