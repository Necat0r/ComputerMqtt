#include "Mqtt.h"

#include <cstring>

#pragma comment(lib, "mosquittopp.lib")

Mqtt::Mqtt(const char * clientId, const char * host, int port, const Message& lastWill)
: mosquittopp(clientId, true /*clean session*/)
{
	mosqpp::lib_init();

	const bool retain = true;
	will_set(lastWill.topic.c_str(), strlen(lastWill.payload.c_str()), lastWill.payload.c_str(), AtLeastOnce, retain);

	connect_async(host, port);
	loop_start();
}

Mqtt::~Mqtt()
{
	loop_stop();
	mosqpp::lib_cleanup();
}

void Mqtt::subscribe(const char* topic)
{
	mosquittopp::subscribe(nullptr, topic, AtLeastOnce);
}

void Mqtt::publish(const Message & message, bool retain)
{
	mosquittopp::publish(nullptr, message.topic.c_str(), strlen(message.payload.c_str()), message.payload.c_str(), AtLeastOnce, retain);
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
	printf("MQTT %s\n", str);
}
