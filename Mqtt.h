#pragma once

#include <string>
#include <mosquittopp.h>

class Mqtt : private mosqpp::mosquittopp
{
	enum Qos : int
	{
		AtMostOnce = 0,
		AtLeastOnce = 1,
		ExactlyOnce = 2,
	};

public:
	struct Message
	{
		std::string topic;
		std::string payload;
	};

	Mqtt(const char* clientId, const char* host, int port, const Message& lastWill);
	~Mqtt();

	void subscribe(const char* topic);
	void publish(const Message& message, bool retain = false);

protected:
	virtual void onConnected() = 0;
	virtual void onMessage(const Message& message) = 0;

private:
	virtual void on_connect(int rc) override;
	virtual void on_message(const struct mosquitto_message * message) override;
	virtual void on_log(int /*level*/, const char * str) override;
};
