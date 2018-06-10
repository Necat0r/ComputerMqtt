#define _CRT_SECURE_NO_WARNINGS

#include "Mqtt.h"
#include "Power.h"
#include "Monitor.h"
#include "Audio\AudioManager.h"

#include <algorithm>
#include <iostream>

static const char* ClientId = "computerMqtt";

static const char* PowerStatus = "computer/power/status";
static const char* PowerControl = "computer/power/control";
static const char* MonitorControl = "computer/monitor/control";

// Audio constants
// FIXME - Make output more generic. 
static const char* AudioOutputStatus = "computer/audio/output";
static const char* AudioOutputControl = "computer/audio/output/control";
static const char* AudioVolumeStatus = "computer/audio/volume";
static const char* AudioVolumeControl = "computer/audio/volume/control";
static const char* AudioMuteStatus = "computer/audio/mute";
static const char* AudioMuteControl = "computer/audio/mute/control";

const char* receiverDeviceId = "{0.0.0.00000000}.{ab6192b9-c22c-44c1-9086-14b4772011bf}";
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

class ComputerMqtt : public Mqtt
{
public:
	ComputerMqtt(const char* host, int port)
		: Mqtt(ClientId, host, port,
			// Last wills
			{
				{ PowerStatus, "true" },
				{ AudioOutputStatus, "false" },
				{ AudioVolumeStatus, "0" },
				{ AudioMuteStatus, "false" },
			})
		, m_audioManager([this](const AudioManager::DeviceInfo& info) { this->onAudioCallback(info); })
	{}

	virtual void onConnected() override
	{
		subscribe(PowerControl);
		subscribe(MonitorControl);

		// Audio topics
		subscribe(AudioOutputControl);
		subscribe(AudioVolumeControl);
		subscribe(AudioMuteControl);

		publish({ PowerStatus, "true" }, true);
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
	}

	void handlePower(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			printf("Could not parse data for topic: %s", PowerControl);
			return;
		}

		if (value == true)
			return;

		printf("Suspending machine");
		standby();
	}

	void handleMonitor(const Message& message)
	{
		bool value = false;
		if (!parseBool(value, message.payload.c_str()))
		{
			printf("Could not parse data for topic: %s", MonitorControl);
			return;
		}

		printf("Turning monitor %s\n", value ? "On" : "Off");
		setMonitorPower(value);
	}

	void onAudioCallback(const AudioManager::DeviceInfo& newInfo)
	{
		Mqtt::Message message;

		// DeviceId
		if (newInfo.deviceId != m_lastInfo.deviceId)
		{
			const bool receiverOutput = newInfo.deviceId == receiverDeviceId;
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
			std::cerr << "Could not parse data for topic: " << message.topic;
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
			std::cerr << "Failed parsing payload: \"" << message.payload << "\", for topic: " << message.topic;
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
			std::cerr << "Could not parse data for topic: " << message.topic;
			return;
		}

		m_audioManager.setMute(value);
	}

	AudioManager m_audioManager;
	AudioManager::DeviceInfo m_lastInfo;
};

int main(int argc, char* argv[])
{
	if (argc < 3)
	{
		printf("Too few arguments: %d\n", argc);
		printf("Usage:\n");
		printf("%s <mqtt host> <port>", argv[0]);
		return -1;
	}

	const char* host = argv[1];
	const int port= atoi(argv[2]);
	if (port == 0)
	{
		printf("Invalid port specified\n");
		return -2;
	}

	printf("Starting service\n");
	ComputerMqtt mqtt(host, port);

	while (true)
	{
		Sleep(1000);
	}

	return 0;
}
