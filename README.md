# 🎙️ Microphone Voice Control (C++ Win32 WASAPI Utility)

**Microphone Voice Control** คือโปรแกรม C++ สำหรับคุมและล็อกระดับเสียงไมโครโฟนบน Windows แบบ Native Win32 + WASAPI ออกแบบมาให้ทำงานอยู่บริเวณ System Tray (ถาดงาน) เปิดปุ๊บติดปั๊บ ใช้ทรัพยากรเครื่องต่ำมาก และเปิดให้นำไปใช้งานได้อย่างอิสระแบบสาธารณะ (Public Domain / Free Software)

---

## 🌟 คุณสมบัติหลัก (Key Features)

- **🔇 Left-Click Instant Mute Toggle**: คลิกซ้ายที่ไอคอนใน System Tray เพื่อปิด/เปิดไมโครโฟนทันที
- **🔴 Pure Red Tinted Muted Icon**: เมื่อปิดไมค์ ไอคอนจะถูกย้อมสีเป็น **สีแดงบริสุทธิ์ 100% (Pure Red Tint)** จากไอคอนเดิมทันทีในหน่วยความจำโดยคงรายละเอียดเดิมไว้ครบถ้วน
- **🔒 Real-time Volume Lock (0 - 100%)**: ล็อกระดับความดังไมค์ไม่ให้แอปพลิเคชันอื่น (เช่น Discord, Zoom, Windows AGC) หรือปุ่มบนหูฟังแอบปรับเปลี่ยนเอง หากมีการเปลี่ยนแปลง ระบบจะดึงเสียงกลับมายังระดับที่ล็อกไว้ทันทีด้วย `IAudioEndpointVolumeCallback`
- **🎚️ Context Menu Volume Step & Custom Input**:
  - เมนูย่อย **"ระดับความดังไมค์ (Mic Volume Level)"** บนเมนูคลิกขวา สามารถเลือกสเต็ปความดัง `0%`, `5%`, `10%`, ..., `100%` ได้ทันที (มีเครื่องหมาย ✔️ ติ๊กที่ค่าปัจจุบัน)
  - ตัวเลือก **"กำหนดเอง... (Custom Value...)"** เปิดหน้าต่าง Dialog ช่องป้อนตัวเลขสำหรับกรอกระดับเสียง 0 - 100% ได้อิสระ
- **🔌 Automatic Device Hotplug Detection**: ตรวจจับการเสียบ/ถอดสายไมโครโฟน หรือไมค์ USB ใน Real-time ด้วย `IMMNotificationClient` และอัปเดตรายชื่อไมค์ในเมนูให้อัตโนมัติ
- **🎙️ Default & Specific Microphone Selection**: สลับไปมาระหว่างไมโครโฟนหลักของระบบ `[System Default]` หรือเลือกไมค์รายตัวตาม Device ID
- **🌐 Bilingual Support (Thai / English)**: สลับภาษาในการแสดงผลเมนูและ Tooltip ได้ระหว่างภาษาไทยและภาษาอังกฤษ
- **🚀 Start with Windows**: รองรับการเปิดทำงานอัตโนมัติเมื่อเปิดเครื่องผ่าน Startup Shortcut (`CSIDL_STARTUP`)
- **🛡️ Clean Antivirus Heuristic Structure**: คอมไพล์ด้วย Static CRT (`/MT`), PE Security Flags (`/GS`, `/DYNAMICBASE`, `/NXCOMPAT`), PDB RSDS Debug Signature (`/Zi /FS`), และ Whole Program Optimization (`/GL /LTCG`) เพื่อขจัดปัญหาการแจ้งเตือนผิดพลาด (False Positives)

---

## 📁 โครงสร้างไฟล์ในโปรเจกต์ (Project Structure)

```text
MicVoiceControl/
├── main.cpp            # จุดเริ่มต้นโปรแกรม (WinMain), System Tray, Hotkey/Click, Mutex & Red Icon Tinting
├── audio_manager.hpp   # WASAPI Interface Header (Device Enumeration, Volume Lock & Hotplug Callback)
├── audio_manager.cpp   # WASAPI Interface Implementation
├── settings.hpp        # INI Settings Manager (%APPDATA%\MicVoiceControl\settings.ini) & Startup Shortcut
├── lang.hpp            # ภาษาในระบบ (Thai / English Localization Dictionary)
├── resource.h          # ID Constants สำหรับ Resource และ Dialog Controls
├── app.rc              # Win32 Resource Script (Embed Icon, Dialog Template, Manifest & VS_VERSION_INFO)
├── app.ico             # ไฟล์ไอคอนหลักของแอปพลิเคชัน
├── build.bat           # สคริปต์คอมไพล์โปรแกรมด้วย MSVC Compiler (Visual Studio 64-bit)
└── README.md           # เอกสารอธิบายโปรเจกต์
```

---

## 🛠️ วิธีการคอมไพล์ (Build Instructions)

### ความต้องการของระบบ (Prerequisites)
- ระบบปฏิบัติการ Windows 10 / Windows 11 (64-bit)
- ติดตั้ง **Visual Studio** (รองรับ 2019, 2022, 2026 หรือ Build Tools) ที่มี C++ Desktop Development Component

### ขั้นตอนการคอมไพล์
1. เปิด **Command Prompt** หรือ **PowerShell** ในโฟลเดอร์โปรเจกต์
2. รันสคริปต์สั่งคอมไพล์:
   ```cmd
   build.bat
   ```
3. เมื่อคอมไพล์สำเร็จ จะได้ไฟล์ `MicVoiceControl.exe` เพียงไฟล์เดียวที่พร้อมใช้งานแบบ Portable ทันที

---

## 📄 ใบอนุญาต (License)

ซอฟต์แวร์นี้เปิดให้ใช้งาน เผยแพร่ ดัดแปลง และพัฒนาต่อยอดได้อย่างอิสระสาธารณะ (Public Domain / Unlicense)
