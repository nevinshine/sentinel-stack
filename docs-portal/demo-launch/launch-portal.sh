#!/bin/bash
set -e

echo "[*] Initializing Sentinel Stack Knowledge Portal - Live Environment..."

# Ensure dependencies are installed
echo "[*] Verifying Node dependencies..."
cd ../
npm install --silent

# Check if Docker is running (optional, for Meilisearch)
if command -v docker &> /dev/null; then
    echo "[*] Checking for Meilisearch container..."
    if ! docker ps | grep -q "meilisearch"; then
        echo "[!] Meilisearch not running. To enable advanced search, run:"
        echo "    docker run -d -p 7700:7700 getmeili/meilisearch:v1.6"
    else
        echo "[+] Meilisearch detected."
    fi
fi

# Run the next.js development server
echo "[+] Starting Next.js portal on http://localhost:3000..."
npm run dev
