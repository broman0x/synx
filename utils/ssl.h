#ifndef SSL_H
#define SSL_H

#include <string>

#ifdef ENABLE_HTTPS

#include <openssl/ssl.h>
#include <openssl/err.h>

namespace synx {

class SSLContext {
public:
    SSLContext();
    ~SSLContext();
    
    bool init();
    bool load_cert(const std::string& cert_path, const std::string& key_path);
    SSL_CTX* get_ctx() const { return ctx_; }
    
    static void init_openssl();
    static void cleanup_openssl();
    
private:
    SSL_CTX* ctx_;
};

class SSLConnection {
public:
    SSLConnection(SSL_CTX* ctx, int fd);
    ~SSLConnection();
    
    int handshake();
    int read(char* buf, int len);
    int write(const char* buf, int len);
    int shutdown();
    
    SSL* get_ssl() const { return ssl_; }
    bool is_secure() const { return ssl_ != nullptr; }
    
private:
    SSL* ssl_;
};

std::string get_ssl_error();

}

#else

namespace synx {

class SSLContext {
public:
    bool init() { return false; }
    bool load_cert(const std::string&, const std::string&) { return false; }
    void* get_ctx() const { return nullptr; }
    static void init_openssl() {}
    static void cleanup_openssl() {}
};

class SSLConnection {
public:
    SSLConnection(void*, int) : ssl_(nullptr) {}
    ~SSLConnection() {}
    int handshake() { return -1; }
    int read(char*, int) { return -1; }
    int write(const char*, int) { return -1; }
    int shutdown() { return -1; }
    void* get_ssl() const { return nullptr; }
    bool is_secure() const { return false; }
private:
    void* ssl_;
};

}

#endif

#endif
