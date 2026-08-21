#!/usr/bin/env bash
set -euo pipefail

if ! command -v docker >/dev/null 2>&1; then
  echo "Docker is required. Install Docker Engine or Docker Desktop first." >&2
  exit 1
fi

if ! docker compose version >/dev/null 2>&1; then
  echo "Docker Compose v2 is required." >&2
  exit 1
fi

if [ ! -f .env ]; then
  if [ -f .env.example ]; then
    cp .env.example .env
    echo "Created .env from .env.example. Review required values before deployment."
  else
    echo "Missing .env and .env.example." >&2
    exit 1
  fi
fi

echo "Validating compose configuration..."
docker compose config -q

echo "Fetching declared container/build dependencies..."
docker compose pull

docker compose build

echo "Bootstrap complete. Start with: docker compose up -d"
