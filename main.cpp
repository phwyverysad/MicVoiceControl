#include <windows.h>
#include <shellapi.h>
#include <commctrl.h>
#include <string>
#include <vector>
#include "resource.h"
#include "audio_manager.hpp"
#include "settings.hpp"
#include "lang.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

#define WM_TRAYICON (WM_APP + 1)
#define WM_SHOW_CUSTOM_DIALOG (WM_APP + 2)
#define TIMER_LOCK_ID 1001

static const wchar_t* MAIN_CLASS_NAME = L"MicVoiceControlMainClass";

static HWND g_hMainWnd = NULL;
static NOTIFYICONDATAW g_nid = { 0 };
static HICON g_hIconNormal = NULL;
static HICON g_hIconMuted = NULL;
static AudioManager g_audio;
static AppSettings g_settings;
static HANDLE g_hMutex = NULL;

// Create a pure red-tinted HICON dynamically in memory from base HICON (exact original icon, no added shapes)
HICON CreateRedTintedIcon(HICON hBaseIcon) {
    if (!hBaseIcon) return NULL;

    ICONINFO iconInfo = { 0 };
    if (!GetIconInfo(hBaseIcon, &iconInfo)) return NULL;

    BITMAP bm = { 0 };
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bm);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    HBITMAP hbmColorRed = CreateCompatibleBitmap(hdcScreen, bm.bmWidth, bm.bmHeight);
    HBITMAP hOldMem = (HBITMAP)SelectObject(hdcMem, hbmColorRed);
    
    DrawIconEx(hdcMem, 0, 0, hBaseIcon, bm.bmWidth, bm.bmHeight, 0, NULL, DI_NORMAL);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = bm.bmWidth;
    bmi.bmiHeader.biHeight = -bm.bmHeight; // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<DWORD> pixels(bm.bmWidth * bm.bmHeight);
    GetDIBits(hdcMem, hbmColorRed, 0, bm.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);

    for (size_t i = 0; i < pixels.size(); i++) {
        DWORD alpha = (pixels[i] >> 24) & 0xFF;
        if (alpha > 0) {
            DWORD r = (pixels[i] >> 16) & 0xFF;
            DWORD g = (pixels[i] >> 8) & 0xFF;
            DWORD b = pixels[i] & 0xFF;

            // Pure Red Tinting: preserve brightness/luminance details while shifting hue to vivid red
            DWORD luminance = (DWORD)(0.299f * r + 0.587f * g + 0.114f * b);
            DWORD newR = min(255, (int)(luminance * 1.1f + 60));
            DWORD newG = (DWORD)(luminance * 0.12f);
            DWORD newB = (DWORD)(luminance * 0.12f);

            pixels[i] = (alpha << 24) | (newR << 16) | (newG << 8) | newB;
        }
    }

    SetDIBits(hdcMem, hbmColorRed, 0, bm.bmHeight, pixels.data(), &bmi, DIB_RGB_COLORS);

    SelectObject(hdcMem, hOldMem);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    ICONINFO redIconInfo = iconInfo;
    redIconInfo.hbmColor = hbmColorRed;
    HICON hRedIcon = CreateIconIndirect(&redIconInfo);

    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DeleteObject(hbmColorRed);

    return hRedIcon;
}

void UpdateTrayState() {
    bool isMuted = g_audio.GetMute();
    bool isLocked = g_audio.IsLocked();
    int curVol = g_audio.GetVolume();
    std::wstring devName = g_audio.GetActiveDeviceName();

    g_nid.hIcon = isMuted ? g_hIconMuted : g_hIconNormal;

    const LocStrings& loc = GetLoc(g_settings.lang);
    std::wstring tooltip = devName + L": " + std::to_wstring(curVol) + L"% ";
    if (isMuted) {
        tooltip += loc.mutedTag;
    } else if (isLocked) {
        tooltip += loc.lockedTag;
    } else {
        tooltip += loc.unlockedTag;
    }

    if (tooltip.length() >= 127) {
        tooltip = tooltip.substr(0, 124) + L"...";
    }

    wcsncpy_s(g_nid.szTip, tooltip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &g_nid);

    g_settings.isMuted = isMuted;
    g_settings.volume = curVol;
    g_settings.isLocked = isLocked;
}

INT_PTR CALLBACK CustomVolDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {
    const LocStrings& loc = GetLoc(g_settings.lang);

    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowTextW(hDlg, loc.volumeMenu);
        SetDlgItemTextW(hDlg, IDC_STATIC, loc.enterVolPrompt);
        SetDlgItemInt(hDlg, IDC_EDIT_CUSTOM_VOL, (UINT)lParam, FALSE);
        SendDlgItemMessageW(hDlg, IDC_EDIT_CUSTOM_VOL, EM_SETSEL, 0, -1);
        SetFocus(GetDlgItem(hDlg, IDC_EDIT_CUSTOM_VOL));
        return FALSE;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == IDOK) {
            BOOL translated = FALSE;
            UINT val = GetDlgItemInt(hDlg, IDC_EDIT_CUSTOM_VOL, &translated, FALSE);
            if (!translated || val > 100) {
                MessageBoxW(hDlg, loc.invalidVol, L"Error", MB_ICONWARNING);
                return TRUE;
            }
            EndDialog(hDlg, (INT_PTR)val);
            return TRUE;
        } else if (id == IDCANCEL) {
            EndDialog(hDlg, -1);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

void OpenCustomVolumeDialog() {
    int currentVol = g_audio.GetVolume();
    INT_PTR res = DialogBoxParamW(GetModuleHandle(NULL), MAKEINTRESOURCEW(IDD_CUSTOM_VOL_DIALOG), g_hMainWnd, CustomVolDlgProc, (LPARAM)currentVol);
    if (res >= 0 && res <= 100) {
        int newVol = (int)res;
        g_audio.SetVolume(newVol);
        g_settings.volume = newVol;
        if (g_audio.IsLocked()) {
            g_audio.SetLockVolume(true, newVol);
        }
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    }
}

void ShowTrayContextMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    const LocStrings& loc = GetLoc(g_settings.lang);
    int currentVol = g_audio.GetVolume();
    bool isStepValue = (currentVol >= 0 && currentVol <= 100 && (currentVol % 5 == 0));

    // Volume Step Submenu (0 - 100% step 5 + Custom)
    HMENU hSubVol = CreatePopupMenu();
    for (int v = 0; v <= 100; v += 5) {
        UINT flags = MF_STRING;
        if (isStepValue && v == currentVol) {
            flags |= MF_CHECKED;
        }
        std::wstring volStr = std::to_wstring(v) + L"%";
        UINT cmdId = IDM_VOL_STEP_BASE + (v / 5);
        InsertMenuW(hSubVol, -1, flags, cmdId, volStr.c_str());
    }
    InsertMenuW(hSubVol, -1, MF_SEPARATOR, 0, NULL);

    std::wstring customLabel = loc.customVolume;
    customLabel += L" (" + std::to_wstring(currentVol) + L"%)";

    UINT customFlags = MF_STRING;
    if (!isStepValue) {
        customFlags |= MF_CHECKED;
    }
    InsertMenuW(hSubVol, -1, customFlags, IDM_VOL_CUSTOM, customLabel.c_str());

    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubVol, loc.volumeMenu);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    // Microphone selector sub-menu
    HMENU hSubMic = CreatePopupMenu();
    bool isDefaultSelected = (g_settings.deviceId == L"default" || g_settings.deviceId.empty());
    UINT defaultFlags = MF_STRING | (isDefaultSelected ? MF_CHECKED : MF_UNCHECKED);
    InsertMenuW(hSubMic, -1, defaultFlags, IDM_MIC_SELECT_DEFAULT, loc.defaultMic);
    InsertMenuW(hSubMic, -1, MF_SEPARATOR, 0, NULL);

    auto devices = g_audio.GetInputDevices();
    for (size_t i = 0; i < devices.size() && i < 50; i++) {
        UINT flags = MF_STRING;
        if (!isDefaultSelected && g_settings.deviceId == devices[i].id) {
            flags |= MF_CHECKED;
        }
        std::wstring itemText = devices[i].name;
        if (devices[i].isDefault) {
            itemText += L" (Default)";
        }
        InsertMenuW(hSubMic, -1, flags, IDM_MIC_SELECT_BASE + (UINT)i, itemText.c_str());
    }

    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_POPUP, (UINT_PTR)hSubMic, loc.selectMic);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING | (g_audio.IsLocked() ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_LOCK_VOLUME, loc.lockVolume);

    bool isMuted = g_audio.GetMute();
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING | (isMuted ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_MUTE_TOGGLE, isMuted ? loc.unmuteMic : loc.muteMic);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING | (g_settings.autoStart ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_AUTOSTART, loc.autoStart);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, (g_settings.lang == Language::THAI) ? IDM_TRAY_LANG_EN : IDM_TRAY_LANG_TH, loc.languageToggle);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
    InsertMenuW(hMenu, -1, MF_BYPOSITION | MF_STRING, IDM_TRAY_EXIT, loc.exitApp);

    SetForegroundWindow(g_hMainWnd);
    UINT cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, g_hMainWnd, NULL);
    DestroyMenu(hMenu);

    if (cmd >= IDM_VOL_STEP_BASE && cmd <= IDM_VOL_STEP_BASE + 20) {
        int newVol = (cmd - IDM_VOL_STEP_BASE) * 5;
        g_audio.SetVolume(newVol);
        g_settings.volume = newVol;
        if (g_audio.IsLocked()) {
            g_audio.SetLockVolume(true, newVol);
        }
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_VOL_CUSTOM) {
        OpenCustomVolumeDialog();
    } else if (cmd == IDM_MIC_SELECT_DEFAULT) {
        g_settings.deviceId = L"default";
        g_audio.SetActiveDevice(L"default");
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd >= IDM_MIC_SELECT_BASE && cmd < IDM_MIC_SELECT_BASE + devices.size()) {
        size_t idx = cmd - IDM_MIC_SELECT_BASE;
        g_settings.deviceId = devices[idx].id;
        g_audio.SetActiveDevice(g_settings.deviceId);
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_LOCK_VOLUME) {
        bool lock = !g_audio.IsLocked();
        g_settings.isLocked = lock;
        g_audio.SetLockVolume(lock, g_audio.GetVolume());
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_MUTE_TOGGLE) {
        bool mute = !g_audio.GetMute();
        g_audio.SetMute(mute);
        g_settings.isMuted = mute;
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_AUTOSTART) {
        g_settings.autoStart = !g_settings.autoStart;
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_LANG_TH) {
        g_settings.lang = Language::THAI;
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_LANG_EN) {
        g_settings.lang = Language::ENGLISH;
        SettingsManager::Save(g_settings);
        UpdateTrayState();
    } else if (cmd == IDM_TRAY_EXIT) {
        PostQuitMessage(0);
    }
}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        SetTimer(hWnd, TIMER_LOCK_ID, 1000, NULL);
        return 0;
    }

    case WM_TIMER: {
        if (wParam == TIMER_LOCK_ID) {
            g_audio.ForceLockCheck();
            UpdateTrayState();
        }
        return 0;
    }

    case WM_APP_VOLUME_CHANGED:
    case WM_APP_DEVICE_CHANGED: {
        UpdateTrayState();
        return 0;
    }

    case WM_SHOW_CUSTOM_DIALOG: {
        OpenCustomVolumeDialog();
        return 0;
    }

    case WM_TRAYICON: {
        if (lParam == WM_LBUTTONUP) {
            // Left-click: Toggle Mute immediately!
            bool currentMute = g_audio.GetMute();
            g_audio.SetMute(!currentMute);
            g_settings.isMuted = !currentMute;
            SettingsManager::Save(g_settings);
            UpdateTrayState();
        } else if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
            // Right-click: Show Context Menu
            ShowTrayContextMenu();
        }
        return 0;
    }

    case WM_DESTROY: {
        KillTimer(hWnd, TIMER_LOCK_ID);
        Shell_NotifyIconW(NIM_DELETE, &g_nid);
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Single Instance Check using FindWindowW (Clean Win32 API, zero AV heuristic flags)
    HWND hExisting = FindWindowW(MAIN_CLASS_NAME, NULL);
    if (hExisting) {
        DWORD dwProcId = 0;
        GetWindowThreadProcessId(hExisting, &dwProcId);
        AllowSetForegroundWindow(dwProcId);
        PostMessageW(hExisting, WM_SHOW_CUSTOM_DIALOG, 0, 0);
        return 0;
    }

    // Common controls initialization
    INITCOMMONCONTROLSEX icex = { sizeof(INITCOMMONCONTROLSEX), ICC_BAR_CLASSES | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icex);

    // Load Settings
    SettingsManager::Load(g_settings);

    // Register Window Class
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(0, MAIN_CLASS_NAME, L"MicVoiceControlHost", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!g_hMainWnd) return 0;

    // Initialize WASAPI Audio Manager
    if (!g_audio.Initialize(g_hMainWnd)) {
        MessageBoxW(NULL, L"Failed to initialize Windows Audio Session API (WASAPI).", L"Error", MB_ICONERROR);
        return 0;
    }

    g_audio.SetActiveDevice(g_settings.deviceId);
    g_audio.SetLockVolume(g_settings.isLocked, g_settings.volume);
    g_audio.SetMute(g_settings.isMuted);

    // Load Base Icon and create Red Tinted Icon for Muted state
    g_hIconNormal = LoadIconW(hInstance, (LPCWSTR)MAKEINTRESOURCEW(IDI_MAIN_ICON));
    if (!g_hIconNormal) {
        g_hIconNormal = LoadIconW(NULL, (LPCWSTR)IDI_APPLICATION);
    }
    g_hIconMuted = CreateRedTintedIcon(g_hIconNormal);

    // Create Tray Icon
    g_nid.cbSize = sizeof(NOTIFYICONDATAW);
    g_nid.hWnd = g_hMainWnd;
    g_nid.uID = 1;
    g_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    g_nid.uCallbackMessage = WM_TRAYICON;
    g_nid.hIcon = g_settings.isMuted ? g_hIconMuted : g_hIconNormal;

    Shell_NotifyIconW(NIM_ADD, &g_nid);
    UpdateTrayState();

    // Message Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Cleanup
    g_audio.Uninitialize();

    if (g_hIconMuted) DestroyIcon(g_hIconMuted);

    return 0;
}
