@echo off
setlocal

REM JSPL Palarivattom Gate - Arduino IDE dependency installer
REM Requires arduino-cli to be available on PATH.

set "LIBS=Adafruit GFX Library@1.12.6 Adafruit SSD1306@2.5.15 Adafruit BusIO@1.17.4"

echo Installing JSPL Arduino dependencies...
echo.

arduino-cli lib update-index
if errorlevel 1 goto :error

arduino-cli lib install "Adafruit GFX Library@1.12.6"
if errorlevel 1 goto :error

arduino-cli lib install "Adafruit SSD1306@2.5.15"
if errorlevel 1 goto :error

arduino-cli lib install "Adafruit BusIO@1.17.4"
if errorlevel 1 goto :error

echo.
echo Dependencies installed successfully.
echo You can now open JSPL_PVM_Gate.ino in Arduino IDE and compile.
goto :done

:error
echo.
echo ERROR: One or more Arduino libraries could not be installed.
echo Make sure arduino-cli is installed and available on PATH.
exit /b 1

:done
exit /b 0
