#include "AudioManager.h"
#include "PolicyConfig.h"

#include <cassert>
#include <codecvt>
#include <iostream>
#include <windows.h>

// Shamelessly stolen from https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
std::wstring s2ws(const std::string& str)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.from_bytes(str);
}

std::string ws2s(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

std::ostream& operator<<(std::ostream& stream, const AudioManager::DeviceInfo& info)
{
	stream << "Id: " << info.deviceId << ", volume: " << info.volume << ", muted: " << info.muted ? "true" : "false";

	return stream;
}

GUID context = { 0x62d4adfa, 0x32fb, 0x457a, 0x8e, 0x6c, 0xd5, 0x20, 0x9f, 0x78, 0x6f, 0xc6 };

AudioManager::AudioManager(Callback&& callback)
: m_callback(std::move(callback))
{
	CoInitialize(NULL);

	const auto enumeratorId = __uuidof(MMDeviceEnumerator);
	const auto interfaceId = __uuidof(IMMDeviceEnumerator);

	auto result = CoCreateInstance(enumeratorId, NULL, CLSCTX_ALL, interfaceId, (void**)&m_enumerator);
	(void)result;
	assert(result == S_OK);

	m_enumerator->RegisterEndpointNotificationCallback(this);

	refreshDeviceInfo();

	m_thread = std::thread(&AudioManager::threadRun, this);
}

AudioManager::~AudioManager()
{
	if (m_enumerator)
		m_enumerator->UnregisterEndpointNotificationCallback(this);

	m_running = false;
	m_condition.notify_all();
	m_thread.join();
}

void AudioManager::setDefaultDevice(const char * deviceId)
{
	CoInitialize(NULL);

	// Note that IPolicyConfig doesn't seem to work properly. The call to CoCreateInstance will fail,
	// at least on Win10; hence the Vista version is being used instead.

	IPolicyConfigVista* policyConfig;
	auto result = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)&policyConfig);
	if (FAILED(result))
		return;

	std::wstring wideDeviceId = s2ws(deviceId);
	policyConfig->SetDefaultEndpoint(wideDeviceId.c_str(), eConsole);
	policyConfig->Release();

	CoUninitialize();
}

void AudioManager::threadRun()
{
	CoInitialize(NULL);

	while (m_running)
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_condition.wait(lock);

		refreshDeviceInfo();
	}

	CoUninitialize();
}

void AudioManager::setVolume(float volume)
{
	m_volume->SetMasterVolumeLevelScalar(volume, &context);
}

void AudioManager::setMute(bool mute)
{
	if (!m_volume)
		return;

	m_volume->SetMute(mute ? TRUE : FALSE, &context);
}

void AudioManager::refreshDeviceInfo()
{
	// Clean up previous registrations
	if (m_volume)
		m_volume->UnregisterControlChangeNotify(this);
	m_volume.release();
	m_device.release();

	// Get new default device
	m_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &m_device);

	// Get device ID
	LPWSTR wideDeviceId = nullptr;
	m_device->GetId(&wideDeviceId);
	std::string deviceId = ws2s(wideDeviceId);
	CoTaskMemFree(wideDeviceId);

	// Get volume & mute
	m_device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&m_volume);
	m_volume->RegisterControlChangeNotify(this);
	
	float volume = 0.f;
	m_volume->GetMasterVolumeLevelScalar(&volume);

	BOOL mute = FALSE;
	m_volume->GetMute(&mute);

	// Update device info
	DeviceInfo info{ deviceId, volume, mute == TRUE };
	if (info == m_deviceInfo)
		return;

	m_deviceInfo = info;
	std::cout << "New state. " << m_deviceInfo << std::endl;

// FIXME - Release lock before callback...
	if (m_callback)
		m_callback(m_deviceInfo);
}

HRESULT AudioManager::OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR /*pwstrDefaultDeviceId*/)
{
	if (flow != eRender)
		return NOERROR;

	if (role != eMultimedia)
		return NOERROR;

	m_condition.notify_all();

	return NOERROR;
}

HRESULT AudioManager::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify)
{
	// Get device ID
	LPWSTR deviceId = nullptr;
	m_device->GetId(&deviceId);
	std::string deviceIdStr = ws2s(deviceId);
	CoTaskMemFree(deviceId);

	const DeviceInfo info{ deviceIdStr, pNotify->fMasterVolume, pNotify->bMuted == TRUE};

	{
		std::unique_lock<std::mutex> lock(m_mutex);

		if (info == m_deviceInfo)
			return NOERROR;

		m_deviceInfo = info;
	}

	std::cout << "New state. " << m_deviceInfo << std::endl;

	if (m_callback)
		m_callback(info);

	return NOERROR;
}
