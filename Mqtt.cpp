#include "Mqtt.h"

#include <cstring>
#include <ctime>

Mqtt::Mqtt(const char * clientId, const char * host, int port)
: mosquittopp(clientId, true /*clean session*/)
{
	mosqpp::lib_init();

	// Home assistant seem intent on having a pointless password
	username_pw_set("ha", "ha");

	connect_async(host, port);
}

Mqtt::~Mqtt()
{
	mosqpp::lib_cleanup();
}

void Mqtt::loop()
{
	mosquittopp::loop(-1, 1000);
}

void Mqtt::subscribe(const char* topic)
{
	mosquittopp::subscribe(nullptr, topic, AtLeastOnce);
}

void Mqtt::publish(const Message & message, bool retain)
{
	mosquittopp::publish(nullptr, message.topic.c_str(), (int)strlen(message.payload.c_str()), message.payload.c_str(), AtLeastOnce, retain);
}

void Mqtt::setWills(const Wills& wills)
{
	// Set wills
	for (auto&& will : wills)
		will_set(will.topic.c_str(), (int)strlen(will.payload.c_str()), will.payload.c_str(), AtLeastOnce, Retain);
}

void Mqtt::on_connect(int rc)
{
	if (rc != 0)
		return;

	onConnected();
}

void Mqtt::on_message(const mosquitto_message * message)
{
	Message msg{ message->topic, std::string((const char*)message->payload, message->payloadlen) };
	onMessage(msg);
}

void Mqtt::on_log(int, const char * str)
{
	std::time_t now = std::time(nullptr);
	std::tm localTime{};

	if (localtime_s(&localTime, &now) == 0)
	{
		char timestamp[20]{};
		std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &localTime);
		printf("[%s] MQTT %s\n", timestamp, str);
		return;
	}

	printf("MQTT %s\n", str);
}
