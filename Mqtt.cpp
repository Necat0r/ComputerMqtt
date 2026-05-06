#include "Mqtt.h"
#include "Log.h"

#include <cstring>

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

void Mqtt::on_disconnect(int rc)
{
	onDisconnected();

	// rc != 0 means unexpected disconnect (e.g. network loss after suspend/resume).
	// Trigger an async reconnect so the next loop() call attempts to reconnect.
	if (rc != 0)
	{
		Log::write("MQTT unexpected disconnect (rc=%d), reconnecting...", rc);
		reconnect_async();
	}
}

void Mqtt::on_message(const mosquitto_message * message)
{
	Message msg{ message->topic, std::string((const char*)message->payload, message->payloadlen) };
	onMessage(msg);
}

void Mqtt::on_log(int, const char * str)
{
	Log::write("MQTT %s", str);
}
