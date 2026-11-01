@echo off
echo Copying updated MultibandCompressor.vst3 to Program Files...
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "C:\Program Files\Common Files\VST3\MultibandCompressor.vst3"
xcopy /E /I /Y "Builds\VisualStudio2022\x64\Release\VST3\MultibandCompressor.vst3" "%~dp0MultibandCompressor.vst3"
if %ERRORLEVEL% EQU 0 (
    echo SUCCESS! MultibandCompressor.vst3 successfully updated.
) else (
    echo ERROR: Could not copy. Please make sure Audacity / DAW is closed first.
)
pause
