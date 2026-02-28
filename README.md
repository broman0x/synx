<div align="center">
  <img src="public/logo.png" alt="synx logo" width="300" />
</div>

# synx
v0.1.0

Web server stabil dan efisien yang ditulis dengan C++17.

## Ringkasan

Synx menggunakan Linux epoll untuk menangani koneksi secara bersamaan dengan efisien. Server ini memiliki arsitektur *multi-worker* berbasis *fork* untuk memaksimalkan penggunaan multi-core. Semua *socket* menggunakan mode *non-blocking* dengan epoll.

## Fitur

- Penjadwalan I/O menggunakan Linux epoll
- Model *Multi-Worker* berbasis *fork* untuk skalabilitas multi-core
- *Non-Blocking I/O* pada semua *socket*
- Dukungan HTTP/1.1 termasuk *keep-alive* dan *chunked transfer encoding*
- Integrasi HTTPS/TLS menggunakan OpenSSL
- Kemampuan *Reverse Proxy*
- *Rate Limiting*
- *Security Hardening*
- Mode *Benchmark* bawaan
- Keamanan memori dengan dukungan integrasi AddressSanitizer

## Instalasi

### Dari *Source Code*

```bash
git clone https://github.com/broman0x/synx.git
cd synx
make https
sudo make install
```

### Instalasi Cepat

```bash
curl -fsSL https://raw.githubusercontent.com/broman0x/synx/main/scripts/install-curl.sh | sudo bash
```

### Uninstalasi

```bash
sudo synx-uninstall
```

## Konfigurasi

Buat file `synx.conf`:

```ini
port = 8080
root = ./public
log = access.log
max_connections = 1024
worker_count = 4

rate_limit = 100
max_request_size = 1048576
request_timeout = 30

https_enable = true
https_port = 8443
https_cert = cert.crt
https_key = cert.key

reverse_proxy = false
upstream = 127.0.0.1:3000
upstream = 127.0.0.1:3001
upstream = 127.0.0.1:3002@2

route = /api/ 127.0.0.1:4000
route = /admin/ 127.0.0.1:5000
```

## Penggunaan

Menjalankan server:

```bash
./build/synx
```

Menjalankan server dengan konfigurasi kustom:

```bash
./build/synx custom.conf
```

Mengesampingkan port yang digunakan:

```bash
./build/synx -p 3000
```

Menentukan jumlah proses *worker*:

```bash
./build/synx -w 8
```

Menjalankan *benchmark* bawaan:

```bash
./build/synx -b
```

## Instruksi Build

*Build* standar:
```bash
make
```

*Build* dengan dukungan HTTPS:
```bash
make https
```

*Build* dengan Address/Leak Sanitizer:
```bash
make debug
```

Menjalankan pengujian (*tests*):
```bash
make test
```


## Lisensi

Proyek ini dilisensikan di bawah Lisensi GPL. Lihat file LICENSE untuk penjelasan selengkapnya.