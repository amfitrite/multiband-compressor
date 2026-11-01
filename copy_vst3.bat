@echo off
set VERSION=2.01
echo ====================================================
echo Deploying Multiband Compressor %VERSION% VST3
echo ====================================================

REM 1. Copy ke direktori VST3 sistem (Program Files)
echo [1/3] Copying to Program Files Common Files...
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "C:\Program Files\Common Files\VST3\MultibandCompressor.vst3"
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "C:\Program Files\Common Files\VST3\Multiband Compressor %VERSION%.vst3"

REM 2. Copy ke root direktori (nama berversi dan standar untuk kemudahan temuan pengguna)
echo [2/3] Copying versioned VST3 to root folder...
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "%~dp0Multiband Compressor %VERSION%.vst3\"
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "%~dp0MultibandCompressor.vst3\"

if %ERRORLEVEL% EQU 0 (
    echo [3/3] SUCCESS! Multiband Compressor %VERSION% VST3 is now available at root!
) else (
    echo ERROR: Could not copy. Please make sure Audacity / DAW is closed first.
)
pause
