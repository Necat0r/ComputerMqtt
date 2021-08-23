#pragma once

#include "SafePtr.h"

#include <vector>
#include <atomic>
#include <thread>
#include <mutex>
#include <string>
#include <condition_variable>
#include <functional>

#include <endpointvolume.h>

#include <Audiopolicy.h>
#include <Mmdeviceapi.h>

class AudioManager : private IMMNotificationClient, private IAudioEndpointVolumeCallback, private IAudioSessionEvents
{
public:
	struct DeviceInfo
	{
		bool operator==(const DeviceInfo& other) const
		{
			return deviceId == other.deviceId && volume == other.volume && muted == other.muted && sessionName == other.sessionName;
		}

		std::string deviceId;
		float volume;
		bool muted;

		std::string sessionName;

	};

	using Callback = std::function<void(const DeviceInfo&)>;

	AudioManager(Callback&& callback);
	~AudioManager();

	const DeviceInfo& getDeviceInfo() const { return m_deviceInfo; }

	void setDefaultDevice(const char* deviceId);
	void setVolume(float volume);
	void setMute(bool mute);

private:
	void threadRun();
	void refreshDeviceInfo();


	// Inherited via IMMNotificationClient
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID /*riid*/, void ** /*ppvObject*/) override { return E_NOTIMPL; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return 0; }
	virtual ULONG STDMETHODCALLTYPE Release(void) override { return 0; }
	virtual HRESULT STDMETHODCALLTYPE OnDeviceStateChanged(LPCWSTR /*pwstrDeviceId*/, DWORD /*dwNewState*/) override { return E_NOTIMPL; }
	virtual HRESULT STDMETHODCALLTYPE OnDeviceAdded(LPCWSTR /*pwstrDeviceId*/) override { return E_NOTIMPL; }
	virtual HRESULT STDMETHODCALLTYPE OnDeviceRemoved(LPCWSTR /*pwstrDeviceId*/) override { return E_NOTIMPL; }
	virtual HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDefaultDeviceId) override;
	virtual HRESULT STDMETHODCALLTYPE OnPropertyValueChanged(LPCWSTR /*pwstrDeviceId*/, const PROPERTYKEY /*key*/) override { return E_NOTIMPL; }

	// Inherited via IAudioEndpointVolumeCallback
	virtual HRESULT STDMETHODCALLTYPE OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify) override;

	// Inherited via IAudioSessionEvents
	virtual HRESULT __stdcall OnDisplayNameChanged(LPCWSTR NewDisplayName, LPCGUID EventContext) override;
	virtual HRESULT __stdcall OnIconPathChanged(LPCWSTR NewIconPath, LPCGUID EventContext) override;
	virtual HRESULT __stdcall OnSimpleVolumeChanged(float NewVolume, BOOL NewMute, LPCGUID EventContext) override;
	virtual HRESULT __stdcall OnChannelVolumeChanged(DWORD ChannelCount, float NewChannelVolumeArray[], DWORD ChangedChannel, LPCGUID EventContext) override;
	virtual HRESULT __stdcall OnGroupingParamChanged(LPCGUID NewGroupingParam, LPCGUID EventContext) override;
	virtual HRESULT __stdcall OnStateChanged(AudioSessionState NewState) override;
	virtual HRESULT __stdcall OnSessionDisconnected(AudioSessionDisconnectReason DisconnectReason) override;

	Callback m_callback;
	DeviceInfo m_deviceInfo;
	using Devices = std::vector<DeviceInfo>;

	std::condition_variable m_condition;
	std::mutex m_mutex;
	std::thread m_thread;
	std::atomic<bool> m_running = true;

	SafePtr<IMMDeviceEnumerator> m_enumerator;
	SafePtr<IMMDevice> m_device;
	SafePtr<IAudioEndpointVolume> m_volume;
	SafePtr<IAudioSessionControl> m_audioSessionControl;
};