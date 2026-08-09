#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <vector>
#include <string>
#include <functional>

#define WM_APP_DEVICE_CHANGED (WM_APP + 101)
#define WM_APP_VOLUME_CHANGED (WM_APP + 102)

struct MicDeviceInfo {
    std::wstring id;
    std::wstring name;
    bool isDefault = false;
};

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool Initialize(HWND hNotifyWnd);
    void Uninitialize();

    std::vector<MicDeviceInfo> GetInputDevices();
    
    bool SetActiveDevice(const std::wstring& deviceId);
    std::wstring GetActiveDeviceId() const { return m_activeDeviceId; }
    std::wstring GetActiveDeviceName();

    int GetVolume(); // 0 - 100
    bool SetVolume(int volumePercent); // 0 - 100

    bool GetMute();
    bool SetMute(bool mute);

    void SetLockVolume(bool lock, int targetVolumePercent);
    bool IsLocked() const { return m_isLocked; }
    int GetLockedVolume() const { return m_targetVolume; }

    void ForceLockCheck();

private:
    bool BindEndpoint();
    void ReleaseEndpoint();

    HWND m_hNotifyWnd = NULL;
    std::wstring m_activeDeviceId = L"default";
    
    IMMDeviceEnumerator* m_pEnumerator = NULL;
    IMMDevice* m_pDevice = NULL;
    IAudioEndpointVolume* m_pEndpointVolume = NULL;

    class VolumeCallback;
    class NotificationCallback;

    VolumeCallback* m_pVolCallback = NULL;
    NotificationCallback* m_pNotifCallback = NULL;

    bool m_isLocked = false;
    int m_targetVolume = 80;

    GUID m_guidContext;

    friend class VolumeCallback;
    friend class NotificationCallback;
};

#endif // AUDIO_MANAGER_HPP
