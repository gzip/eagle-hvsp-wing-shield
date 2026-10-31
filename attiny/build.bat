@echo off
setlocal enabledelayedexpansion

:: Configuration
set MCU=attiny85
set F_CPU=1000000UL
set TARGET=main
set CC=avr-gcc
set OBJCOPY=avr-objcopy
set SIZE=avr-size
set AVRDUDE=avrdude

:: Auto-detect MSYS2 toolchain paths if not already on PATH
where %CC% >nul 2>nul
if %errorlevel% neq 0 (
    if exist C:\msys64\ucrt64\bin\%CC%.exe (
        set "PATH=C:\msys64\ucrt64\bin;!PATH!"
    ) else if exist C:\msys64\mingw64\bin\%CC%.exe (
        set "PATH=C:\msys64\mingw64\bin;!PATH!"
    )
)

:: Programmer defaults (can be overridden: e.g. 'build flash usbtiny' or set PROGRAMMER=arduino)
if not "%2"=="" set PROGRAMMER=%2
if "%PROGRAMMER%"=="" set PROGRAMMER=usbasp
if "%PORT%"=="" (
    set PORT_ARG=
) else (
    set PORT_ARG=-P %PORT%
)

:: Bit clock / speed override (e.g. set BITCLOCK=32 or 'build flash usbasp 32')
if not "%3"=="" set BITCLOCK=%3
if "%BITCLOCK%"=="" (
    if /I "%PROGRAMMER%"=="usbasp" set BITCLOCK=32
)
if not "%BITCLOCK%"=="" (
    set BITCLOCK_ARG=-B %BITCLOCK%
) else (
    set BITCLOCK_ARG=
)

:: Check for commands
if /I "%1"=="clean" goto :clean
if /I "%1"=="flash" goto :flash
if /I "%1"=="" goto :compile

echo Usage:
echo   build.bat                        - Compile and produce hex file
echo   build.bat flash [prog] [speed]   - Compile and flash (e.g. build flash usbasp 32)
echo   build.bat clean                  - Remove intermediate build files
echo.
echo Supported Programmers:
echo   usbasp  (default, auto-sets -B 32 for reliable 1MHz factory chips)
echo   usbtiny (Little Wire / USBtinyISP)
echo   arduino (requires PORT=COMx)
echo.
echo Environment Variables:
echo   PROGRAMMER (default: usbasp)
echo   PORT       (optional: e.g. COM3 for serial programmers)
echo   BITCLOCK   (optional: e.g. 32 for slow ISP clock)
exit /b 0

:compile
echo === Building %TARGET%.hex for %MCU% (%F_CPU% Hz) ===

where %CC% >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: '%CC%' not found on PATH. Please ensure the AVR-GCC toolchain is installed and on your PATH.
    exit /b 1
)

echo [1/3] Compiling %TARGET%.c...
%CC% -Wall -Wno-array-bounds -Os -DF_CPU=%F_CPU% -mmcu=%MCU% -c %TARGET%.c -o %TARGET%.o
if %errorlevel% neq 0 (
    echo Compilation failed.
    exit /b %errorlevel%
)

echo [2/3] Linking %TARGET%.elf...
%CC% -mmcu=%MCU% %TARGET%.o -o %TARGET%.elf
if %errorlevel% neq 0 (
    echo Linking failed.
    exit /b %errorlevel%
)

echo [3/3] Generating %TARGET%.hex...
%OBJCOPY% -j .text -j .data -O ihex %TARGET%.elf %TARGET%.hex
if %errorlevel% neq 0 (
    echo Hex generation failed.
    exit /b %errorlevel%
)

where %SIZE% >nul 2>nul
if %errorlevel% equ 0 (
    echo.
    %SIZE% --format=avr --mcu=%MCU% %TARGET%.elf
)

echo.
echo Build succeeded: %TARGET%.hex is ready.
exit /b 0

:flash
call :compile
if %errorlevel% neq 0 exit /b %errorlevel%

echo.
echo === Flashing %TARGET%.hex to %MCU% via %PROGRAMMER% ===

where %AVRDUDE% >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: '%AVRDUDE%' not found on PATH. Please ensure avrdude is installed and on your PATH.
    exit /b 1
)

%AVRDUDE% -c %PROGRAMMER% -p t85 %PORT_ARG% %BITCLOCK_ARG% -U flash:w:%TARGET%.hex:i
if %errorlevel% neq 0 (
    echo Flashing failed.
    exit /b %errorlevel%
)

echo Flashing completed successfully.
exit /b 0

:clean
echo Cleaning build files...
if exist %TARGET%.o del /f /q %TARGET%.o
if exist %TARGET%.elf del /f /q %TARGET%.elf
if exist %TARGET%.hex del /f /q %TARGET%.hex
echo Clean complete.
exit /b 0
