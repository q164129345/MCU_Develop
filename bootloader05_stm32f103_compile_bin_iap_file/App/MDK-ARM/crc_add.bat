@echo off
echo ===============================
echo  Add CRC32 to App.bin
echo ===============================

set INPUT_FILE=.\Output\App.bin
set OUTPUT_FILE=.\Output\App_crc.bin

for %%i in (%INPUT_FILE%) do set BIN_SIZE=%%~zi
echo App.bin size: %BIN_SIZE% bytes

.\srec_cat.exe %INPUT_FILE% -Binary -crop 0 %BIN_SIZE% -crc32-l-e %BIN_SIZE% -o %OUTPUT_FILE% -Binary

if exist %OUTPUT_FILE% (
    echo CRC32 appended successfully: %OUTPUT_FILE%
) else (
    echo Failed to generate CRC32 bin file, please check srec_cat tool and parameters!
)

