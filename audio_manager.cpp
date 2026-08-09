#include "audio_manager.hpp"
#include <Functiondiscoverykeys_devpkey.h>
#include <objbase.h>
#include <rpcndr.h>

class AudioManager::VolumeCallback : public IAudioEndpointVolumeCallback {
public:
    VolumeCallback(AudioManager* parent) : m_parent(parent), m_ref(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_ref);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = InterlockedDecrement(&m_ref);
        if (ref == 0) delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IAudioEndpointVolumeCallback)) {
            *ppv = static_cast<IAudioEndpointVolumeCallback*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override {
        if (!pNotify || !m_parent) return S_OK;

        if (m_parent->m_hNotifyWnd) {
            PostMessageW(m_parent->m_hNotifyWnd, WM_APP_VOLUME_CHANGED, 0, 0);
        }

        if (m_parent->m_isLocked && m_parent->m_pEndpointVolume) {
            float targetScalar = (float)m_parent->m_targetVolume / 100.0f;
            float diff = pNotify->fMasterVolume - targetScalar;
            if (diff < -0.005f || diff > 0.005f) {
                m_parent->m_pEndpointVolume->SetMasterVolumeLevelScalar(targetScalar, &m_parent->m_guidContext);
            }
        }
        return S_OK;
    }

private:
    AudioManager* m_parent;
    LONG m_ref;
};

class AudioManager::NotificationCallback : public IMMNotificationClient {
public:
    NotificationCallback(AudioManager* parent) : m_parent(parent), m_ref(1) {}

    ULONG STDMETHODCALLTYPE AddRef() override {
        return InterlockedIncrement(&m_ref);
    }

    ULONG STDMETHODCALLTYPE Release() override {
        ULONG ref = InterlockedDecrement(&m_ref);
        if (ref == 0) delete this;
        return ref;
    }

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IMMNotificationClient)) {
            *ppv = static_cast<IMMNotificationClient*>(this);
            AddRef();
            return S_OK;
        }
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR pwstrDeviceId, DWORD dwNewState) override {
        NotifyParent();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR pwstrDeviceId) override {
        NotifyParent();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR pwstrDeviceId) override {
        NotifyParent();
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override {
        if (flow == eCapture) {
            NotifyParent();
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR pwstrDeviceId, const PROPERTYKEY key) override {
        return S_OK;
    }

private:
    void NotifyParent() {
        if (m_parent && m_parent->m_hNotifyWnd) {
            PostMessageW(m_parent->m_hNotifyWnd, WM_APP_DEVICE_CHANGED, 0, 0);
        }
    }

    AudioManager* m_parent;
    LONG m_ref;
};

AudioManager::AudioManager() {
    CoCreateGuid(&m_guidContext);
}

AudioManager::~AudioManager() {
    Uninitialize();
}

bool AudioManager::Initialize(HWND hNotifyWnd) {
    m_hNotifyWnd = hNotifyWnd;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&m_pEnumerator);
    if (FAILED(hr)) return false;

    m_pNotifCallback = new NotificationCallback(this);
    m_pEnumerator->RegisterEndpointNotificationCallback(m_pNotifCallback);

    BindEndpoint();
    return true;
}

void AudioManager::Uninitialize() {
    ReleaseEndpoint();

    if (m_pEnumerator) {
        if (m_pNotifCallback) {
            m_pEnumerator->UnregisterEndpointNotificationCallback(m_pNotifCallback);
            m_pNotifCallback->Release();
            m_pNotifCallback = NULL;
        }
        m_pEnumerator->Release();
        m_pEnumerator = NULL;
    }
    CoUninitialize();
}

void AudioManager::ReleaseEndpoint() {
    if (m_pEndpointVolume) {
        if (m_pVolCallback) {
            m_pEndpointVolume->UnregisterControlChangeNotify(m_pVolCallback);
            m_pVolCallback->Release();
            m_pVolCallback = NULL;
        }
        m_pEndpointVolume->Release();
        m_pEndpointVolume = NULL;
    }
    if (m_pDevice) {
        m_pDevice->Release();
        m_pDevice = NULL;
    }
}

bool AudioManager::BindEndpoint() {
    ReleaseEndpoint();

    if (!m_pEnumerator) return false;

    HRESULT hr = E_FAIL;
    if (m_activeDeviceId == L"default" || m_activeDeviceId.empty()) {
        hr = m_pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &m_pDevice);
        if (FAILED(hr)) {
            hr = m_pEnumerator->GetDefaultAudioEndpoint(eCapture, eConsole, &m_pDevice);
        }
    } else {
        hr = m_pEnumerator->GetDevice(m_activeDeviceId.c_str(), &m_pDevice);
    }

    if (FAILED(hr) || !m_pDevice) return false;

    hr = m_pDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, NULL, (void**)&m_pEndpointVolume);
    if (FAILED(hr) || !m_pEndpointVolume) return false;

    m_pVolCallback = new VolumeCallback(this);
    m_pEndpointVolume->RegisterControlChangeNotify(m_pVolCallback);

    if (m_isLocked) {
        ForceLockCheck();
    }

    return true;
}

std::vector<MicDeviceInfo> AudioManager::GetInputDevices() {
    std::vector<MicDeviceInfo> result;
    if (!m_pEnumerator) return result;

    IMMDeviceCollection* pCollection = NULL;
    HRESULT hr = m_pEnumerator->EnumAudioEndpoints(eCapture, DEVICE_STATE_ACTIVE, &pCollection);
    if (FAILED(hr) || !pCollection) return result;

    UINT count = 0;
    pCollection->GetCount(&count);

    IMMDevice* pDefaultDev = NULL;
    std::wstring defaultId = L"";
    if (SUCCEEDED(m_pEnumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &pDefaultDev))) {
        LPWSTR pId = NULL;
        if (SUCCEEDED(pDefaultDev->GetId(&pId))) {
            defaultId = pId;
            CoTaskMemFree(pId);
        }
        pDefaultDev->Release();
    }

    for (UINT i = 0; i < count; i++) {
        IMMDevice* pEndpoint = NULL;
        if (SUCCEEDED(pCollection->Item(i, &pEndpoint))) {
            LPWSTR pId = NULL;
            if (SUCCEEDED(pEndpoint->GetId(&pId))) {
                MicDeviceInfo info;
                info.id = pId;
                info.isDefault = (info.id == defaultId);

                IPropertyStore* pProps = NULL;
                if (SUCCEEDED(pEndpoint->OpenPropertyStore(STGM_READ, &pProps))) {
                    PROPVARIANT varName;
                    PropVariantInit(&varName);
                    if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                        info.name = varName.pwszVal ? varName.pwszVal : L"Microphone";
                        PropVariantClear(&varName);
                    } else {
                        info.name = L"Microphone";
                    }
                    pProps->Release();
                } else {
                    info.name = L"Microphone";
                }

                result.push_back(info);
                CoTaskMemFree(pId);
            }
            pEndpoint->Release();
        }
    }

    pCollection->Release();
    return result;
}

bool AudioManager::SetActiveDevice(const std::wstring& deviceId) {
    m_activeDeviceId = deviceId;
    return BindEndpoint();
}

std::wstring AudioManager::GetActiveDeviceName() {
    if (m_activeDeviceId == L"default" || m_activeDeviceId.empty()) {
        if (m_pDevice) {
            IPropertyStore* pProps = NULL;
            if (SUCCEEDED(m_pDevice->OpenPropertyStore(STGM_READ, &pProps))) {
                PROPVARIANT varName;
                PropVariantInit(&varName);
                if (SUCCEEDED(pProps->GetValue(PKEY_Device_FriendlyName, &varName))) {
                    std::wstring name = varName.pwszVal ? varName.pwszVal : L"Default Microphone";
                    PropVariantClear(&varName);
                    pProps->Release();
                    return name;
                }
                pProps->Release();
            }
        }
        return L"Default Microphone";
    }

    auto devices = GetInputDevices();
    for (const auto& dev : devices) {
        if (dev.id == m_activeDeviceId) {
            return dev.name;
        }
    }
    return L"Microphone";
}

int AudioManager::GetVolume() {
    if (!m_pEndpointVolume) return m_targetVolume;
    float scalar = 0.0f;
    if (SUCCEEDED(m_pEndpointVolume->GetMasterVolumeLevelScalar(&scalar))) {
        return (int)(scalar * 100.0f + 0.5f);
    }
    return m_targetVolume;
}

bool AudioManager::SetVolume(int volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;

    if (m_isLocked) {
        m_targetVolume = volumePercent;
    }

    if (!m_pEndpointVolume) return false;
    float scalar = (float)volumePercent / 100.0f;
    HRESULT hr = m_pEndpointVolume->SetMasterVolumeLevelScalar(scalar, &m_guidContext);
    return SUCCEEDED(hr);
}

bool AudioManager::GetMute() {
    if (!m_pEndpointVolume) return false;
    BOOL bMute = FALSE;
    if (SUCCEEDED(m_pEndpointVolume->GetMute(&bMute))) {
        return (bMute == TRUE);
    }
    return false;
}

bool AudioManager::SetMute(bool mute) {
    if (!m_pEndpointVolume) return false;
    HRESULT hr = m_pEndpointVolume->SetMute(mute ? TRUE : FALSE, &m_guidContext);
    return SUCCEEDED(hr);
}

void AudioManager::SetLockVolume(bool lock, int targetVolumePercent) {
    m_isLocked = lock;
    if (targetVolumePercent >= 0) {
        m_targetVolume = targetVolumePercent;
    }
    if (m_isLocked) {
        ForceLockCheck();
    }
}

void AudioManager::ForceLockCheck() {
    if (m_isLocked && m_pEndpointVolume) {
        float scalar = (float)m_targetVolume / 100.0f;
        m_pEndpointVolume->SetMasterVolumeLevelScalar(scalar, &m_guidContext);
    }
}
