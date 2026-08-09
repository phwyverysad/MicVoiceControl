#ifndef LANG_HPP
#define LANG_HPP

#include <string>

enum class Language {
    THAI = 0,
    ENGLISH = 1
};

struct LocStrings {
    const wchar_t* appTitle;
    const wchar_t* volumeMenu;
    const wchar_t* customVolume;
    const wchar_t* enterVolPrompt;
    const wchar_t* invalidVol;
    const wchar_t* selectMic;
    const wchar_t* defaultMic;
    const wchar_t* lockVolume;
    const wchar_t* muteMic;
    const wchar_t* unmuteMic;
    const wchar_t* autoStart;
    const wchar_t* languageToggle;
    const wchar_t* exitApp;
    const wchar_t* lockedTag;
    const wchar_t* unlockedTag;
    const wchar_t* mutedTag;
    const wchar_t* micVolumeLabel;
};

static const LocStrings STR_TH = {
    L"Microphone Voice Control",
    L"ระดับความดังไมค์ (Volume)",
    L"กำหนดเอง... (Custom)",
    L"ระบุระดับเสียงไมโครโฟน (0 - 100%):",
    L"กรุณากรอกตัวเลขระหว่าง 0 ถึง 100",
    L"เลือกไมโครโฟน",
    L"[ค่าเริ่มต้นระบบ]",
    L"ล็อกระดับเสียงไมค์",
    L"ปิดไมโครโฟน (Mute)",
    L"เปิดไมโครโฟน (Unmute)",
    L"เปิดเมื่อเปิดเครื่อง (Auto-start)",
    L"ภาษา / Language (TH)",
    L"ปิดโปรแกรม",
    L"[LOCKED]",
    L"[UNLOCKED]",
    L"[MUTED]",
    L"ระดับเสียงไมค์:"
};

static const LocStrings STR_EN = {
    L"Microphone Voice Control",
    L"Mic Volume Level",
    L"Custom Value...",
    L"Enter Mic Volume Level (0 - 100%):",
    L"Please enter a number between 0 and 100.",
    L"Select Microphone",
    L"[System Default]",
    L"Lock Mic Volume",
    L"Mute Microphone",
    L"Unmute Microphone",
    L"Start with Windows",
    L"Language / ภาษา (EN)",
    L"Exit",
    L"[LOCKED]",
    L"[UNLOCKED]",
    L"[MUTED]",
    L"Mic Volume:"
};

inline const LocStrings& GetLoc(Language lang) {
    return (lang == Language::THAI) ? STR_TH : STR_EN;
}

#endif // LANG_HPP
