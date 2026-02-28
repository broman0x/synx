#!/bin/bash

set -e

SYNX_VERSION="0.1.0"
INSTALL_DIR="/usr/local/bin"
UNINSTALL_SCRIPT="/usr/local/bin/synx-uninstall"

echo "========================================"
echo "  synx Quick Install via curl"
echo "========================================"

if [ "$EUID" -ne 0 ]; then
    echo "[ERROR] Please run as root (sudo ./install-curl.sh)"
    exit 1
fi

echo "[INFO] Detecting architecture..."
ARCH=$(uname -m)
if [ "$ARCH" = "x86_64" ]; then
    ARCH_SUFFIX="x86_64"
elif [ "$ARCH" = "aarch64" ]; then
    ARCH_SUFFIX="aarch64"
else
    echo "[WARN] Architecture $ARCH not officially supported, trying x86_64 binary..."
    ARCH_SUFFIX="x86_64"
fi

echo "[INFO] Architecture: $ARCH ($ARCH_SUFFIX)"

TEMP_DIR=$(mktemp -d)
cd ${TEMP_DIR}

echo "[1/4] Downloading synx from GitHub Releases..."
DOWNLOAD_URL="https://github.com/broman0x/synx/releases/download/v${SYNX_VERSION}/synx-${SYNX_VERSION}-linux-${ARCH_SUFFIX}.tar.gz"

if curl -sSf -L -o synx.tar.gz "$DOWNLOAD_URL"; then
    echo "[INFO] Download successful from GitHub Releases"
else
    echo "[WARN] Failed to download from Releases, falling back to source build..."
    rm -rf ${TEMP_DIR}/*
    
    apt-get update -qq
    apt-get install -y -qq g++ make libssl-dev git
    
    git clone --depth 1 https://github.com/broman0x/synx.git
    cd synx
    make https
    
    echo "[3/4] Installing..."
    cp synx ${INSTALL_DIR}/synx
    chmod +x ${INSTALL_DIR}/synx
    
    cd /
    rm -rf ${TEMP_DIR}
    
    echo ""
    echo "========================================"
    echo "  Installation complete! (from source)"
    echo "========================================"
    echo ""
    echo "Usage: synx [config_file]"
    echo "Example: synx synx.conf"
    echo ""
    echo "Uninstall: sudo synx-uninstall"
    echo ""
    echo "Test: curl http://localhost:8080/"
    echo ""
    exit 0
fi

echo "[2/4] Extracting..."
tar -xzf synx.tar.gz

echo "[3/4] Installing..."
cp synx ${INSTALL_DIR}/synx
chmod +x ${INSTALL_DIR}/synx

cp synx.conf /etc/synx.conf.example

cat > ${UNINSTALL_SCRIPT} << 'EOF'
#!/bin/bash

echo "========================================"
echo "  synx Web Server Uninstaller"
echo "========================================"

if [ "$EUID" -ne 0 ]; then
    echo "[ERROR] Please run as root (sudo synx-uninstall)"
    exit 1
fi

echo "[1/4] Stopping synx..."
systemctl stop synx 2>/dev/null || true
systemctl disable synx 2>/dev/null || true
pkill -f "synx" 2>/dev/null || true

echo "[2/4] Removing binary..."
rm -f /usr/local/bin/synx
rm -f /usr/local/bin/synx-uninstall

echo "[3/4] Removing config files..."
rm -f /etc/synx.conf
rm -f /etc/synx.conf.example

echo "[4/4] Removing systemd service..."
rm -f /etc/systemd/system/synx.service
systemctl daemon-reload 2>/dev/null || true

echo ""
echo "========================================"
echo "  Uninstallation complete!"
echo "========================================"
echo ""
echo "Note: Your web root (/var/www/synx or custom) is preserved."
echo "Remove manually: sudo rm -rf /var/www/synx"
echo ""
EOF

chmod +x ${UNINSTALL_SCRIPT}

echo "[4/4] Cleaning up..."
cd /
rm -rf ${TEMP_DIR}

echo ""
echo "========================================"
echo "  Installation complete!"
echo "========================================"
echo ""
echo "Usage: synx [config_file]"
echo "Example: synx synx.conf"
echo ""
echo "Uninstall: sudo synx-uninstall"
echo ""
echo "Test: curl http://localhost:8080/"
echo ""
