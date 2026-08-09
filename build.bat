@echo off
chcp 65001 >nul
echo Building Microphone Voice Control application...

call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

echo Compiling resources...
rc.exe /fo app.res app.rc
if %ERRORLEVEL% NEQ 0 (
    echo Resource compilation failed!
    exit /b %ERRORLEVEL%
)

echo Compiling C++ sources with Production Release LTCG (/GL), Static CRT (/MT), Debug Directory (/Zi) and PE security flags...
cl.exe /nologo /utf-8 /MT /O2 /GL /W3 /GS /Zi /FS /EHsc /std:c++17 /DUNICODE /D_UNICODE main.cpp audio_manager.cpp app.res /Fe:MicVoiceControl.exe /link /LTCG /INCREMENTAL:NO /MACHINE:X64 /DEBUG /OPT:REF /OPT:ICF /SUBSYSTEM:WINDOWS,6.01 /DYNAMICBASE /NXCOMPAT /HIGHENTROPYVA user32.lib gdi32.lib shell32.lib ole32.lib oleaut32.lib comctl32.lib shlwapi.lib

if %ERRORLEVEL% EQU 0 (
    echo.
    echo =======================================================
    echo BUILD SUCCESSFUL! Generated executable: MicVoiceControl.exe
    echo =======================================================
) else (
    echo.
    echo BUILD FAILED! Please check error messages above.
)
