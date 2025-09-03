@echo off
rem -------------
rem  Add CRC32 to App.bin
rem -------------

setlocal enabledelayedexpansion

set INPUT_FILE=.\build\Debug\stm32f103zet6.bin
set OUTPUT_FILE=.\build\Debug\App_crc.bin

rem ── 1. 计算原始 BIN 长度 ──
for %%i in ("%INPUT_FILE%") do set BIN_SIZE=%%~zi
echo App.bin size            : %BIN_SIZE% bytes

rem ── 2. 追加 CRC32（小端）到文件尾 ──
.\srec_cat.exe ^
    "%INPUT_FILE%" -Binary ^
    -crop 0 %BIN_SIZE% ^
    -crc32-l-e %BIN_SIZE% ^
    -o "%OUTPUT_FILE%" -Binary

if not exist "%OUTPUT_FILE%" (
    echo Failed to generate CRC32 bin file!
    goto :eof
)

rem ── 3. 获取追加 CRC32 后 BIN 长度 ──
for %%i in ("%OUTPUT_FILE%") do set BIN_SIZE_CRC=%%~zi

rem ── 4. 裁剪最后 4 字节并打印 CRC32 ──
set /a END_ADDR=%BIN_SIZE%+4

for /f "tokens=2-5" %%a in ('
    .\srec_cat.exe "%OUTPUT_FILE%" -Binary ^
        -crop %BIN_SIZE% !END_ADDR! ^
        -o - -hex-dump
') do (
    set CRC_B0=%%a
    set CRC_B1=%%b
    set CRC_B2=%%c
    set CRC_B3=%%d
    goto :PRINT_CRC
)

:PRINT_CRC
set CRC32=0x!CRC_B3!!CRC_B2!!CRC_B1!!CRC_B0!
echo CRC32 (little-endian stored): !CRC32!
echo App_crc.bin size        : !BIN_SIZE_CRC! bytes

echo CRC32 appended successfully: %OUTPUT_FILE%
endlocal
