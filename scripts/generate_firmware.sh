#!/usr/bin/env bash
set -e

FIRMWARE_DIR="firmware"
KEY_DIR="scripts/keys"
mkdir -p "$FIRMWARE_DIR"
mkdir -p "$KEY_DIR"

cat > "$FIRMWARE_DIR/ecu_firmware.bin" <<'EOF'
ECU firmware simulation payload
EOF

openssl genpkey -algorithm RSA -pkeyopt rsa_keygen_bits:2048 -out "$KEY_DIR/ecu_private.pem"
openssl rsa -in "$KEY_DIR/ecu_private.pem" -pubout -out "$KEY_DIR/ecu_public.pem"

openssl dgst -sha256 -sign "$KEY_DIR/ecu_private.pem" -out "$FIRMWARE_DIR/ecu_firmware.sig" "$FIRMWARE_DIR/ecu_firmware.bin"

echo "Generated firmware binary and signature."
echo "Public key: $KEY_DIR/ecu_public.pem"
echo "Firmware: $FIRMWARE_DIR/ecu_firmware.bin"
echo "Signature: $FIRMWARE_DIR/ecu_firmware.sig"