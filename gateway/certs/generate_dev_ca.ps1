param(
    [string]$CertDir = "D:\codex\projectB\gateway\certs"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

New-Item -ItemType Directory -Path $CertDir -Force | Out-Null
Push-Location $CertDir

Remove-Item -Force -ErrorAction SilentlyContinue ca.key,ca.crt,ca.srl,server.key,server.csr,server.crt,client.key,client.csr,client.crt

openssl genrsa -out ca.key 2048 | Out-Null
openssl req -x509 -new -nodes -key ca.key -sha256 -days 3650 -subj "/CN=ProjectB-Dev-CA" -out ca.crt | Out-Null

openssl genrsa -out server.key 2048 | Out-Null
openssl req -new -key server.key -subj "/CN=projectb-gateway.local" -out server.csr | Out-Null
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 825 -sha256 | Out-Null

openssl genrsa -out client.key 2048 | Out-Null
openssl req -new -key client.key -subj "/CN=projectb-client" -out client.csr | Out-Null
openssl x509 -req -in client.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out client.crt -days 825 -sha256 | Out-Null

Pop-Location
Write-Host "dev certificates generated: $CertDir"
