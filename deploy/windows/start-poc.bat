@echo off
setlocal
cd /d "%~dp0..\.."
if not exist .env copy .env.example .env
where docker >nul 2>&1
if errorlevel 1 (
  echo Docker is required. Install Docker Desktop and try again.
  exit /b 1
)
docker compose -f docker-compose.poc.yml up -d
if errorlevel 1 exit /b 1
echo Jayalakshmi DRIVE PoC infrastructure is running.
docker compose -f docker-compose.poc.yml ps
endlocal
