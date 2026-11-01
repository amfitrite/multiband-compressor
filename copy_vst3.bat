@echo off
set VERSION=2.02
echo ====================================================
echo Deploying Multiband Compressor %VERSION% VST3
echo ====================================================

REM 1. Copy bundle VST3 lengkap ke Program Files Common Files (untuk DAW modern)
echo [1/3] Copying VST3 bundle to Program Files...
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "C:\Program Files\Common Files\VST3\MultibandCompressor.vst3"
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "C:\Program Files\Common Files\VST3\Multiband Compressor %VERSION%.vst3"

REM 2. Copy single file .vst3 langsung ke root folder utama (mudah ditemukan & langsung pakai)
echo [2/3] Copying standalone .vst3 file to root directory...
copy /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3\Contents\x86_64-win\MultibandCompressor.vst3" "%~dp0Multiband Compressor %VERSION%.vst3"

if %ERRORLEVEL% EQU 0 (
    echo [3/3] SUCCESS! File "Multiband Compressor %VERSION%.vst3" is ready in the root folder!
) else (
    echo ERROR: Could not copy. Please make sure DAW is closed.
)
pause
