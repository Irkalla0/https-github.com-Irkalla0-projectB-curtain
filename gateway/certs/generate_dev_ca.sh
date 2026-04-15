#!/usr/bin/env bash
set -euo pipefail

CERT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${CERT_DIR}"

rm -f ca.key ca.crt server.key server.csr server.crt client.key client.csr client.crt

openssl genrsa -out ca.key 2048
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -subj "/CN=ProjectB-Dev-CA" -out ca.crt

openssl genrsa -out server.key 2048
openssl req -new -key server.key -subj "/CN=projectb-gateway.local" -out server.csr
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 825 -sha256

openssl genrsa -out client.key 2048
openssl req -new -key client.key -subj "/CN=projectb-client" -out client.csr
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 825 -sha256

echo "dev certificates generated in ${CERT_DIR}"
