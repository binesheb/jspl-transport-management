@echo off
setlocal
cd /d "%~dp0..\.."
if not exist .env copy .env.example .env
where docker >nul 2>&1
if errorlevel 1 (
  echo Docker is required. Install Docker Desktop and try again.
  exit /b 1
)
docker compose -f docker-compose.poc.yml up -d --build
if errorlevel 1 exit /b 1
echo.
echo Jayalakshmi DRIVE PoC infrastructure is running.
echo API:       http://localhost:8000
 echo Dashboard: http://localhost:8080
echo.
echo Use the laptop IPv4 address on the driver/manager phone.
echo Example:   http://192.168.1.20:8000
ipconfig | findstr /R /C:"IPv4 Address" /C:"IPv4 Address. . . . . . . . . . . :"
echo.
docker compose -f docker-compose.poc.yml ps
endlocal
