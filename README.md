<div align="center">

# Microphone Voice Control

**โปรแกรมล็อกและควบคุมระดับเสียงไมโครโฟนบน Windows จาก System Tray**

[![Platform](https://img.shields.io/badge/Platform-Windows-0078D6?style=flat-square&logo=windows&logoColor=white)](https://github.com/phwyverysad/MicVoiceControl)
[![Language](https://img.shields.io/badge/Language-C%2B%2B17-00599C?style=flat-square&logo=cplusplus&logoColor=white)](https://en.cppreference.com/)
[![License](https://img.shields.io/badge/License-MIT-green.svg?style=flat-square)](LICENSE)
[![Download](https://img.shields.io/badge/Download-Latest%20Release-brightgreen?style=flat-square)](https://github.com/phwyverysad/MicVoiceControl/releases)
[![GitHub Stars](https://img.shields.io/github/stars/phwyverysad/MicVoiceControl?style=flat-square&color=gold)](https://github.com/phwyverysad/MicVoiceControl/stargazers)
[![GitHub Issues](https://img.shields.io/github/issues/phwyverysad/MicVoiceControl?style=flat-square&color=orange)](https://github.com/phwyverysad/MicVoiceControl/issues)

[ภาพรวม](#ภาพรวม) | [ฟีเจอร์หลัก](#ฟีเจอร์หลัก) | [การใช้งาน](#การใช้งาน) | [การคอมไพล์จาก Source Code](#การคอมไพล์จาก-source-code) | [ความปลอดภัยและประสิทธิภาพ](#ความปลอดภัยและประสิทธิภาพ) | [สัญญาอนุญาต](#สัญญาอนุญาต)

</div>

---

## ภาพรวม

Microphone Voice Control (MicVoiceControl) คือแอปพลิเคชันขนาดเล็กแบบเปิดทำงานบน System Tray สำหรับระบบปฏิบัติการ Windows พัฒนาด้วยภาษา C++17 ออกแบบมาเพื่อควบคุม ปิด/เปิดเสียง และล็อกระดับความดังของไมโครโฟนไม่ให้แอปพลิเคชันอื่นปรับเปลี่ยนเองโดยไม่ได้รับอนุญาต

---

## ฟีเจอร์หลัก

### การควบคุมเสียงไมโครโฟน
* **ปิด/เปิดเสียงอย่างรวดเร็ว (Quick Mute)**: คลิกซ้ายที่ไอคอนบน System Tray เพื่อสลับการเปิดหรือปิดไมโครโฟนทันที (ไอคอนจะเปลี่ยนเป็นสีแดงเมื่อ Mute)
* **การล็อกระดับความดัง (Volume Lock)**: ระบบจะตรวจสอบและดึงระดับความดังกลับมาค่าเดิมทันที หากมีแอปพลิเคชันอื่นหรือระบบปฏิบัติการปรับเปลี่ยนความดังไมโครโฟน
* **การปรับความดัง**: ปรับระดับความดังผ่านเมนูคลิกขวา (ขยับทีละ 5% ในช่วง 0-100%) หรือกำหนดตัวเลขเปอร์เซ็นต์ด้วยตัวเอง

### การจัดการและตัวเลือก
* **การเลือกไมโครโฟน**: รองรับการเลือกไมโครโฟนเจาะจงรายตัว หรือใช้ไมโครโฟนเริ่มต้นของระบบ (Default Input Device)
* **รองรับหลายภาษา**: สามารถสลับภาษาในการใช้งานระหว่าง ภาษาไทย และ ภาษาอังกฤษ
* **เปิดพร้อมระบบปฏิบัติการ**: รองรับการตั้งค่าเปิดทำงานอัตโนมัติเมื่อเริ่มต้น Windows (Auto-start on Windows boot)

---

## การใช้งาน

1. ดาวน์โหลดและเปิดใช้งานไฟล์ `MicVoiceControl.exe`
2. **คลิกซ้ายที่ไอคอนบน System Tray**: เพื่อเปิดหรือปิดไมโครโฟน
3. **คลิกขวาที่ไอคอนบน System Tray**: เพื่อเปิดเมนูตั้งค่าระดับความดัง เลือกไมโครโฟน สลับภาษา หรือตั้งค่าเปิดพร้อมเครื่อง

---

## การคอมไพล์จาก Source Code

### สิ่งที่จำเป็นต้องมี
* ระบบปฏิบัติการ Windows 10 หรือ Windows 11 (64-bit)
* Microsoft Visual Studio 2019 / 2022 / 2026 (พร้อม C++ Desktop Development tools) หรือ MSVC `cl.exe`

### ขั้นตอนการคอมไพล์

1. คลองน์คลังข้อมูล (Repository)
   ```cmd
   git clone https://github.com/phwyverysad/MicVoiceControl.git
   cd MicVoiceControl
   ```

2. รันสคริปต์คอมไพล์ผ่าน `build.bat`
   ```cmd
   build.bat
   ```
   *สคริปต์จะทำการคอมไพล์ไฟล์ทรัพยากร (`app.rc`) และคอมไพล์โค้ด C++ ด้วย MSVC (/O2 /GL /MT) ได้เป็นไฟล์ `MicVoiceControl.exe`*

---

## ความปลอดภัยและประสิทธิภาพ

* **เบาและกินทรัพยากรต่ำ**: เขียนด้วย C++17 และ Win32 / WASAPI Native API ไม่พึ่งพา Framework ขนาดใหญ่ ทำงานได้รวดเร็วและใช้หน่วยความจำน้อยมาก
* **ไม่มีการส่งข้อมูลออก**: ข้อมูลการตั้งค่าทั้งหมดถูกบันทึกไว้ในเครื่องของผู้ใช้เท่านั้น

---

## สัญญาอนุญาต

โปรเจกต์นี้เผยแพร่ภายใต้สัญญาอนุญาต MIT License

```
MIT License - Copyright (c) 2026 phwyverysad
```
