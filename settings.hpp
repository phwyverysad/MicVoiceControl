#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>
#include <string>
#include "lang.hpp"

struct AppSettings {
    int volume = 80;
    bool isLocked = true;
    bool isMuted = false;
    std::wstring deviceId = L"default";
    Language lang = Language::THAI;
    bool autoStart = false;
};

class SettingsManager {
public:
    static std::wstring GetIniFilePath() {
        wchar_t appData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_APPDATA, NULL, 0, appData))) {
            std::wstring dir = std::wstring(appData) + L"\\MicVoiceControl";
            CreateDirectoryW(dir.c_str(), NULL);
            return dir + L"\\settings.ini";
        }
        return L"settings.ini";
    }

    static std::wstring GetStartupShortcutPath() {
        wchar_t startupPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath))) {
            return std::wstring(startupPath) + L"\\MicVoiceControl.lnk";
        }
        return L"";
    }

    static void Load(AppSettings& settings) {
        std::wstring ini = GetIniFilePath();
        settings.volume = GetPrivateProfileIntW(L"Settings", L"Volume", 80, ini.c_str());
        if (settings.volume < 0) settings.volume = 0;
        if (settings.volume > 100) settings.volume = 100;

        settings.isLocked = GetPrivateProfileIntW(L"Settings", L"IsLocked", 1, ini.c_str()) != 0;
        settings.isMuted = GetPrivateProfileIntW(L"Settings", L"IsMuted", 0, ini.c_str()) != 0;

        wchar_t devBuf[512] = { 0 };
        GetPrivateProfileStringW(L"Settings", L"DeviceId", L"default", devBuf, 512, ini.c_str());
        settings.deviceId = devBuf;

        int langInt = GetPrivateProfileIntW(L"Settings", L"Language", 0, ini.c_str());
        settings.lang = (langInt == 1) ? Language::ENGLISH : Language::THAI;

        settings.autoStart = IsAutoStartEnabled();
    }

    static void Save(const AppSettings& settings) {
        std::wstring ini = GetIniFilePath();
        WritePrivateProfileStringW(L"Settings", L"Volume", std::to_wstring(settings.volume).c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"IsLocked", settings.isLocked ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"IsMuted", settings.isMuted ? L"1" : L"0", ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"DeviceId", settings.deviceId.c_str(), ini.c_str());
        WritePrivateProfileStringW(L"Settings", L"Language", (settings.lang == Language::ENGLISH) ? L"1" : L"0", ini.c_str());

        SetAutoStart(settings.autoStart);
    }

    static bool IsAutoStartEnabled() {
        std::wstring shortcutPath = GetStartupShortcutPath();
        if (shortcutPath.empty()) return false;
        DWORD dwAttr = GetFileAttributesW(shortcutPath.c_str());
        return (dwAttr != INVALID_FILE_ATTRIBUTES && !(dwAttr & FILE_ATTRIBUTE_DIRECTORY));
    }

    static void SetAutoStart(bool enable) {
        std::wstring shortcutPath = GetStartupShortcutPath();
        if (shortcutPath.empty()) return;

        if (enable) {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(NULL, exePath, MAX_PATH);

            CoInitialize(NULL);
            IShellLinkW* pShellLink = NULL;
            HRESULT hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkW, (void**)&pShellLink);
            if (SUCCEEDED(hr)) {
                pShellLink->SetPath(exePath);
                pShellLink->SetDescription(L"Microphone Voice Control");

                IPersistFile* pPersistFile = NULL;
                hr = pShellLink->QueryInterface(IID_IPersistFile, (void**)&pPersistFile);
                if (SUCCEEDED(hr)) {
                    pPersistFile->Save(shortcutPath.c_str(), TRUE);
                    pPersistFile->Release();
                }
                pShellLink->Release();
            }
        } else {
            DeleteFileW(shortcutPath.c_str());
        }
    }
};

#endif // SETTINGS_HPP
