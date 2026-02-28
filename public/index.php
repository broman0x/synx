<?php
    $phpVersion = phpversion();
    $serverTime = date('Y-m-d H:i:sP');
    $clientIP = $_SERVER['REMOTE_ADDR'] ?? '127.0.0.1';
    $reqMethod = $_SERVER['REQUEST_METHOD'] ?? 'GET';
?>
<!DOCTYPE html>
<html lang="id">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <meta name="description" content="synx - Web server dengan C++17 dan Linux epoll.">
    <meta name="keywords" content="web server, C++, epoll, reverse proxy, Linux, PHP">
    <meta name="robots" content="index, follow">
    <title>synx - Web Server (PHP Native)</title>
    <link rel="icon" type="image/png" href="/logo.png">
    <link rel="stylesheet" href="style.css">
</head>
<body>
    <div class="layout">
        <header class="header">
            <div class="brand">
                <img src="/logo.png" alt="synx logo" class="logo-img">
                <span class="logo-text">synx</span>
                <span class="badge">v0.1.0</span>
            </div>
            <nav class="nav">
                <a href="https://github.com/broman0x/synx" target="_blank" rel="noopener">GitHub</a>
            </nav>
        </header>

        <main class="main">
            <section class="hero">
                <div class="status-indicator">
                    <div class="pulse"></div>
                    <span>Server aktif di port 8080 (PHP Native Backend)</span>
                </div>
                <h1 class="headline">Web Server C++17</h1>
                <p class="subheadline">Server web yang stabil dan efisien. Halaman ini diproses secara dinamis oleh backend PHP.</p>
                
                <div style="background: rgba(255,255,255,0.05); padding: 1.5rem; border-radius: 8px; border: 1px solid rgba(255,255,255,0.1); margin: 2rem auto; max-width: 600px; text-align: left; display: grid; grid-template-columns: 1fr 1fr; gap: 1rem;">
                    <div>
                        <span style="font-size: 0.75rem; color: #888; text-transform: uppercase;">Waktu Sistem</span>
                        <div style="font-weight: bold; font-family: monospace; color: #fff; margin-top: 0.25rem;"><?= htmlspecialchars($serverTime) ?></div>
                    </div>
                    <div>
                        <span style="font-size: 0.75rem; color: #888; text-transform: uppercase;">IP Client</span>
                        <div style="font-weight: bold; font-family: monospace; color: #fff; margin-top: 0.25rem;"><?= htmlspecialchars($clientIP) ?></div>
                    </div>
                    <div>
                        <span style="font-size: 0.75rem; color: #888; text-transform: uppercase;">Versi PHP</span>
                        <div style="font-weight: bold; font-family: monospace; color: #fff; margin-top: 0.25rem;"><?= htmlspecialchars($phpVersion) ?></div>
                    </div>
                    <div>
                        <span style="font-size: 0.75rem; color: #888; text-transform: uppercase;">Metode Request</span>
                        <div style="font-weight: bold; font-family: monospace; color: #e74c3c; margin-top: 0.25rem;"><?= htmlspecialchars($reqMethod) ?></div>
                    </div>
                </div>

                <div class="actions">
                    <a href="#fitur" class="btn primary">Lihat Fitur</a>
                    <a href="https://github.com/broman0x/synx" class="btn secondary">Dokumentasi</a>
                </div>
            </section>

            <section class="grid-features" id="fitur">
                <div class="feature-cell">
                    <h3>Server Konten Statis</h3>
                    <p>Mengirim file statis langsung ke browser. Cocok untuk website HTML/CSS, framework frontend, dan aset media.</p>
                </div>
                <div class="feature-cell">
                    <h3>Reverse Proxy & Native Backend</h3>
                    <p>Meneruskan request ke aplikasi backend secara Native maupun eksternal seperti Node.js, Python, PHP-FPM, dll.</p>
                </div>
                <div class="feature-cell">
                    <h3>Arsitektur Stabil</h3>
                    <p>Model I/O dengan Linux epoll untuk menangani koneksi secara stabil.</p>
                </div>
                <div class="feature-cell">
                    <h3>Keamanan</h3>
                    <p>Dukungan TLS/SSL, rate limiting, batas koneksi, dan sanitasi path (proteksi 403 / Forbidden file source).</p>
                </div>
                <div class="feature-cell">
                    <h3>Multi-Worker</h3>
                    <p>Arsitektur berbasis fork memanfaatkan semua core CPU untuk throughput maksimal.</p>
                </div>
                <div class="feature-cell">
                    <h3>Keep-Alive</h3>
                    <p>Dukungan HTTP/1.1 dengan persistent connections untuk pengelolaan koneksi jaringan terbaik.</p>
                </div>
            </section>

            <section class="terminal">
                <div class="terminal-header">
                    <div class="terminal-buttons">
                        <span class="terminal-btn terminal-btn-red"></span>
                        <span class="terminal-btn terminal-btn-yellow"></span>
                        <span class="terminal-btn terminal-btn-green"></span>
                    </div>
                    <span class="terminal-title">administrator@synx:~</span>
                </div>
                <div class="terminal-body">
                    <div class="terminal-line">
                        <span class="prompt">$</span> <span class="cmd">curl</span> <span class="arg">-I</span> <span class="url">http://localhost:8080/</span>
                    </div>
                    <div class="terminal-output">
                        <span class="output-line">HTTP/1.1 200 OK</span>
                        <span class="output-line">Server: synx/0.1.0</span>
                        <span class="output-line">Connection: keep-alive</span>
                        <span class="output-line">Content-Type: text/html</span>
                    </div>
                    <div class="terminal-line">
                        <span class="prompt">$</span> <span class="cmd">wrk</span> <span class="arg">-t12 -c400 -d10s</span> <span class="url">http://localhost:8080/</span>
                    </div>
                    <div class="terminal-output">
                        <span class="output-line">Running 10s test @ http://localhost:8080/</span>
                        <span class="output-line">  12 threads and 400 connections</span>
                        <span class="output-line highlight">  Requests/sec: 231,450.55</span>
                        <span class="output-line">  Transfer/sec:     32.50MB</span>
                    </div>
                </div>
            </section>

            <section class="quick-start">
                <h2 class="section-title">Mulai Menggunakan</h2>
                <div class="code-block">
                    <div class="code-header">
                        <span>Instalasi (dari source)</span>
                        <button class="copy-btn" onclick="copyCode()">
                            <span>Salin</span>
                        </button>
                    </div>
                    <pre><code id="install-code">git clone https://github.com/broman0x/synx.git
                        cd synx
                        make
                    ./build/synx</code></pre>
                </div>
            </section>
        </main>

        <footer class="footer">
            <div class="footer-content">
                <div class="footer-brand">
                    <span class="logo-text">synx</span>
                    <p>Web server ditulis dengan C++17.</p>
                </div>
                <div class="footer-links">
                    <div class="footer-column">
                        <h4>Produk</h4>
                        <a href="#fitur">Fitur</a>
                        <a href="#">Dokumentasi</a>
                    </div>
                    <div class="footer-column">
                        <h4>Komunitas</h4>
                        <a href="https://github.com/broman0x/synx" target="_blank">GitHub</a>
                        <a href="https://github.com/broman0x/synx/issues" target="_blank">Issues</a>
                    </div>
                </div>
            </div>
            <div class="footer-bottom">
                <p>2026 synx web server. Open source dengan lisensi GPL.</p>
            </div>
        </footer>
    </div>
    <script src="script.js"></script>
</body>
</html>
