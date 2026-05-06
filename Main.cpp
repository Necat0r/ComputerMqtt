#define _CRT_SECURE_NO_WARNINGS

#include "Mqtt.h"
#include "Power.h"
#include "Monitor.h"
#include "Audio\AudioManager.h"
#include "HomeAssistant.h"
#include "Log.h"

#include <Windows.h>
#include <shellapi.h>
#include <algorithm>

static const char* ClientId = "computerMqtt";

static const char* PowerStatus = "computer/power";
static const char* PowerControl = "computer/power/control";
static const char* MonitorStatus = "computer/monitor";
static const char* MonitorControl = "computer/monitor/control";

// Audio constants
// FIXME - Make output more generic.
static const char* AudioOutputStatus = "computer/audio/output";
static const char* AudioOutputControl = "computer/audio/output/control";
static const char* AudioVolumeStatus = "computer/audio/volume";
static const char* AudioVolumeControl = "computer/audio/volume/control";
static const char* AudioMuteStatus = "computer/audio/mute";
static const char* AudioMuteControl = "computer/audio/mute/control";

static const char* HomeAssistantBirthTopic = "homeassistant/status";
static const char* HomeAssistantDiscoveryTopic = "homeassistant/device/computer/config";

const char* recieverDeviceIds[] = {
	"{0.0.0.00000000}.{ab6192b9-c22c-44c1-9086-14b4772011bf}",
	"{0.0.0.00000000}.{bbf40758-edce-4f98-b10d-63a1b25cdc14}" };

const char* receiverDeviceId = "{0.0.0.00000000}.{ab6192b9-c22c-44c1-9086-14b4772011bf}";
const char* receiverDeviceId2 = "{0.0.0.00000000}.{bbf40758-edce-4f98-b10d-63a1b25cdc14}";
const char* headphonesDeviceId = "{0.0.0.00000000}.{f5d2779d-740e-49ea-b3cd-eaa9e8e0d2bc}";
//const char* viveDeviceId = "{0.0.0.00000000}.{08735c7b-757c-4332-9976-555a5ec79fca}";

const int volumeScale = 100;

namespace
{
	inline char charToLower(char c)
	{
		return (char)::tolower(c);
	}

	bool parseBool(bool& boolOut, const char* payload)
	{
		std::string str(payload);
		std::transform(str.begin(), str.end(), str.begin(), ::charToLower);

		if (str == "true" || str == "1")
		{
			boolOut = true;
			return true;
		}
		else if (str == "false" || str == "0")
		{
			boolOut = false;
			return true;
		}

		return false;
	}
}

static constexpr UINT WMAPP_TRAYICON     = WM_APP + 1;
static constexpr UINT WMAPP_SETCONNECTED = WM_APP + 2;
static constexpr UINT TRAY_ICON_ID       = 1;
static HWND g_hwnd = nullptr;

class ComputerMqtt : public Mqtt
{
public:
	ComputerMqtt(const char* host, int port)
		: Mqtt(ClientId, host, port)
		, m_audioManager([this](const AudioManager::DeviceInfo& info) { this->onAudioCallback(info); })
		, m_monitor([this](bool monitorOn) { this->onMonitorPowerCallback(monitorOn); })
	{}

	virtual void onConnected() override
	{
		PostMessage(g_hwnd, WMAPP_SETCONNECTED, TRUE, 0);

		subscribe(PowerControl);
		subscribe(MonitorControl);

		// Audio topics
		subscribe(AudioOutputControl);
		subscribe(AudioVolumeControl);
		subscribe(AudioMuteControl);

		publish({ PowerStatus, "true" }, true);
		publish({ MonitorStatus, "true" }, true);

		subscribe(HomeAssistantBirthTopic);

		// Last wills
		setWills({
			{ PowerStatus, "false" },
			{ MonitorStatus, "false" },
			{ AudioOutputStatus, "false" },
			{ AudioVolumeStatus, "0" },
			{ AudioMuteStatus, "false" },
		});

		sendHomeAssistantDiscoveryMessage();

		initPowerNotifications([this](bool resumed) {
			publish({ PowerStatus, resumed ? "true" : "false" }, true);
		});
	}

	virtual void onDisconnected() override
	{
		PostMessage(g_hwnd, WMAPP_SETCONNECTED, FALSE, 0);
	}

	virtual void onMessage(const Message& message) override
	{
		auto&& topic = message.topic;

		if (topic == PowerControl)
			handlePower(message);
		else if (topic == MonitorControl)
			handleMonitor(message);
		else if (topic == AudioOutputControl)
			handleAudioOutput(message);
		else if (topic == AudioVolumeControl)
			handleAudioVolume(message);
		else if (topic == AudioMuteControl)
			handleAudioMute(message);
		else if (topic == HomeAssistantBirthTopic)
			handleHomeAssistantBirth(message);
	}

	void handlePower(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			Log::write("Could not parse data for topic: %s", PowerControl);
			return;
		}

		if (value == true)
			return;

		Log::write("Suspending machine");

		Mqtt::Message statusMessage;
		statusMessage.topic = PowerStatus;
		statusMessage.payload = "false";
		publish(statusMessage, true);

		// Try to force the message to be delivered.
		loop();

		standby();
	}

	void handleMonitor(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			Log::write("Could not parse data for topic: %s", MonitorControl);
			return;
		}

		Log::write("Turning monitor %s", value ? "On" : "Off");
		m_monitor.setPower(value);
	}

	void onMonitorPowerCallback(bool monitorOn)
	{
		Mqtt::Message message;
		message.topic = MonitorStatus;
		message.payload = monitorOn ? "true" : "false";

		publish(message, true);
	}

	void onAudioCallback(const AudioManager::DeviceInfo& newInfo)
	{
		Mqtt::Message message;

		// DeviceId
		if (newInfo.deviceId != m_lastInfo.deviceId)
		{
			bool receiverOutput = false;
			for (int i = 0; i < sizeof(recieverDeviceIds) / sizeof(recieverDeviceIds[0]); ++i)
				receiverOutput = receiverOutput || newInfo.deviceId == recieverDeviceIds[i];

			//const bool receiverOutput = newInfo.deviceId == receiverDeviceId || newInfo.deviceId == receiverDeviceId2;
			message.topic = AudioOutputStatus;
			message.payload = receiverOutput ? "true" : "false";
			publish(message, true);
		}

		// Volume
		if (newInfo.volume != m_lastInfo.volume)
		{
			message.topic = AudioVolumeStatus;
			int volume = int(newInfo.volume * volumeScale);
			message.payload = std::to_string(volume);
			publish(message, true);
		}

		// Mute
		if (newInfo.muted != m_lastInfo.muted)
		{
			message.topic = AudioMuteStatus;
			message.payload = newInfo.muted ? "true" : "false";
			publish(message, true);
		}

		m_lastInfo = newInfo;
	}

	void handleAudioOutput(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			Log::write("Could not parse data for topic: %s", message.topic.c_str());
			return;
		}

		const char* deviceId = nullptr;
		if (value)
			deviceId = receiverDeviceId;
		else
			deviceId = headphonesDeviceId;

		m_audioManager.setDefaultDevice(deviceId);
	}

	void handleAudioVolume(const Message& message)
	{
		int scaledVolume = 0;
		auto result = sscanf(message.payload.c_str(), "%d", &scaledVolume);
		if (result != 1)
		{
			Log::write("Failed parsing payload: \"%s\", for topic: %s", message.payload.c_str(), message.topic.c_str());
			return;
		}

		float volume = float(scaledVolume) / float(volumeScale);
		m_audioManager.setVolume(volume);
	}

	void handleAudioMute(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			Log::write("Could not parse data for topic: %s", message.topic.c_str());
			return;
		}

		m_audioManager.setMute(value);
	}

	void handleHomeAssistantBirth(const Message& message)
	{
		bool online = message.payload == "online";
		if (online)
			sendHomeAssistantDiscoveryMessage();
	}

	void sendHomeAssistantDiscoveryMessage()
	{
		ha::Discovery discovery;

		// Availability is the same between all components
		ha::Component::Availability availability;
		availability.topic = PowerStatus;

		// FIXME - Should uniqueId be globally or device-locally unique?

		discovery.device = ha::Discovery::Device{
			.name = "Computer",
			.identifier = "computer",
		};

		discovery.origin = ha::Discovery::Origin{
			.name = "Computer",
			.softwareVersion = "1.0",
			.supportUrl = "http://glhf"
		};

		// Power switch
		{
			ha::Switch power;
			power.name = "Power";
			power.uniqueId = "power";

			power.icon = "mdi:power";

			power.stateTopic = PowerStatus;
			power.commandTopic = PowerControl;

			power.availability = availability;

			discovery.switches.push_back(std::move(power));
		}

		// Monitor switch
		{
			ha::Switch monitor;
			monitor.name = "Monitor";
			monitor.uniqueId = "monitor";
			monitor.deviceClass = "switch";

			monitor.icon = "mdi:monitor";

			monitor.stateTopic = MonitorStatus;
			monitor.commandTopic = MonitorControl;

			monitor.availability = availability;

			discovery.switches.push_back(std::move(monitor));
		}

		// Volume
		{
			ha::Number volume;

			volume.name = "Volume";
			volume.uniqueId = "volume";
			//volume.deviceClass = "None";

			volume.icon = "mdi:speaker";

			volume.stateTopic = AudioVolumeStatus;
			volume.commandTopic = AudioVolumeControl;

			volume.mode = ha::Number::Mode::Slider;
			volume.min = 0;
			volume.max = 100;

			volume.unitOfMeasurement = "%";

			volume.availability = availability;

			discovery.numbers.push_back(std::move(volume));
		}

		// Mute
		{
			ha::Switch mute;

			mute.name = "Mute";
			mute.uniqueId = "mute";
			mute.deviceClass = "switch";

			mute.icon = "mdi:volume-mute";

			mute.stateTopic = AudioMuteStatus;
			mute.commandTopic = AudioMuteControl;

			mute.availability = availability;

			discovery.switches.push_back(std::move(mute));
		}

		std::string discoveryPayload = ha::toJson(discovery);

		publish({ HomeAssistantDiscoveryTopic, discoveryPayload }, false);
	}

	AudioManager m_audioManager;
	AudioManager::DeviceInfo m_lastInfo;
	Monitor m_monitor;
};

// ---- Win32 message loop infrastructure ----

static DWORD g_mainThreadId = 0;

static void trayAdd(HWND hwnd)
{
	NOTIFYICONDATA nid{};
	nid.cbSize           = sizeof(nid);
	nid.hWnd             = hwnd;
	nid.uID              = TRAY_ICON_ID;
	nid.uFlags           = NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_MESSAGE;
	nid.uCallbackMessage = WMAPP_TRAYICON;
	nid.hIcon            = LoadIcon(nullptr, IDI_WARNING); // disconnected until MQTT connects
	wcscpy_s(nid.szTip, L"ComputerMqtt \u2014 Disconnected");
	Shell_NotifyIcon(NIM_ADD, &nid);
	nid.uVersion = NOTIFYICON_VERSION_4;
	Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

static void trayUpdate(HWND hwnd, bool connected)
{
	NOTIFYICONDATA nid{};
	nid.cbSize = sizeof(nid);
	nid.hWnd   = hwnd;
	nid.uID    = TRAY_ICON_ID;
	nid.uFlags = NIF_ICON | NIF_TIP | NIF_SHOWTIP;
	nid.hIcon  = LoadIcon(nullptr, connected ? IDI_INFORMATION : IDI_WARNING);
	wcscpy_s(nid.szTip, connected ? L"ComputerMqtt \u2014 Connected" : L"ComputerMqtt \u2014 Disconnected");
	Shell_NotifyIcon(NIM_MODIFY, &nid);
}

static void trayRemove(HWND hwnd)
{
	NOTIFYICONDATA nid{};
	nid.cbSize = sizeof(nid);
	nid.hWnd   = hwnd;
	nid.uID    = TRAY_ICON_ID;
	Shell_NotifyIcon(NIM_DELETE, &nid);
}

static BOOL WINAPI ctrlHandler(DWORD ctrlType)
{
	if (ctrlType == CTRL_C_EVENT || ctrlType == CTRL_CLOSE_EVENT)
	{
		PostThreadMessage(g_mainThreadId, WM_QUIT, 0, 0);
		return TRUE;
	}
	return FALSE;
}

static LRESULT CALLBACK msgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_TIMER && wParam == 1)
	{
		auto* mqtt = reinterpret_cast<ComputerMqtt*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		if (mqtt)
			mqtt->loop();

		return 0;
	}

	if (msg == WMAPP_SETCONNECTED)
	{
		trayUpdate(hwnd, wParam != 0);
		return 0;
	}

	if (msg == WMAPP_TRAYICON)
	{
		if (LOWORD(lParam) == WM_RBUTTONUP)
		{
			POINT pt{};
			GetCursorPos(&pt);
			HMENU menu = CreatePopupMenu();
			AppendMenuW(menu, MF_STRING, 1, L"Quit");
			SetForegroundWindow(hwnd);
			int cmd = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
				pt.x, pt.y, 0, hwnd, nullptr);
			// Required: clears the foreground-lock so SetForegroundWindow works next time.
			PostMessage(hwnd, WM_NULL, 0, 0);
			DestroyMenu(menu);
			if (cmd == 1)
				PostQuitMessage(0);
		}
		return 0;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		Log::write("Too few arguments: %d", argc);
		Log::write("Usage: %s <mqtt host> <port>", argv[0]);
		return -1;
	}

	const char* host = argv[1];
	const int port= atoi(argv[2]);
	if (port == 0)
	{
		Log::write("Invalid port specified");
		return -2;
	}

	Log::enableConsole();

	g_mainThreadId = GetCurrentThreadId();
	SetConsoleCtrlHandler(ctrlHandler, TRUE);

	// Hidden top-level window (0x0, never shown).
	// Must be a real top-level window, NOT HWND_MESSAGE, so that
	// SetForegroundWindow succeeds and TrackPopupMenu shows instantly.
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	WNDCLASSEX wc{};
	wc.cbSize        = sizeof(wc);
	wc.lpfnWndProc   = msgWndProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = L"ComputerMqttMsg";
	RegisterClassEx(&wc);

	HWND hwnd = CreateWindowEx(0, L"ComputerMqttMsg", nullptr, WS_POPUP,
		0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

	g_hwnd = hwnd;
	trayAdd(hwnd);

	Log::write("Starting");
	ComputerMqtt mqtt(host, port);

	SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&mqtt));
	SetTimer(hwnd, 1, 1000, nullptr);

	MSG winMsg{};
	while (GetMessage(&winMsg, nullptr, 0, 0))
		DispatchMessage(&winMsg);

	KillTimer(hwnd, 1);
	trayRemove(hwnd);
	DestroyWindow(hwnd);
	Log::write("Shutting down");
	return 0;
}
